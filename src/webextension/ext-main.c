/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2018 Daniel Carl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#include <jsc/jsc.h>
#include <gio/gio.h>
#include <glib.h>
#include <libsoup/soup.h>
#include <webkit/webkit-web-process-extension.h>

#include "ext-main.h"
#include "ext-util.h"
#include "js-snippets.h"

static gboolean on_authorize_authenticated_peer(GDBusAuthObserver *observer,
        GIOStream *stream, GCredentials *credentials, gpointer extension);
static void on_dbus_connection_created(GObject *source_object,
        GAsyncResult *result, gpointer data);
static void emit_page_created(GDBusConnection *connection, guint64 pageid);
static void emit_page_created_pending(GDBusConnection *connection);
static void queue_page_created_signal(guint64 pageid);
static WebKitWebPage *get_web_page_or_return_dbus_error(GDBusMethodInvocation *invocation,
        WebKitWebProcessExtension *extension, guint64 pageid);
static void dbus_handle_method_call(GDBusConnection *conn, const char *sender,
        const char *object_path, const char *interface_name, const char *method,
        GVariant *parameters, GDBusMethodInvocation *invocation, gpointer data);
static void on_page_created(WebKitWebProcessExtension *ext, WebKitWebPage *webpage, gpointer data);
static void on_web_page_document_loaded(WebKitWebPage *webpage, gpointer extension);
static gboolean on_web_page_user_message_received(WebKitWebPage *web_page,
        WebKitUserMessage *message, gpointer user_data);
static gboolean on_web_page_send_request(WebKitWebPage *webpage, WebKitURIRequest *request,
        WebKitURIResponse *response, gpointer extension);

static const GDBusInterfaceVTable interface_vtable = {
    dbus_handle_method_call,
    NULL,
    NULL
};

static const char introspection_xml[] =
    "<node>"
    " <interface name='" VB_WEBEXTENSION_INTERFACE "'>"
    "  <method name='EvalJs'>"
    "   <arg type='t' name='page_id' direction='in'/>"
    "   <arg type='s' name='js' direction='in'/>"
    "   <arg type='b' name='success' direction='out'/>"
    "   <arg type='s' name='result' direction='out'/>"
    "  </method>"
    "  <method name='EvalJsNoResult'>"
    "   <arg type='t' name='page_id' direction='in'/>"
    "   <arg type='s' name='js' direction='in'/>"
    "  </method>"
    "  <method name='FocusInput'>"
    "   <arg type='t' name='page_id' direction='in'/>"
    "  </method>"
    "  <signal name='PageCreated'>"
    "   <arg type='t' name='page_id' direction='out'/>"
    "  </signal>"
    "  <!-- VerticalScroll signal removed during DOM API migration."
    "       Scroll percentage no longer updates in statusbar (Top/Bot/XX%)."
    "       To restore: implement scroll tracking without DOM API. -->"
    "  <signal name='VerticalScroll'>"
    "   <arg type='t' name='page_id' direction='out'/>"
    "   <arg type='t' name='max' direction='out'/>"
    "   <arg type='q' name='percent' direction='out'/>"
    "   <arg type='t' name='top' direction='out'/>"
    "  </signal>"
    "  <method name='SetHeaderSetting'>"
    "   <arg type='s' name='headers' direction='in'/>"
    "  </method>"
    "  <method name='LockInput'>"
    "   <arg type='t' name='page_id' direction='in'/>"
    "   <arg type='s' name='elemend_id' direction='in'/>"
    "  </method>"
    "  <method name='UnlockInput'>"
    "   <arg type='t' name='page_id' direction='in'/>"
    "   <arg type='s' name='elemend_id' direction='in'/>"
    "  </method>"
    " </interface>"
    "</node>";

/* Global struct to hold internal used variables. */
struct Ext {
    guint               regid;
    GDBusConnection     *connection;
    GHashTable          *headers;
    /* GHashTable *documents removed - was only used for DOM API event listener tracking */
    GArray              *page_created_signals;
};
struct Ext ext = {0};


/**
 * Webextension entry point.
 * WebKitGTK 6.0: Function name changed from webkit_web_extension_initialize_with_user_data
 * to webkit_web_process_extension_initialize_with_user_data
 */
G_MODULE_EXPORT
void webkit_web_process_extension_initialize_with_user_data(WebKitWebProcessExtension *extension, GVariant *data)
{
    char *server_address;
    GDBusAuthObserver *observer;

    /* WebKitGTK 6.0: D-Bus removed - using WebKitUserMessage instead */
    g_variant_get(data, "(ms)", &server_address);

    /* Connect to page-created signal to handle new pages */
    g_signal_connect(extension, "page-created", G_CALLBACK(on_page_created), NULL);

    /* D-Bus connection code disabled - all IPC via WebKitUserMessage */
#if 0
    /* Write to a log file since stderr is not connected in sandboxed web process */
    FILE *logfile = fopen("/tmp/vimb_webext.log", "a");
    if (logfile) {
        fprintf(logfile, "WebExtension: Initializing with user data\n");
        fflush(logfile);
    }

    g_warning("WebExtension: Initializing with user data");
    g_variant_get(data, "(m&s)", &server_address);
    if (!server_address) {
        if (logfile) {
            fprintf(logfile, "WebExtension: UI process did not start D-Bus server\n");
            fflush(logfile);
            fclose(logfile);
        }
        g_warning("UI process did not start D-Bus server");
        return;
    }
    if (logfile) {
        fprintf(logfile, "WebExtension: D-Bus server address: %s\n", server_address);
        fflush(logfile);
    }
    g_warning("WebExtension: D-Bus server address: %s", server_address);

    g_signal_connect(extension, "page-created", G_CALLBACK(on_page_created), NULL);

    /* WebKitGTK 6.0 web process: Try synchronous connection since async callback
     * doesn't seem to be invoked in the sandboxed web process */
    GError *error = NULL;
    GDBusConnection *connection;
    int retry_count = 0;
    const int max_retries = 5;
    const int retry_delay_ms = 100;

    g_warning("WebExtension: Attempting SYNCHRONOUS connection to D-Bus server");

    /* Retry connection a few times in case server isn't ready yet */
    while (retry_count < max_retries) {
        if (retry_count > 0) {
            g_warning("WebExtension: Retry %d/%d after %dms", retry_count, max_retries, retry_delay_ms);
            g_usleep(retry_delay_ms * 1000);
        }

        connection = g_dbus_connection_new_for_address_sync(
            server_address,
            G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
            NULL,  /* guid */
            NULL,  /* cancellable */
            &error);

        if (connection) {
            break;  /* Success! */
        }

        if (error) {
            g_warning("WebExtension: Connection attempt %d failed: %s", retry_count + 1, error->message);
            g_error_free(error);
            error = NULL;
        }

        retry_count++;
    }

    if (!connection) {
        g_warning("WebExtension: Failed to connect after %d attempts", max_retries);
        return;
    }

    g_warning("WebExtension: Synchronous D-Bus connection established!");

    /* Register the webextension object */
    static GDBusNodeInfo *node_info = NULL;
    if (!node_info) {
        node_info = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
    }

    ext.regid = g_dbus_connection_register_object(
        connection,
        VB_WEBEXTENSION_OBJECT_PATH,
        node_info->interfaces[0],
        &interface_vtable,
        WEBKIT_WEB_PROCESS_EXTENSION(extension),
        NULL,
        &error);

    if (!ext.regid) {
        g_warning("WebExtension: Failed to register object: %s", error->message);
        g_error_free(error);
        g_object_unref(connection);
        return;
    }

    g_warning("WebExtension: Object registered, emitting pending signals");
    emit_page_created_pending(connection);
    ext.connection = connection;
    g_warning("WebExtension: Setup complete, ext.connection=%p", (void*)ext.connection);

    if (logfile) {
        fprintf(logfile, "WebExtension: Synchronous connection successful\n");
        fflush(logfile);
        fclose(logfile);
    }
#endif  /* D-Bus connection code disabled */
}

/* WebKitGTK 6.0: D-Bus authorization and connection code disabled - using WebKitUserMessage instead */
#if 0
static gboolean on_authorize_authenticated_peer(GDBusAuthObserver *observer,
        GIOStream *stream, GCredentials *credentials, gpointer extension)
{
    gboolean authorized = FALSE;

    g_warning("WebExtension: authorize_authenticated_peer called");

    if (credentials) {
        GCredentials *own_credentials;

        GError *error   = NULL;
        own_credentials = g_credentials_new();
        if (g_credentials_is_same_user(credentials, own_credentials, &error)) {
            authorized = TRUE;
            g_warning("WebExtension: Authorization successful");
        } else {
            g_warning("WebExtension: Failed to authorize connection: %s", error->message);
            g_error_free(error);
        }
        g_object_unref(own_credentials);
    } else {
        g_warning("WebExtension: No credentials received from UI process");
    }

    return authorized;
}

static void on_dbus_connection_created(GObject *source_object,
        GAsyncResult *result, gpointer data)
{
    static GDBusNodeInfo *node_info = NULL;
    GDBusConnection *connection;
    GError *error = NULL;

    g_warning("WebExtension: on_dbus_connection_created callback INVOKED");

    if (!node_info) {
        node_info = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
    }

    connection = g_dbus_connection_new_for_address_finish(result, &error);
    if (error) {
        g_warning("WebExtension: Failed to connect to UI process: %s", error->message);

        /* Log to file for debugging */
        FILE *logfile = fopen("/tmp/vimb_webext.log", "a");
        if (logfile) {
            fprintf(logfile, "WebExtension: Failed to connect: %s\n", error->message);
            fclose(logfile);
        }

        g_error_free(error);
        return;
    }
    g_warning("WebExtension: D-Bus connection established");

    /* register the webextension object */
    ext.regid = g_dbus_connection_register_object(
            connection,
            VB_WEBEXTENSION_OBJECT_PATH,
            node_info->interfaces[0],
            &interface_vtable,
            WEBKIT_WEB_PROCESS_EXTENSION(data),
            NULL,
            &error);

    if (!ext.regid) {
        g_warning("Failed to register web extension object: %s", error->message);
        g_error_free(error);
        g_object_unref(connection);
        return;
    }

    emit_page_created_pending(connection);
    ext.connection = connection;
    g_warning("WebExtension: D-Bus connection registered, ext.connection set");
}
#endif  /* End of D-Bus connection code */

/**
 * Emit the page created signal that is used in the UI process to finish the
 * dbus proxy connection.
 */
static void emit_page_created(GDBusConnection *connection, guint64 pageid)
{
    GError *error = NULL;

    /* propagate the signal over dbus */
    g_dbus_connection_emit_signal(G_DBUS_CONNECTION(connection), NULL,
            VB_WEBEXTENSION_OBJECT_PATH, VB_WEBEXTENSION_INTERFACE,
            "PageCreated", g_variant_new("(t)", pageid), &error);

    if (error) {
        g_warning("Failed to emit signal PageCreated: %s", error->message);
        g_error_free(error);
    }
}

/**
 * Emit queued page created signals.
 */
static void emit_page_created_pending(GDBusConnection *connection)
{
    int i;
    guint64 pageid;

    if (!ext.page_created_signals) {
        return;
    }

    for (i = 0; i < ext.page_created_signals->len; i++) {
        pageid = g_array_index(ext.page_created_signals, guint64, i);
        emit_page_created(connection, pageid);
    }

    g_array_free(ext.page_created_signals, TRUE);
    ext.page_created_signals = NULL;
}

/**
 * Write the page id of the created page to a queue to send them to the ui
 * process when the dbus connection is established.
 */
static void queue_page_created_signal(guint64 pageid)
{
    if (!ext.page_created_signals) {
        ext.page_created_signals = g_array_new(FALSE, FALSE, sizeof(guint64));
    }
    ext.page_created_signals = g_array_append_val(ext.page_created_signals, pageid);
}

static WebKitWebPage *get_web_page_or_return_dbus_error(GDBusMethodInvocation *invocation,
        WebKitWebProcessExtension *extension, guint64 pageid)
{
    /* WebKitGTK 6.0: Function renamed */
    WebKitWebPage *page = webkit_web_process_extension_get_page(extension, pageid);
    if (!page) {
        g_warning("invalid page id %lu", pageid);
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR,
                G_DBUS_ERROR_INVALID_ARGS, "Invalid page ID: %"G_GUINT64_FORMAT, pageid);
    }

    return page;
}

/**
 * Handle dbus method calls.
 */
static void dbus_handle_method_call(GDBusConnection *conn, const char *sender,
        const char *object_path, const char *interface_name, const char *method,
        GVariant *parameters, GDBusMethodInvocation *invocation, gpointer extension)
{
    char *value;
    guint64 pageid;
    WebKitWebPage *page;

    if (g_str_has_prefix(method, "EvalJs")) {
        char *result       = NULL;
        gboolean success;
        gboolean no_result;
        JSCValue *ref     = NULL;
        JSCContext *jsContext;

        g_variant_get(parameters, "(ts)", &pageid, &value);
        page = get_web_page_or_return_dbus_error(invocation, WEBKIT_WEB_PROCESS_EXTENSION(extension), pageid);
        if (!page) {
            return;
        }

        no_result = !g_strcmp0(method, "EvalJsNoResult");

        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        WebKitFrame *frame = webkit_web_page_get_main_frame(page);
        G_GNUC_END_IGNORE_DEPRECATIONS
        if (!frame) {
            g_warning("EvalJs: main frame is NULL for page %" G_GUINT64_FORMAT, pageid);
            if (no_result) {
                g_dbus_method_invocation_return_value(invocation, NULL);
            } else {
                g_dbus_method_invocation_return_value(invocation, g_variant_new("(bs)", FALSE, ""));
            }
            return;
        }

        /* WebKitGTK 6.0: Use page's main JavaScript context to access injected functions */
        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        jsContext = webkit_frame_get_js_context(frame);
        G_GNUC_END_IGNORE_DEPRECATIONS

        if (!jsContext) {
            g_warning("EvalJs: JavaScript context is NULL for page %" G_GUINT64_FORMAT, pageid);
            if (no_result) {
                g_dbus_method_invocation_return_value(invocation, NULL);
            } else {
                g_dbus_method_invocation_return_value(invocation, g_variant_new("(bs)", FALSE, "JavaScript context is NULL"));
            }
            return;
        }

        success = ext_util_js_eval(jsContext, value, &ref);

        if (no_result) {
            g_dbus_method_invocation_return_value(invocation, NULL);
        } else {
            result = ext_util_js_ref_to_string(jsContext, ref);
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(bs)", success, result));
            g_free(result);
        }
    } else if (!g_strcmp0(method, "FocusInput")) {
        JSCValue *result = NULL;
        JSCContext *jsContext;
        WebKitFrame *frame;

        g_variant_get(parameters, "(t)", &pageid);
        page = get_web_page_or_return_dbus_error(invocation, WEBKIT_WEB_PROCESS_EXTENSION(extension), pageid);
        if (!page) {
            return;
        }

        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        frame = webkit_web_page_get_main_frame(page);
        G_GNUC_END_IGNORE_DEPRECATIONS
        if (!frame) {
            g_warning("FocusInput: main frame is NULL for page %" G_GUINT64_FORMAT, pageid);
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", FALSE));
            return;
        }

        /* WebKitGTK 6.0: Use page's main JavaScript context to access injected functions */
        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        jsContext = webkit_frame_get_js_context(frame);
        G_GNUC_END_IGNORE_DEPRECATIONS

        /* Focus first visible input using minified JavaScript */
        ext_util_js_eval(jsContext, JS_FOCUS_INPUT, &result);
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (!g_strcmp0(method, "SetHeaderSetting")) {
        g_variant_get(parameters, "(s)", &value);

        if (ext.headers) {
            soup_header_free_param_list(ext.headers);
            ext.headers = NULL;
        }
        ext.headers = soup_header_parse_param_list(value);
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (!g_strcmp0(method, "LockInput")) {
        char *js;
        JSCValue *result = NULL;
        JSCContext *jsContext;
        WebKitFrame *frame;

        g_variant_get(parameters, "(ts)", &pageid, &value);
        page = get_web_page_or_return_dbus_error(invocation, WEBKIT_WEB_PROCESS_EXTENSION(extension), pageid);
        if (!page) {
            return;
        }

        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        frame = webkit_web_page_get_main_frame(page);
        G_GNUC_END_IGNORE_DEPRECATIONS
        if (!frame) {
            g_warning("LockInput: main frame is NULL for page %" G_GUINT64_FORMAT, pageid);
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", FALSE));
            return;
        }

        /* WebKitGTK 6.0: Use page's main JavaScript context to access injected functions */
        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        jsContext = webkit_frame_get_js_context(frame);
        G_GNUC_END_IGNORE_DEPRECATIONS

        /* Lock input using minified JavaScript - escape ID to prevent injection */
        char *escaped_id = g_strescape(value, NULL);
        js = g_strdup_printf(JS_LOCK_INPUT, escaped_id);
        ext_util_js_eval(jsContext, js, &result);
        g_free(js);
        g_free(escaped_id);

        g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (!g_strcmp0(method, "UnlockInput")) {
        char *js;
        JSCValue *result = NULL;
        JSCContext *jsContext;
        WebKitFrame *frame;

        g_variant_get(parameters, "(ts)", &pageid, &value);
        page = get_web_page_or_return_dbus_error(invocation, WEBKIT_WEB_PROCESS_EXTENSION(extension), pageid);
        if (!page) {
            return;
        }

        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        frame = webkit_web_page_get_main_frame(page);
        G_GNUC_END_IGNORE_DEPRECATIONS
        if (!frame) {
            g_warning("UnlockInput: main frame is NULL for page %" G_GUINT64_FORMAT, pageid);
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", FALSE));
            return;
        }

        /* WebKitGTK 6.0: Use page's main JavaScript context to access injected functions */
        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        jsContext = webkit_frame_get_js_context(frame);
        G_GNUC_END_IGNORE_DEPRECATIONS

        /* Unlock input using minified JavaScript - escape ID to prevent injection */
        char *escaped_id = g_strescape(value, NULL);
        js = g_strdup_printf(JS_UNLOCK_INPUT, escaped_id);
        ext_util_js_eval(jsContext, js, &result);
        g_free(js);
        g_free(escaped_id);

        g_dbus_method_invocation_return_value(invocation, NULL);
    }
}

/**
 * Callback for web extensions page-created signal.
 */
static void on_page_created(WebKitWebProcessExtension *extension, WebKitWebPage *webpage, gpointer data)
{
    guint64 pageid;

    if (!webpage) {
        g_warning("on_page_created called with NULL webpage!");
        return;
    }

    if (!extension) {
        g_warning("on_page_created called with NULL extension!");
        return;
    }

    pageid = webkit_web_page_get_id(webpage);
    g_warning("WebExtension: Page created, page_id=%" G_GUINT64_FORMAT, pageid);

    /* WebKitGTK 6.0: D-Bus signal emission removed - using WebKitUserMessage only */

    g_object_connect(webpage,
            "signal::send-request", G_CALLBACK(on_web_page_send_request), extension,
            "signal::document-loaded", G_CALLBACK(on_web_page_document_loaded), extension,
            "signal::user-message-received", G_CALLBACK(on_web_page_user_message_received), extension,
            NULL);
}

/**
 * Callback for web pages document-loaded signal.
 */
static void on_web_page_document_loaded(WebKitWebPage *webpage, gpointer extension)
{
    guint64 pageid = webkit_web_page_get_id(webpage);
    WebKitUserMessage *message;

    g_warning("WebExtension: Document loaded, sending page-document-loaded message, page_id=%" G_GUINT64_FORMAT, pageid);

    /* Send message with page ID as parameter.
     * This allows the main process to match this page with the correct dbus proxy. */
    message = webkit_user_message_new("page-document-loaded",
                                      g_variant_new("(t)", pageid));
    webkit_web_page_send_message_to_view(webpage, message, NULL, NULL, NULL);
    g_warning("WebExtension: page-document-loaded message sent");
}

/**
 * Handle user messages sent from the UI process to the webextension.
 * This replaces D-Bus method calls for WebKitGTK 6.0.
 */
__attribute__((used))
static gboolean on_web_page_user_message_received(WebKitWebPage *web_page,
                                                   WebKitUserMessage *message,
                                                   gpointer user_data)
{
    const char *name = webkit_user_message_get_name(message);
    GVariant *parameters = webkit_user_message_get_parameters(message);

    g_warning("WebExtension: Received message '%s' from UI process", name);

    /* EvalJs - Evaluate JavaScript and return result */
    if (g_strcmp0(name, "EvalJs") == 0) {
        const char *js_code;
        WebKitFrame *frame;
        JSCContext *js_context;
        JSCValue *result = NULL;
        gboolean success = FALSE;

        g_variant_get(parameters, "(&s)", &js_code);
        g_warning("WebExtension: EvalJs request: %s", js_code);

        frame = webkit_web_page_get_main_frame(web_page);
        if (frame) {
            G_GNUC_BEGIN_IGNORE_DEPRECATIONS
            js_context = webkit_frame_get_js_context(frame);
            G_GNUC_END_IGNORE_DEPRECATIONS

            if (js_context) {
                success = ext_util_js_eval(js_context, js_code, &result);

                if (success && result) {
                    char *result_str = ext_util_js_ref_to_string(js_context, result);
                    WebKitUserMessage *reply = webkit_user_message_new("EvalJs-reply",
                        g_variant_new("(bs)", TRUE, result_str ? result_str : ""));
                    webkit_user_message_send_reply(message, reply);
                    g_free(result_str);
                    return TRUE;
                }
            }
        }

        /* Error case */
        WebKitUserMessage *reply = webkit_user_message_new("EvalJs-reply",
            g_variant_new("(bs)", FALSE, ""));
        webkit_user_message_send_reply(message, reply);
        return TRUE;
    }

    /* EvalJsNoResult - Evaluate JavaScript without waiting for result */
    if (g_strcmp0(name, "EvalJsNoResult") == 0) {
        const char *js_code;
        WebKitFrame *frame;
        JSCContext *js_context;

        g_variant_get(parameters, "(&s)", &js_code);
        g_warning("WebExtension: EvalJsNoResult request: %s", js_code);

        frame = webkit_web_page_get_main_frame(web_page);
        if (frame) {
            G_GNUC_BEGIN_IGNORE_DEPRECATIONS
            js_context = webkit_frame_get_js_context(frame);
            G_GNUC_END_IGNORE_DEPRECATIONS

            if (js_context) {
                ext_util_js_eval(js_context, js_code, NULL);
            }
        }
        return TRUE;
    }

    /* FocusInput - Focus an input element */
    if (g_strcmp0(name, "FocusInput") == 0) {
        WebKitFrame *frame = webkit_web_page_get_main_frame(web_page);
        JSCContext *js_context;

        g_warning("WebExtension: FocusInput request");

        if (frame) {
            G_GNUC_BEGIN_IGNORE_DEPRECATIONS
            js_context = webkit_frame_get_js_context(frame);
            G_GNUC_END_IGNORE_DEPRECATIONS

            if (js_context) {
                ext_util_js_eval(js_context, JS_FOCUS_INPUT, NULL);
            }
        }
        return TRUE;
    }

    /* LockInput - Lock an input element */
    if (g_strcmp0(name, "LockInput") == 0) {
        const char *element_id;
        WebKitFrame *frame;
        JSCContext *js_context;

        g_variant_get(parameters, "(&s)", &element_id);
        g_warning("WebExtension: LockInput request for element: %s", element_id);

        frame = webkit_web_page_get_main_frame(web_page);
        if (frame) {
            G_GNUC_BEGIN_IGNORE_DEPRECATIONS
            js_context = webkit_frame_get_js_context(frame);
            G_GNUC_END_IGNORE_DEPRECATIONS

            if (js_context) {
                char *escaped_id = g_strescape(element_id, NULL);
                char *js = g_strdup_printf(JS_LOCK_INPUT, escaped_id);
                ext_util_js_eval(js_context, js, NULL);
                g_free(js);
                g_free(escaped_id);
            }
        }
        return TRUE;
    }

    /* UnlockInput - Unlock an input element */
    if (g_strcmp0(name, "UnlockInput") == 0) {
        const char *element_id;
        WebKitFrame *frame;
        JSCContext *js_context;

        g_variant_get(parameters, "(&s)", &element_id);
        g_warning("WebExtension: UnlockInput request for element: %s", element_id);

        frame = webkit_web_page_get_main_frame(web_page);
        if (frame) {
            G_GNUC_BEGIN_IGNORE_DEPRECATIONS
            js_context = webkit_frame_get_js_context(frame);
            G_GNUC_END_IGNORE_DEPRECATIONS

            if (js_context) {
                char *escaped_id = g_strescape(element_id, NULL);
                char *js = g_strdup_printf(JS_UNLOCK_INPUT, escaped_id);
                ext_util_js_eval(js_context, js, NULL);
                g_free(js);
                g_free(escaped_id);
            }
        }
        return TRUE;
    }

    g_warning("WebExtension: Unknown message '%s'", name);
    return FALSE;
}

/**
 * Callback for web pages send-request signal.
 */
static gboolean on_web_page_send_request(WebKitWebPage *webpage, WebKitURIRequest *request,
        WebKitURIResponse *response, gpointer extension)
{
    char *name, *value;
    SoupMessageHeaders *headers;
    GHashTableIter iter;

    if (!ext.headers) {
        return FALSE;
    }

    /* Change request headers according to the users preferences. */
    headers = webkit_uri_request_get_http_headers(request);
    if (!headers) {
        return FALSE;
    }

    g_hash_table_iter_init(&iter, ext.headers);
    while (g_hash_table_iter_next(&iter, (gpointer*)&name, (gpointer*)&value)) {
        /* Null value is used to indicate that the header should be
         * removed completely. */
        if (value == NULL) {
            soup_message_headers_remove(headers, name);
        } else {
            soup_message_headers_replace(headers, name, value);
        }
    }

    return FALSE;
}
