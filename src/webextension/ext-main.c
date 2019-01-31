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
#include "ext-dom.h"
#include "ext-util.h"
#include "../scripts/scripts.h"

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
static void on_window_object_cleared(WebKitScriptWorld *world,
        WebKitWebPage *webpage, WebKitFrame *frame, gpointer user_data);
static void js_exception_handler(JSCContext *context, JSCException *exception);
static char *form_message_serializer(guint64 page_id, gboolean is_focused);
static char *scroll_message_serializer(guint64 page_id, glong max,
        guint percent, glong top);

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
    "  <method name='SetHeaderSetting'>"
    "   <arg type='s' name='headers' direction='in'/>"
    "  </method>"
    " </interface>"
    "</node>";

/* Global struct to hold internal used variables. */
struct Ext {
    guint               regid;
    GDBusConnection     *connection;
    GHashTable          *headers;
    GArray              *page_created_signals;
    WebKitScriptWorld   *script_world;
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

    ext.script_world = webkit_script_world_get_default();

    g_signal_connect(extension, "page-created", G_CALLBACK(on_page_created), NULL);
    g_signal_connect(ext.script_world, "window-object-cleared",
            G_CALLBACK(on_window_object_cleared), NULL);

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
        gboolean no_result;
        JSCContext *js_context;
        JSCValue *js_value;
        JSCException *js_exception;

        g_variant_get(parameters, "(ts)", &pageid, &value);
        page = get_web_page_or_return_dbus_error(invocation, WEBKIT_WEB_EXTENSION(extension), pageid);
        if (!page) {
            return;
        }

        no_result  = !g_strcmp0(method, "EvalJsNoResult");
        js_context = webkit_frame_get_js_context(webkit_web_page_get_main_frame(page));
        js_value   = jsc_context_evaluate(js_context, value, -1);

        if (no_result) {
            g_dbus_method_invocation_return_value(invocation, NULL);
        } else {
            gboolean success = TRUE;
            char *result     = NULL;

            js_exception = jsc_context_get_exception(js_context);
            if (js_exception) {
                success = FALSE;
                result  = jsc_exception_to_string(js_exception);
                jsc_context_clear_exception(js_context);
            } else if (!jsc_value_is_undefined(js_value)) {
                result = jsc_value_to_string(js_value);
            }

            g_dbus_method_invocation_return_value(invocation,
                    g_variant_new("(bs)", success, result ? result : ""));
            g_free(result);
        }
        g_object_unref(js_context);
        g_object_unref(js_value);
    } else if (!g_strcmp0(method, "FocusInput")) {
        JSCContext *js_context;
        JSCValue *js_vimb, *js_result;

        g_variant_get(parameters, "(t)", &pageid);
        page = get_web_page_or_return_dbus_error(invocation,
                WEBKIT_WEB_EXTENSION(extension), pageid);
        if (!page) {
            return;
        }

        js_context = webkit_frame_get_js_context(webkit_web_page_get_main_frame(page));
        js_vimb    = jsc_context_get_value(js_context, "Vimb");
        js_result  = jsc_value_object_invoke_method(js_vimb, "focusInput",
                G_TYPE_NONE);

        g_dbus_method_invocation_return_value(invocation, NULL);

        g_object_unref(js_result);
        g_object_unref(js_vimb);
        g_object_unref(js_context);
    } else if (!g_strcmp0(method, "SetHeaderSetting")) {
        g_variant_get(parameters, "(s)", &value);

        if (ext.headers) {
            soup_header_free_param_list(ext.headers);
            ext.headers = NULL;
        }
        ext.headers = soup_header_parse_param_list(value);
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
}

/**
 * Callback for web extensions page-created signal.
 */
static void on_page_created(WebKitWebExtension *extension, WebKitWebPage *webpage, gpointer data)
{
    guint64 pageid = webkit_web_page_get_id(webpage);

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
    WebKitFrame *frame;
    JSCContext *js_context;
    JSCValue *js_result,
             *js_vimb_toplevel,
             *js_scroll_serializer;

    frame            = webkit_web_page_get_main_frame(webpage);
    js_context       = webkit_frame_get_js_context_for_script_world(frame, ext.script_world);
    js_vimb_toplevel = jsc_context_get_value(js_context, "VimbToplevel");

    js_scroll_serializer = jsc_value_new_function(js_context,
            "scrollMessageSerializer",
            G_CALLBACK(scroll_message_serializer), NULL, NULL,
            G_TYPE_STRING, 4,
            G_TYPE_UINT64, G_TYPE_ULONG, G_TYPE_UINT, G_TYPE_ULONG);
    js_result = jsc_value_object_invoke_method(js_vimb_toplevel,
            "applyScrollObservers",
            G_TYPE_UINT64, webkit_web_page_get_id(webpage),
            JSC_TYPE_VALUE, js_scroll_serializer,
            G_TYPE_NONE);

    g_object_unref(js_vimb_toplevel);
    g_object_unref(js_scroll_serializer);
    g_object_unref(js_result);
    g_object_unref(js_context);
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

static void on_window_object_cleared(WebKitScriptWorld *world,
        WebKitWebPage *webpage, WebKitFrame *frame, gpointer user_data)
{
    JSCContext *js_context;
    JSCValue *js_result,
             *js_vimb,
             *js_form_serializer;

    /* Inject out Vimb JS obejct. */
    js_context = webkit_frame_get_js_context_for_script_world(frame, world);

    jsc_context_push_exception_handler(js_context,
            (JSCExceptionHandler)js_exception_handler, NULL, NULL);

    js_result = jsc_context_evaluate(js_context, JS_VIMB, -1);
    g_object_unref(js_result);

    js_vimb            = jsc_context_get_value(js_context, "Vimb");
    js_form_serializer = jsc_value_new_function(js_context,
            "formMessageSerializer",
            G_CALLBACK(form_message_serializer), NULL, NULL,
            G_TYPE_STRING, 2,
            G_TYPE_UINT64, G_TYPE_BOOLEAN);
    js_result = jsc_value_object_invoke_method(js_vimb,
            "applyFocusChangeObservers",
            G_TYPE_UINT64, webkit_web_page_get_id(webpage),
            JSC_TYPE_VALUE, js_form_serializer,
            G_TYPE_NONE);
    g_object_unref(js_result);
    g_object_unref(js_form_serializer);
    g_object_unref(js_vimb);

    js_result = jsc_context_evaluate(js_context, JS_HINTS, -1);
    g_object_unref(js_result);

    /* Inject toplevel script only in the main frame. */
    if (webkit_frame_is_main_frame(frame)) {
        /* Inject the toplevel script. */
        js_result = jsc_context_evaluate(js_context, JS_VIMB_TOPLEVEL, -1);
        g_object_unref(js_result);
    }

    g_object_unref(js_context);
}

static void js_exception_handler(JSCContext *context, JSCException *exception)
{
    JSCValue *js_console,
             *js_result;

    js_console = jsc_context_get_value(context, "console");
    js_result  = jsc_value_object_invoke_method(js_console, "error", JSC_TYPE_EXCEPTION, exception, G_TYPE_NONE);
    g_object_unref(js_result);
    g_object_unref(js_console);

    g_warning("JavaScriptException: %s", jsc_exception_report(exception));

    jsc_context_throw_exception(context, exception);
}

static char *form_message_serializer(guint64 page_id, gboolean is_focused)
{
    GVariant *variant;
    char *message;

    variant = g_variant_new("(tb)", page_id, is_focused);
    message = g_variant_print(variant, FALSE);
    g_variant_unref(variant);

    return message;
}

static char *scroll_message_serializer(guint64 page_id, glong max,
        guint percent, glong top)
{
    GVariant *variant;
    char *message;
    variant = g_variant_new("(ttqt)", page_id, max, percent, top);
    message = g_variant_print(variant, FALSE);
    g_variant_unref(variant);

    return message;
}
