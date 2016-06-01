/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2016 Daniel Carl
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

#include <gio/gio.h>
#include <glib.h>

#include "ext-proxy.h"
#include "main.h"
#include "webextension/ext-main.h"

static void dbus_call(Client *c, const char *method, GVariant *param,
        GAsyncReadyCallback callback);
static void on_editable_change_focus(GDBusConnection *connection,
        const char *sender_name, const char *object_path,
        const char *interface_name, const char *signal_name,
        GVariant *parameters, gpointer data);
static void on_name_appeared(GDBusConnection *connection, const char *name,
        const char *owner, gpointer data);
static void on_proxy_created(GDBusProxy *new_proxy, GAsyncResult *result,
        gpointer data);
static void on_web_extension_page_created(GDBusConnection *connection,
        const char *sender_name, const char *object_path,
        const char *interface_name, const char *signal_name,
        GVariant *parameters, gpointer data);

/* TODO we need potentially multiple proxies. Because a single instance of
 * vimb may hold multiple clients which may use more than one webprocess and
 * therefore multiple webextension instances. */
extern struct Vimb vb;


/**
 * Request the web extension to focus first editable element.
 * Returns whether an focusable element was found or not.
 */
void ext_proxy_focus_input(Client *c)
{
    dbus_call(c, "FocusInput", NULL, NULL);
}

/**
 * Initialize the dbus proxy by watching for appearing dbus name.
 */
void ext_proxy_init(const char *id)
{
    char *service_name;

    service_name = g_strdup_printf("%s-%s", VB_WEBEXTENSION_SERVICE_NAME, id);
    g_bus_watch_name(
            G_BUS_TYPE_SESSION,
            service_name,
            G_BUS_NAME_WATCHER_FLAGS_NONE,
            (GBusNameAppearedCallback)on_name_appeared,
            NULL,
            NULL,
            NULL);
    g_free(service_name);
}

/**
 * Send the headers string to the webextension.
 */
void ext_proxy_set_header(Client *c, const char *headers)
{
    dbus_call(c, "SetHeaderSetting", g_variant_new("(s)", headers), NULL);
}

/**
 * Call a dbus method.
 */
static void dbus_call(Client *c, const char *method, GVariant *param,
        GAsyncReadyCallback callback)
{
    /* TODO add function to queue calls until the proxy connection is
     * established */
    if (!c->dbusproxy) {
        return;
    }
    g_dbus_proxy_call(c->dbusproxy, method, param, G_DBUS_CALL_FLAGS_NONE, -1, NULL, callback, c);
}

/**
 * Callback called if a editable element changes it's focus state.
 */
static void on_editable_change_focus(GDBusConnection *connection,
        const char *sender_name, const char *object_path,
        const char *interface_name, const char *signal_name,
        GVariant *parameters, gpointer data)
{
    gboolean is_focused;
    Client *c = (Client*)data;
    g_variant_get(parameters, "(b)", &is_focused);

    /* Don't change the mode if we are in pass through mode. */
    if (c->mode->id == 'n' && is_focused) {
        vb_enter(c, 'i');
    } else if (c->mode->id == 'i' && !is_focused) {
        vb_enter(c, 'n');
    }
    /* TODO allo strict-focus to ignore focus event for initial set focus */
}

/**
 * Called when the name of the webextension appeared on the dbus session bus.
 */
static void on_name_appeared(GDBusConnection *connection, const char *name,
        const char *owner, gpointer data)
{
    int flags = G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START
        | G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES
        | G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS;

    /* Create the proxy to communicate over dbus. */
    g_dbus_proxy_new(connection, flags, NULL, name,
            VB_WEBEXTENSION_OBJECT_PATH, VB_WEBEXTENSION_INTERFACE, NULL,
            (GAsyncReadyCallback)on_proxy_created, NULL);
}

/**
 * Callback called when the dbus proxy is created.
 */
static void on_proxy_created(GDBusProxy *new_proxy, GAsyncResult *result, gpointer data)
{
    GDBusConnection *connection;
    GDBusProxy *proxy;
    GError *error = NULL;

    proxy      = g_dbus_proxy_new_finish(result, &error);
    connection = g_dbus_proxy_get_connection(proxy);

    if (!proxy) {
        g_warning("Error creating web extension proxy: %s", error->message);
        g_error_free(error);
        return;
    }
    g_dbus_proxy_set_default_timeout(proxy, 100);
    g_dbus_connection_signal_subscribe(
            connection,
            NULL,
            VB_WEBEXTENSION_INTERFACE,
            "PageCreated",
            VB_WEBEXTENSION_OBJECT_PATH,
            NULL,
            G_DBUS_SIGNAL_FLAGS_NONE,
            (GDBusSignalCallback)on_web_extension_page_created,
            proxy,
            NULL);
}

/**
 * Listen to the VerticalScroll signal of the webextension and set the scroll
 * percent value on the client to update the statusbar.
 */
static void on_vertical_scroll(GDBusConnection *connection,
        const char *sender_name, const char *object_path,
        const char *interface_name, const char *signal_name,
        GVariant *parameters, gpointer data)
{
    Client *c = (Client*)data;
    g_variant_get(parameters, "(tt)", &c->state.scroll_max, &c->state.scroll_percent);

    vb_statusbar_update(c);
}

/**
 * Called when the web context created the page.
 *
 * Find the right client to the page id returned from the webextension.
 * Add the proxy connection to the client for later calls.
 */
static void on_web_extension_page_created(GDBusConnection *connection,
        const char *sender_name, const char *object_path,
        const char *interface_name, const char *signal_name,
        GVariant *parameters, gpointer data)
{
    Client *p;
    guint64 page_id;

    g_variant_get(parameters, "(t)", &page_id);

    /* Search for the client with the same page id as returned by the
     * webextension. */
    for (p = vb.clients; p && p->page_id != page_id; p = p->next);

    if (p) {
        /* Set the dbus proxy on the right client based on page id. */
        p->dbusproxy = data;

        g_dbus_connection_signal_subscribe(connection, NULL,
                VB_WEBEXTENSION_INTERFACE, "VerticalScroll",
                VB_WEBEXTENSION_OBJECT_PATH, NULL, G_DBUS_SIGNAL_FLAGS_NONE,
                (GDBusSignalCallback)on_vertical_scroll, p, NULL);
        g_dbus_connection_signal_subscribe(connection, NULL,
                VB_WEBEXTENSION_INTERFACE, "EditableChangeFocus",
                VB_WEBEXTENSION_OBJECT_PATH, NULL, G_DBUS_SIGNAL_FLAGS_NONE,
                (GDBusSignalCallback)on_editable_change_focus, p, NULL);
    }
}
