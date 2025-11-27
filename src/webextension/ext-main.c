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

#include <JavaScriptCore/JavaScript.h>
#include <gio/gio.h>
#include <glib.h>
#include <libsoup/soup.h>
#include <webkit2/webkit-web-extension.h>

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
        WebKitWebExtension *extension, guint64 pageid);
static void dbus_handle_method_call(GDBusConnection *conn, const char *sender,
        const char *object_path, const char *interface_name, const char *method,
        GVariant *parameters, GDBusMethodInvocation *invocation, gpointer data);
static void on_page_created(WebKitWebExtension *ext, WebKitWebPage *webpage, gpointer data);
static void on_web_page_document_loaded(WebKitWebPage *webpage, gpointer extension);
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
 */
G_MODULE_EXPORT
void webkit_web_extension_initialize_with_user_data(WebKitWebExtension *extension, GVariant *data)
{
    char *server_address;
    GDBusAuthObserver *observer;

    g_variant_get(data, "(m&s)", &server_address);
    if (!server_address) {
        g_warning("UI process did not start D-Bus server");
        return;
    }

    g_signal_connect(extension, "page-created", G_CALLBACK(on_page_created), NULL);

    observer = g_dbus_auth_observer_new();
    g_signal_connect(observer, "authorize-authenticated-peer",
            G_CALLBACK(on_authorize_authenticated_peer), extension);

    g_dbus_connection_new_for_address(server_address,
            G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT, observer, NULL,
            (GAsyncReadyCallback)on_dbus_connection_created, extension);
    g_object_unref(observer);
}

static gboolean on_authorize_authenticated_peer(GDBusAuthObserver *observer,
        GIOStream *stream, GCredentials *credentials, gpointer extension)
{
    gboolean authorized = FALSE;
    if (credentials) {
        GCredentials *own_credentials;

        GError *error   = NULL;
        own_credentials = g_credentials_new();
        if (g_credentials_is_same_user(credentials, own_credentials, &error)) {
            authorized = TRUE;
        } else {
            g_warning("Failed to authorize web extension connection: %s", error->message);
            g_error_free(error);
        }
        g_object_unref(own_credentials);
    } else {
        g_warning ("No credentials received from UI process.\n");
    }

    return authorized;
}

static void on_dbus_connection_created(GObject *source_object,
        GAsyncResult *result, gpointer data)
{
    static GDBusNodeInfo *node_info = NULL;
    GDBusConnection *connection;
    GError *error = NULL;

    if (!node_info) {
        node_info = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
    }

    connection = g_dbus_connection_new_for_address_finish(result, &error);
    if (error) {
        g_warning("Failed to connect to UI process: %s", error->message);
        g_error_free(error);
        return;
    }

    /* register the webextension object */
    ext.regid = g_dbus_connection_register_object(
            connection,
            VB_WEBEXTENSION_OBJECT_PATH,
            node_info->interfaces[0],
            &interface_vtable,
            WEBKIT_WEB_EXTENSION(data),
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
}

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
        WebKitWebExtension *extension, guint64 pageid)
{
    WebKitWebPage *page = webkit_web_extension_get_page(extension, pageid);
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
        JSValueRef ref     = NULL;
        JSGlobalContextRef jsContext;

        g_variant_get(parameters, "(ts)", &pageid, &value);
        page = get_web_page_or_return_dbus_error(invocation, WEBKIT_WEB_EXTENSION(extension), pageid);
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

        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        jsContext = webkit_frame_get_javascript_context_for_script_world(
            frame,
            webkit_script_world_get_default()
        );
        G_GNUC_END_IGNORE_DEPRECATIONS

        success = ext_util_js_eval(jsContext, value, &ref);

        if (no_result) {
            g_dbus_method_invocation_return_value(invocation, NULL);
        } else {
            result = ext_util_js_ref_to_string(jsContext, ref);
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(bs)", success, result));
            g_free(result);
        }
    } else if (!g_strcmp0(method, "FocusInput")) {
        JSValueRef result = NULL;
        JSGlobalContextRef jsContext;
        WebKitFrame *frame;

        g_variant_get(parameters, "(t)", &pageid);
        page = get_web_page_or_return_dbus_error(invocation, WEBKIT_WEB_EXTENSION(extension), pageid);
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

        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        jsContext = webkit_frame_get_javascript_context_for_script_world(
            frame,
            webkit_script_world_get_default()
        );
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
        JSValueRef result = NULL;
        JSGlobalContextRef jsContext;
        WebKitFrame *frame;

        g_variant_get(parameters, "(ts)", &pageid, &value);
        page = get_web_page_or_return_dbus_error(invocation, WEBKIT_WEB_EXTENSION(extension), pageid);
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

        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        jsContext = webkit_frame_get_javascript_context_for_script_world(
            frame,
            webkit_script_world_get_default()
        );
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
        JSValueRef result = NULL;
        JSGlobalContextRef jsContext;
        WebKitFrame *frame;

        g_variant_get(parameters, "(ts)", &pageid, &value);
        page = get_web_page_or_return_dbus_error(invocation, WEBKIT_WEB_EXTENSION(extension), pageid);
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

        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        jsContext = webkit_frame_get_javascript_context_for_script_world(
            frame,
            webkit_script_world_get_default()
        );
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
static void on_page_created(WebKitWebExtension *extension, WebKitWebPage *webpage, gpointer data)
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

    if (ext.connection) {
        emit_page_created(ext.connection, pageid);
    } else {
        queue_page_created_signal(pageid);
    }

    g_object_connect(webpage,
            "signal::send-request", G_CALLBACK(on_web_page_send_request), extension,
            "signal::document-loaded", G_CALLBACK(on_web_page_document_loaded), extension,
            NULL);
}

/**
 * Callback for web pages document-loaded signal.
 */
static void on_web_page_document_loaded(WebKitWebPage *webpage, gpointer extension)
{
    guint64 pageid = webkit_web_page_get_id(webpage);
    WebKitUserMessage *message;

    /* Send message with page ID as parameter.
     * This allows the main process to match this page with the correct dbus proxy. */
    message = webkit_user_message_new("page-document-loaded",
                                      g_variant_new("(t)", pageid));
    webkit_web_page_send_message_to_view(webpage, message, NULL, NULL, NULL);
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
