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

#include <gio/gio.h>
#include <glib.h>

#include "ext-proxy.h"
#include "main.h"
#include "webextension/ext-main.h"

static gboolean on_authorize_authenticated_peer(GDBusAuthObserver *observer,
        GIOStream *stream, GCredentials *credentials, gpointer data);
static gboolean on_new_connection(GDBusServer *server,
        GDBusConnection *connection, gpointer data);
static void on_connection_close(GDBusConnection *connection, gboolean
        remote_peer_vanished, GError *error, gpointer data);
static void on_proxy_created (GDBusProxy *proxy, GAsyncResult *result,
        gpointer data);
static void on_vertical_scroll(GDBusConnection *connection,
        const char *sender_name, const char *object_path,
        const char *interface_name, const char *signal_name,
        GVariant *parameters, gpointer data);
static void dbus_call(Client *c, const char *method, GVariant *param,
        GAsyncReadyCallback callback);
static GVariant *dbus_call_sync(Client *c, const char *method, GVariant
        *param);
static void on_web_extension_page_created(GDBusConnection *connection,
        const char *sender_name, const char *object_path,
        const char *interface_name, const char *signal_name,
        GVariant *parameters, gpointer data);

/* TODO we need potentially multiple proxies. Because a single instance of
 * vimb may hold multiple clients which may use more than one webprocess and
 * therefore multiple webextension instances. */
extern struct Vimb vb;
static GDBusServer *dbusserver;


/**
 * Initialize the dbus proxy by watching for appearing dbus name.
 */
const char *ext_proxy_init(void)
{
    char *address, *guid;
    GDBusAuthObserver *observer;
    GError *error = NULL;

    address  = g_strdup_printf("unix:tmpdir=%s", g_get_tmp_dir());
    guid     = g_dbus_generate_guid();
    observer = g_dbus_auth_observer_new();

    g_signal_connect(observer, "authorize-authenticated-peer",
            G_CALLBACK(on_authorize_authenticated_peer), NULL);

    /* Use sync call because server must be starte before the web extension
     * attempt to connect */
    dbusserver = g_dbus_server_new_sync(address, G_DBUS_SERVER_FLAGS_NONE,
            guid, observer, NULL, &error);

    if (error) {
        g_warning("Failed to start web extension server on %s: %s", address, error->message);
        g_error_free(error);
        goto out;
    }

    g_signal_connect(dbusserver, "new-connection", G_CALLBACK(on_new_connection), NULL);
    g_dbus_server_start(dbusserver);

out:
    g_free(address);
    g_free(guid);
    g_object_unref(observer);

    return g_dbus_server_get_client_address(dbusserver);
}

/* TODO move this to a lib or somthing that can be used from ui and web
 * process together */
static gboolean on_authorize_authenticated_peer(GDBusAuthObserver *observer,
        GIOStream *stream, GCredentials *credentials, gpointer data)
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
        g_warning ("No credentials received from web extension.\n");
    }

    return authorized;
}

static gboolean on_new_connection(GDBusServer *server,
        GDBusConnection *connection, gpointer data)
{
    /* Create dbus proxy. */
    g_return_val_if_fail(G_IS_DBUS_CONNECTION(connection), FALSE);

    g_signal_connect(connection, "closed", G_CALLBACK(on_connection_close), NULL);

    g_dbus_proxy_new(connection,
            G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES|G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS,
            NULL,
            NULL,
            VB_WEBEXTENSION_OBJECT_PATH,
            VB_WEBEXTENSION_INTERFACE,
            NULL,
            (GAsyncReadyCallback)on_proxy_created,
            NULL);

    return TRUE;
}

static void on_connection_close(GDBusConnection *connection, gboolean
        remote_peer_vanished, GError *error, gpointer data)
{
    if (error && !remote_peer_vanished) {
        g_warning("Unexpected lost connection to web extension: %s", error->message);
    }
}

static void on_proxy_created(GDBusProxy *new_proxy, GAsyncResult *result,
        gpointer data)
{
    GError *error = NULL;
    GDBusProxy *proxy;
    GDBusConnection *connection;

    proxy = g_dbus_proxy_new_finish(result, &error);
    if (!proxy) {
        if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
            g_warning("Error creating web extension proxy: %s", error->message);
        }
        g_error_free(error);

        /* TODO cancel the dbus connection - use cancelable */
        return;
    }

    connection = g_dbus_proxy_get_connection(proxy);
    g_dbus_connection_signal_subscribe(connection, NULL,
            VB_WEBEXTENSION_INTERFACE, "PageCreated",
            VB_WEBEXTENSION_OBJECT_PATH, NULL, G_DBUS_SIGNAL_FLAGS_NONE,
            (GDBusSignalCallback)on_web_extension_page_created, proxy,
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
    glong max, top;
    guint percent;
    guint64 pageid;
    Client *c;

    g_variant_get(parameters, "(ttqt)", &pageid, &max, &percent, &top);
    c = vb_get_client_for_page_id(pageid);
    if (c) {
        c->state.scroll_max     = max;
        c->state.scroll_percent = percent;
        c->state.scroll_top     = top;
    }

    vb_statusbar_update(c);
}

void ext_proxy_eval_script(Client *c, char *js, GAsyncReadyCallback callback)
{
    if (callback) {
        dbus_call(c, "EvalJs", g_variant_new("(ts)", c->page_id, js), callback);
    } else {
        dbus_call(c, "EvalJsNoResult", g_variant_new("(ts)", c->page_id, js), NULL);
    }
}

GVariant *ext_proxy_eval_script_sync(Client *c, char *js)
{
    return dbus_call_sync(c, "EvalJs", g_variant_new("(ts)", c->page_id, js));
}

/**
 * Request the web extension to focus first editable element.
 * Returns whether an focusable element was found or not.
 */
void ext_proxy_focus_input(Client *c)
{
    dbus_call(c, "FocusInput", g_variant_new("(t)", c->page_id), NULL);
}

/**
 * Send the headers string to the webextension.
 */
void ext_proxy_set_header(Client *c, const char *headers)
{
    dbus_call(c, "SetHeaderSetting", g_variant_new("(s)", headers), NULL);
}

void ext_proxy_lock_input(Client *c, const char *element_id)
{
    dbus_call(c, "LockInput", g_variant_new("(ts)", c->page_id, element_id), NULL);
}

void ext_proxy_unlock_input(Client *c, const char *element_id)
{
    dbus_call(c, "UnlockInput", g_variant_new("(ts)", c->page_id, element_id), NULL);
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
 * Call a dbus method syncron.
 */
static GVariant *dbus_call_sync(Client *c, const char *method, GVariant *param)
{
    GVariant *result = NULL;
    GError *error = NULL;

    if (!c->dbusproxy) {
        return NULL;
    }

    result = g_dbus_proxy_call_sync(c->dbusproxy, method, param,
        G_DBUS_CALL_FLAGS_NONE, 500, NULL, &error);

    if (error) {
        g_warning("Failed dbus method %s: %s", method, error->message);
        g_error_free(error);
    }

    return result;
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
    Client *c;
    guint64 pageid;

    g_variant_get(parameters, "(t)", &pageid);

    /* Search for the client with the same page id as returned by the
     * webextension. */
    c = vb_get_client_for_page_id(pageid);
    if (c) {
        /* Set the dbus proxy on the right client based on page id. */
        c->dbusproxy = (GDBusProxy*)data;

        /* Subscribe to dbus signals here. */
        g_dbus_connection_signal_subscribe(connection, NULL,
                VB_WEBEXTENSION_INTERFACE, "VerticalScroll",
                VB_WEBEXTENSION_OBJECT_PATH, NULL, G_DBUS_SIGNAL_FLAGS_NONE,
                (GDBusSignalCallback)on_vertical_scroll, NULL, NULL);
    }
}
