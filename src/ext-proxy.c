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

/* WebKitGTK 6.0: D-Bus infrastructure completely removed.
 * All IPC now uses WebKitUserMessage API. */

extern struct Vimb vb;

/**
 * Initialize web extension communication.
 * WebKitGTK 6.0: D-Bus has been replaced with WebKitUserMessage.
 * This function is kept for compatibility but returns NULL.
 */
const char *ext_proxy_init(void)
{
    /* WebKitGTK 6.0: All IPC now uses WebKitUserMessage API.
     * D-Bus infrastructure has been completely removed. */
    return NULL;
}

/* WebKitGTK 6.0: All D-Bus infrastructure removed - using WebKitUserMessage */

/**
 * Evaluate JavaScript in the webextension using WebKitUserMessage.
 * WebKitGTK 6.0: Replaces D-Bus EvalJs method.
 */
void ext_proxy_eval_script(Client *c, char *js, GAsyncReadyCallback callback)
{
    WebKitUserMessage *message;

    if (callback) {
        /* With callback - wait for response */
        message = webkit_user_message_new("EvalJs", g_variant_new("(s)", js));
        webkit_web_view_send_message_to_page(c->webview, message, NULL, callback, c);
    } else {
        /* No callback - fire and forget */
        message = webkit_user_message_new("EvalJsNoResult", g_variant_new("(s)", js));
        webkit_web_view_send_message_to_page(c->webview, message, NULL, NULL, NULL);
    }
}

/* Data structure for synchronous evaluation */
typedef struct {
    GVariant *result;
    gboolean done;
} SyncEvalData;

/* Callback for synchronous JavaScript evaluation */
static void on_sync_eval_finished(GObject *source_object, GAsyncResult *result, gpointer user_data)
{
    SyncEvalData *data = (SyncEvalData *)user_data;
    WebKitWebView *webview = WEBKIT_WEB_VIEW(source_object);
    GError *error = NULL;
    WebKitUserMessage *reply;

    reply = webkit_web_view_send_message_to_page_finish(webview, result, &error);
    if (error) {
        g_warning("Sync eval script failed: %s", error->message);
        g_error_free(error);
        data->result = NULL;
    } else if (reply) {
        /* Copy the parameters since the reply will be freed */
        GVariant *params = webkit_user_message_get_parameters(reply);
        if (params) {
            data->result = g_variant_ref(params);
        }
        g_object_unref(reply);
    }

    data->done = TRUE;
}

/**
 * Evaluate JavaScript synchronously using WebKitUserMessage.
 * WebKitGTK 6.0: Replaces D-Bus EvalJs method.
 * Uses GLib main loop iteration to wait for the async result.
 */
GVariant *ext_proxy_eval_script_sync(Client *c, char *js)
{
    WebKitUserMessage *message;
    SyncEvalData data = { NULL, FALSE };
    GMainContext *context;

    g_return_val_if_fail(c != NULL && c->webview != NULL, NULL);

    message = webkit_user_message_new("EvalJs", g_variant_new("(s)", js));
    webkit_web_view_send_message_to_page(c->webview, message, NULL,
        on_sync_eval_finished, &data);

    /* Iterate the main loop until we get a response */
    context = g_main_context_default();
    while (!data.done) {
        g_main_context_iteration(context, TRUE);
    }

    return data.result;
}

/**
 * Evaluate JavaScript directly in the page's JavaScript context.
 * This runs JS in the same world as user scripts injected via
 * webkit_user_content_manager_add_script(), allowing access to
 * variables defined there (like vimbDomOps).
 *
 * WebKitGTK 6.0: Uses webkit_web_view_evaluate_javascript().
 */
void ext_proxy_eval_script_in_page(Client *c, const char *js)
{
    g_return_if_fail(c != NULL && c->webview != NULL);

    webkit_web_view_evaluate_javascript(c->webview, js, -1, NULL, NULL, NULL, NULL, NULL);
}

/**
 * Focus an input element using WebKitUserMessage.
 * WebKitGTK 6.0: Replaces D-Bus FocusInput method.
 */
void ext_proxy_focus_input(Client *c)
{
    WebKitUserMessage *message;

    g_warning("Main process: Sending FocusInput message");

    message = webkit_user_message_new("FocusInput", NULL);
    webkit_web_view_send_message_to_page(c->webview, message, NULL, NULL, NULL);
}

/**
 * Set headers using WebKitUserMessage.
 * WebKitGTK 6.0: Replaces D-Bus SetHeaderSetting method.
 */
void ext_proxy_set_header(Client *c, const char *headers)
{
    /* Headers are now set via WebKitWebContext, not per-page.
     * This function is deprecated but kept for compatibility. */
}

/**
 * Lock input element using WebKitUserMessage.
 * WebKitGTK 6.0: Replaces D-Bus LockInput method.
 */
void ext_proxy_lock_input(Client *c, const char *element_id)
{
    WebKitUserMessage *message;

    g_warning("Main process: Sending LockInput message for element: %s", element_id);

    message = webkit_user_message_new("LockInput", g_variant_new("(s)", element_id));
    webkit_web_view_send_message_to_page(c->webview, message, NULL, NULL, NULL);
}

/**
 * Unlock input element using WebKitUserMessage.
 * WebKitGTK 6.0: Replaces D-Bus UnlockInput method.
 */
void ext_proxy_unlock_input(Client *c, const char *element_id)
{
    WebKitUserMessage *message;

    g_warning("Main process: Sending UnlockInput message for element: %s", element_id);

    message = webkit_user_message_new("UnlockInput", g_variant_new("(s)", element_id));
    webkit_web_view_send_message_to_page(c->webview, message, NULL, NULL, NULL);
}

/**
 * Returns the current selection if there is one as newly allocates string.
 *
 * Result must be freed by caller with g_free.
 */
char *ext_proxy_get_current_selection(Client *c)
{
    char *selection, *js;
    gboolean success;
    GVariant *jsreturn;

    js       = g_strdup_printf("getSelection().toString();");
    jsreturn = ext_proxy_eval_script_sync(c, js);
    g_free(js);

    if (jsreturn == NULL) {
        g_warning("cannot get current selection: failed to evaluate js");
        return NULL;
    }

    g_variant_get(jsreturn, "(bs)", &success, &selection);
    g_variant_unref(jsreturn);

    if (!success) {
        g_warning("can not get current selection: %s", selection);
        g_free(selection);
        return NULL;
    }

    return selection;
}

/* WebKitGTK 6.0: All D-Bus functions removed - using WebKitUserMessage */
