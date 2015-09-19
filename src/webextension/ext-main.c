/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2015 Daniel Carl
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

static void add_onload_event_observers(WebKitDOMDocument *doc);
static void dbus_emit_signal(const char *name, GVariant *data);
static void dbus_handle_method_call(GDBusConnection *conn, const char *sender,
        const char *object_path, const char *interface_name, const char *method,
        GVariant *parameters, GDBusMethodInvocation *invocation, gpointer data);
static gboolean dbus_own_name_sync(GDBusConnection *connection, const char *name,
        GBusNameOwnerFlags flags);
static void on_dbus_name_acquire(GDBusConnection *connection, const char *name, gpointer data);
static void on_editable_change_focus(WebKitDOMEventTarget *target, WebKitDOMEvent *event);
static void on_page_created(WebKitWebExtension *ext, WebKitWebPage *page, gpointer data);
static void on_web_page_document_loaded(WebKitWebPage *page, gpointer data);
static gboolean on_web_page_send_request(WebKitWebPage *page, WebKitURIRequest *request,
        WebKitURIResponse *response, gpointer data);

static const GDBusInterfaceVTable interface_vtable = {
    dbus_handle_method_call,
    NULL,
    NULL
};

static const char introspection_xml[] =
    "<node>"
    " <interface name='" VB_WEBEXTENSION_INTERFACE "'>"
    "  <signal name='EditableChangeFocus'>"
    "   <arg type='b' name='focused' direction='out'/>"
    "  </signal>"
    "  <method name='FocusInput'>"
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
    WebKitWebPage       *webpage;
    WebKitDOMElement    *active;
    GHashTable          *headers;
    GHashTable          *documents;
    gboolean            input_focus;
};
struct Ext ext = {0};


/**
 * Webextension entry point.
 */
G_MODULE_EXPORT
void webkit_web_extension_initialize_with_user_data(WebKitWebExtension *extension, GVariant *data)
{
    char *extid, *service_name;
    g_variant_get(data, "(s)", &extid);

    /* Get the DBus connection for the bus type. It would be a better to use
     * the async g_bus_own_name for this, but this leads to cases where pages
     * are created and documents are loaded before we get a name and are able
     * to call to the UI process. */
    ext.connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);

    service_name = g_strdup_printf("%s-%s", VB_WEBEXTENSION_SERVICE_NAME, extid);

    /* Try to own name synchronously. */
    if (ext.connection && dbus_own_name_sync(ext.connection, service_name, G_BUS_NAME_OWNER_FLAGS_NONE)) {
        on_dbus_name_acquire(ext.connection, service_name, extension);
    }

    g_free(service_name);

    g_signal_connect(extension, "page-created", G_CALLBACK(on_page_created), NULL);
}

/**
 * Add observers to doc event for given document and all the contained iframes
 * too.
 */
static void add_onload_event_observers(WebKitDOMDocument *doc)
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
            G_CALLBACK(on_editable_change_focus), TRUE, NULL);
    webkit_dom_event_target_add_event_listener(target, "blur",
            G_CALLBACK(on_editable_change_focus), TRUE, NULL);
    /* Check for focused editable elements also if they where focused before
     * the event observer where set up. */
    /* TODO this is not needed for strict-focus=on */
    on_editable_change_focus(target, NULL);
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

    /* Don't do anythings if the dbus connection was not established. */
    if (!ext.connection) {
        return;
    }

    /* propagate the signal over dbus */
    g_dbus_connection_emit_signal(ext.connection, NULL,
            VB_WEBEXTENSION_OBJECT_PATH, VB_WEBEXTENSION_INTERFACE, name,
            data, &error);

    /* check for error */
    if (error) {
        g_warning("Failed to emit signal '%s': %s", name, error->message);
        g_error_free(error);
    }
}

/**
 * Handle dbus method calls.
 */
static void dbus_handle_method_call(GDBusConnection *conn, const char *sender,
        const char *object_path, const char *interface_name, const char *method,
        GVariant *parameters, GDBusMethodInvocation *invocation, gpointer data)
{
    char *value;

    if (!g_strcmp0(method, "FocusInput")) {
        ext_dom_focus_input(webkit_web_page_get_dom_document(ext.webpage));
        g_dbus_method_invocation_return_value(invocation, NULL);
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
 * The synchronous and blocking pendent to the g_bus_own_name().
 *
 * @bus_type: The type of bus to own a name on.
 * @name:     The well-known name to own.
 * @flags:    A set of flags from the #GBusNameOwnerFlags enumeration.
 */
static gboolean dbus_own_name_sync(GDBusConnection *connection, const char *name,
        GBusNameOwnerFlags flags)
{
    GError *error = NULL;
    guint32 request_name_reply = 0;
    GVariant *result;

    result = g_dbus_connection_call_sync(
            connection,
            "org.freedesktop.DBus",
            "/org/freedesktop/DBus",
            "org.freedesktop.DBus",
            "RequestName",
            g_variant_new("(su)", name, flags),
            G_VARIANT_TYPE("(u)"),
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            NULL,
            &error);

    if (result) {
        g_variant_get(result, "(u)", &request_name_reply);
        g_variant_unref(result);

        if (1 == request_name_reply) {
            return TRUE;
        }
    } else {
        g_warning("Failed to acquire DBus name: %s", error->message);
        g_error_free(error);
    }
    return FALSE;
}

/**
 * Called when the dbus name is aquired and registers our object.
 */
static void on_dbus_name_acquire(GDBusConnection *connection, const char *name, gpointer data)
{
    GError *error = NULL;
    static GDBusNodeInfo *node_info = NULL;

    g_return_if_fail(G_IS_DBUS_CONNECTION(connection));

    if (!node_info) {
        node_info = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
    }

    /* register the webextension object */
    ext.connection = connection;
    ext.regid      = g_dbus_connection_register_object(
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
    }
}

/**
 * Callback called if a editable element changes it focus state.
 * Event target may be a WebKitDOMDocument (in case of iframe) or a
 * WebKitDOMDOMWindow.
 */
static void on_editable_change_focus(WebKitDOMEventTarget *target, WebKitDOMEvent *event)
{
    gboolean input_focus;
    WebKitDOMDocument *doc;
    WebKitDOMElement *active;

    if (WEBKIT_DOM_IS_DOM_WINDOW(target)) {
        g_object_get(target, "document", &doc, NULL);
    } else {
        /* target is a doc document */
        doc = WEBKIT_DOM_DOCUMENT(target);
    }
    active = webkit_dom_document_get_active_element(doc);
    /* Don't do anything if there is no active element or the active element
     * is the same as before. */
    if (!active || active == ext.active) {
        return;
    }
    if (WEBKIT_DOM_IS_HTML_IFRAME_ELEMENT(active)) {
        WebKitDOMHTMLIFrameElement *iframe;
        WebKitDOMDocument *subdoc;

        iframe = WEBKIT_DOM_HTML_IFRAME_ELEMENT(active);
        subdoc = webkit_dom_html_iframe_element_get_content_document(iframe);
        add_onload_event_observers(subdoc);
        return;
    }

    ext.active = active;

    /* Check if the active element is an editable element. */
    input_focus = ext_dom_is_editable(active);
    if (input_focus != ext.input_focus) {
        ext.input_focus = input_focus;

        dbus_emit_signal("EditableChangeFocus", g_variant_new("(b)", input_focus));
    }
}

/**
 * Callback for web extensions page-created signal.
 */
static void on_page_created(WebKitWebExtension *extension, WebKitWebPage *page, gpointer data)
{
    /* Save the new created page in the extension data for later use. */
    ext.webpage = page;

    g_object_connect(page,
            "signal::send-request", G_CALLBACK(on_web_page_send_request), NULL,
            "signal::document-loaded", G_CALLBACK(on_web_page_document_loaded), NULL,
            NULL);

    dbus_emit_signal("PageCreated", g_variant_new("(t)", webkit_web_page_get_id(page)));
}

/**
 * Callback for web pages document-loaded signal.
 */
static void on_web_page_document_loaded(WebKitWebPage *page, gpointer data)
{
    /* If there is a hashtable of known document - detroy this and create a
     * new hashtable. */
    if (ext.documents) {
        g_hash_table_unref(ext.documents);
    }
    ext.documents = g_hash_table_new(g_direct_hash, g_direct_equal);

    add_onload_event_observers(webkit_web_page_get_dom_document(page));
}

/**
 * Callback for web pages send-request signal.
 */
static gboolean on_web_page_send_request(WebKitWebPage *page, WebKitURIRequest *request,
        WebKitURIResponse *response, gpointer data)
{
    /* Change request headers according to the users preferences. */
    if (ext.headers) {
        char *name, *value;
        SoupMessageHeaders *headers;
        GHashTableIter iter;

        headers = webkit_uri_request_get_http_headers(request);
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
    }

    return FALSE;
}
