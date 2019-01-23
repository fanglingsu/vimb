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

static gboolean on_authorize_authenticated_peer(GDBusAuthObserver *observer,
        GIOStream *stream, GCredentials *credentials, gpointer extension);
static void on_dbus_connection_created(GObject *source_object,
        GAsyncResult *result, gpointer data);
static void add_onload_event_observers(WebKitDOMDocument *doc,
        WebKitWebPage *page);
static void on_document_scroll(WebKitDOMEventTarget *target, WebKitDOMEvent *event,
        WebKitWebPage *page);
static void emit_page_created(GDBusConnection *connection, guint64 pageid);
static void emit_page_created_pending(GDBusConnection *connection);
static void queue_page_created_signal(guint64 pageid);
static void dbus_emit_signal(const char *name, GVariant *data);
static WebKitWebPage *get_web_page_or_return_dbus_error(GDBusMethodInvocation *invocation,
        WebKitWebExtension *extension, guint64 pageid);
static void dbus_handle_method_call(GDBusConnection *conn, const char *sender,
        const char *object_path, const char *interface_name, const char *method,
        GVariant *parameters, GDBusMethodInvocation *invocation, gpointer data);
static void on_editable_change_focus(WebKitDOMEventTarget *target,
        WebKitDOMEvent *event, WebKitWebPage *page);
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
    GHashTable          *documents;
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
 * Add observers to doc event for given document and all the contained iframes
 * too.
 */
static void add_onload_event_observers(WebKitDOMDocument *doc,
        WebKitWebPage *page)
{
    WebKitDOMEventTarget *target;

    /* Add the document to the table of known documents or if already exists
     * return to not apply observers multiple times. */
    if (!g_hash_table_add(ext.documents, doc)) {
        return;
    }

    /* We have to use default view instead of the document itself in case this
     * function is called with content document of an iframe. Else the event
     * observing does not work. */
    target = WEBKIT_DOM_EVENT_TARGET(webkit_dom_document_get_default_view(doc));

    webkit_dom_event_target_add_event_listener(target, "focus",
            G_CALLBACK(on_editable_change_focus), TRUE, page);
    webkit_dom_event_target_add_event_listener(target, "blur",
            G_CALLBACK(on_editable_change_focus), TRUE, page);
    /* Check for focused editable elements also if they where focused before
     * the event observer where set up. */
    /* TODO this is not needed for strict-focus=on */
    on_editable_change_focus(target, NULL, page);

    /* Observe scroll events to get current position in the document. */
    webkit_dom_event_target_add_event_listener(target, "scroll",
            G_CALLBACK(on_document_scroll), FALSE, page);
    /* Call the callback explicitly to make sure we have the right position
     * shown in statusbar also in cases the user does not scroll. */
    on_document_scroll(target, NULL, page);
}

/**
 * Callback called when the document is scrolled.
 */
static void on_document_scroll(WebKitDOMEventTarget *target, WebKitDOMEvent *event,
        WebKitWebPage *page)
{
    WebKitDOMDocument *doc;

    if (WEBKIT_DOM_IS_DOM_WINDOW(target)) {
        g_object_get(target, "document", &doc, NULL);
    } else {
        /* target is a doc document */
        doc = WEBKIT_DOM_DOCUMENT(target);
    }

    if (doc) {
        WebKitDOMElement *body, *de;
        glong max = 0, top = 0, scrollTop, scrollHeight, clientHeight;
        guint percent = 0;

        de = webkit_dom_document_get_document_element(doc);
        if (!de) {
            return;
        }

        body = WEBKIT_DOM_ELEMENT(webkit_dom_document_get_body(doc));
        if (!body) {
            return;
        }

        scrollTop = MAX(webkit_dom_element_get_scroll_top(de),
                webkit_dom_element_get_scroll_top(body));

        clientHeight = webkit_dom_dom_window_get_inner_height(
                webkit_dom_document_get_default_view(doc));

        scrollHeight = MAX(webkit_dom_element_get_scroll_height(de),
                webkit_dom_element_get_scroll_height(body));

        /* Get the maximum scrollable page size. This is the size of the whole
         * document - height of the viewport. */
        max = scrollHeight - clientHeight;
        if (max > 0) {
            percent = (guint)(0.5 + (scrollTop * 100 / max));
            top = scrollTop;
        }

        dbus_emit_signal("VerticalScroll", g_variant_new("(ttqt)",
                webkit_web_page_get_id(page), max, percent, top));
    }
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

/**
 * Emits a signal over dbus.
 *
 * @name:   Signal name to emit.
 * @data:   GVariant value used as value for the signal or NULL.
 */
static void dbus_emit_signal(const char *name, GVariant *data)
{
    GError *error = NULL;

    if (!ext.connection) {
        return;
    }

    /* propagate the signal over dbus */
    g_dbus_connection_emit_signal(ext.connection, NULL,
            VB_WEBEXTENSION_OBJECT_PATH, VB_WEBEXTENSION_INTERFACE, name,
            data, &error);
    if (error) {
        g_warning("Failed to emit signal '%s': %s", name, error->message);
        g_error_free(error);
    }
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
            char *result = NULL;
            /* TODO how to check if an exception was thrown to get it's
             * message? */
            if (!jsc_value_is_undefined(js_value)) {
                result = jsc_value_to_string(js_value);
            }
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(bs)", TRUE, result ? result : "undefined"));
            g_free(result);
        }
        g_object_unref(js_value);
    } else if (!g_strcmp0(method, "FocusInput")) {
        g_variant_get(parameters, "(t)", &pageid);
        page = get_web_page_or_return_dbus_error(invocation, WEBKIT_WEB_EXTENSION(extension), pageid);
        if (!page) {
            return;
        }
        ext_dom_focus_input(webkit_web_page_get_dom_document(page));
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
        g_variant_get(parameters, "(ts)", &pageid, &value);
        page = get_web_page_or_return_dbus_error(invocation, WEBKIT_WEB_EXTENSION(extension), pageid);
        if (!page) {
            return;
        }
        ext_dom_lock_input(webkit_web_page_get_dom_document(page), value);
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else if (!g_strcmp0(method, "UnlockInput")) {
        g_variant_get(parameters, "(ts)", &pageid, &value);
        page = get_web_page_or_return_dbus_error(invocation, WEBKIT_WEB_EXTENSION(extension), pageid);
        if (!page) {
            return;
        }
        ext_dom_unlock_input(webkit_web_page_get_dom_document(page), value);
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
}

/**
 * Callback called if a editable element changes it focus state.
 * Event target may be a WebKitDOMDocument (in case of iframe) or a
 * WebKitDOMDOMWindow.
 */
static void on_editable_change_focus(WebKitDOMEventTarget *target,
        WebKitDOMEvent *event, WebKitWebPage *page)
{
    WebKitDOMDocument *doc;
    WebKitDOMDOMWindow *dom_window;
    WebKitDOMElement *active;
    GVariant *variant;
    char *message;

    if (WEBKIT_DOM_IS_DOM_WINDOW(target)) {
        g_object_get(target, "document", &doc, NULL);
    } else {
        /* target is a doc document */
        doc = WEBKIT_DOM_DOCUMENT(target);
    }

    dom_window = webkit_dom_document_get_default_view(doc);
    if (!dom_window) {
        return;
    }

    active = webkit_dom_document_get_active_element(doc);
    /* Don't do anything if there is no active element */
    if (!active) {
        return;
    }
    if (WEBKIT_DOM_IS_HTML_IFRAME_ELEMENT(active)) {
        WebKitDOMHTMLIFrameElement *iframe;
        WebKitDOMDocument *subdoc;

        iframe = WEBKIT_DOM_HTML_IFRAME_ELEMENT(active);
        subdoc = webkit_dom_html_iframe_element_get_content_document(iframe);
        add_onload_event_observers(subdoc, page);
        return;
    }

    /* Check if the active element is an editable element. */
    variant = g_variant_new("(tb)", webkit_web_page_get_id(page),
            ext_dom_is_editable(active));
    message = g_variant_print(variant, FALSE);
    g_variant_unref(variant);
    if (!webkit_dom_dom_window_webkit_message_handlers_post_message(dom_window, "focus", message)) {
        g_warning("Error sending focus message");
    }
    g_free(message);
    g_object_unref(dom_window);
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
    /* If there is a hashtable of known document - detroy this and create a
     * new hashtable. */
    if (ext.documents) {
        g_hash_table_unref(ext.documents);
    }
    ext.documents = g_hash_table_new(g_direct_hash, g_direct_equal);

    add_onload_event_observers(webkit_web_page_get_dom_document(webpage), webpage);
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
