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

static void on_page_created(WebKitWebProcessExtension *ext, WebKitWebPage *webpage, gpointer data);
static void on_web_page_document_loaded(WebKitWebPage *webpage, gpointer extension);
static gboolean on_web_page_user_message_received(WebKitWebPage *web_page,
        WebKitUserMessage *message, gpointer user_data);
static gboolean on_web_page_send_request(WebKitWebPage *webpage, WebKitURIRequest *request,
        WebKitURIResponse *response, gpointer extension);
static void on_window_object_cleared(WebKitScriptWorld *world, WebKitWebPage *page,
        WebKitFrame *frame, gpointer user_data);

/* Global struct to hold internal used variables. */
struct Ext {
    GHashTable          *headers;
    GHashTable          *page_frames;  /* Maps page_id -> main WebKitFrame */
};
struct Ext ext = {0};


/**
 * Webextension entry point.
 * WebKitGTK 6.0: Function name changed from webkit_web_extension_initialize_with_user_data
 * to webkit_web_process_extension_initialize_with_user_data
 */
G_MODULE_EXPORT
void webkit_web_process_extension_initialize_with_user_data(WebKitWebProcessExtension *extension, G_GNUC_UNUSED GVariant *data)
{
    WebKitScriptWorld *world;

    /* Initialize the page frames hash table */
    ext.page_frames = g_hash_table_new(g_direct_hash, g_direct_equal);

    /* Connect to page-created signal to handle new pages */
    g_signal_connect(extension, "page-created", G_CALLBACK(on_page_created), NULL);

    /* Connect to script world's window-object-cleared signal to get frame references
     * This is the non-deprecated way to access frames in WebKitGTK 6.0+ */
    world = webkit_script_world_get_default();
    g_signal_connect(world, "window-object-cleared", G_CALLBACK(on_window_object_cleared), NULL);
}

/**
 * Callback for script world's window-object-cleared signal.
 * This is called when the JavaScript window object is cleared for a frame,
 * typically when a new document is loaded.
 */
static void on_window_object_cleared(WebKitScriptWorld *world, WebKitWebPage *page,
        WebKitFrame *frame, gpointer user_data)
{
    /* Only store the main frame */
    if (webkit_frame_is_main_frame(frame)) {
        guint64 page_id = webkit_web_page_get_id(page);
        /* Store the frame reference - the frame is owned by WebKit, we just keep a pointer */
        g_hash_table_insert(ext.page_frames, GUINT_TO_POINTER(page_id), frame);
    }
}

/**
 * Get the main frame for a web page using our cached reference.
 */
static WebKitFrame *get_main_frame(WebKitWebPage *web_page)
{
    guint64 page_id = webkit_web_page_get_id(web_page);
    return g_hash_table_lookup(ext.page_frames, GUINT_TO_POINTER(page_id));
}

/**
 * Callback for web extensions page-created signal.
 */
static void on_page_created(WebKitWebProcessExtension *extension, WebKitWebPage *webpage, gpointer data)
{
    if (!webpage) {
        g_warning("on_page_created called with NULL webpage!");
        return;
    }

    if (!extension) {
        g_warning("on_page_created called with NULL extension!");
        return;
    }

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

    /* Send message with page ID as parameter.
     * This allows the main process to match this page with the correct dbus proxy. */
    message = webkit_user_message_new("page-document-loaded",
                                      g_variant_new("(t)", pageid));
    webkit_web_page_send_message_to_view(webpage, message, NULL, NULL, NULL);
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

    /* EvalJs - Evaluate JavaScript and return result */
    if (g_strcmp0(name, "EvalJs") == 0) {
        const char *js_code;
        WebKitFrame *frame;
        JSCContext *js_context;
        JSCValue *result = NULL;
        gboolean success = FALSE;

        g_variant_get(parameters, "(&s)", &js_code);

        frame = get_main_frame(web_page);
        if (frame) {
            js_context = webkit_frame_get_js_context(frame);

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

        frame = get_main_frame(web_page);
        if (frame) {
            js_context = webkit_frame_get_js_context(frame);

            if (js_context) {
                ext_util_js_eval(js_context, js_code, NULL);
            }
        }
        return TRUE;
    }

    /* FocusInput - Focus an input element */
    if (g_strcmp0(name, "FocusInput") == 0) {
        WebKitFrame *frame = get_main_frame(web_page);
        JSCContext *js_context;

        if (frame) {
            js_context = webkit_frame_get_js_context(frame);

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

        frame = get_main_frame(web_page);
        if (frame) {
            js_context = webkit_frame_get_js_context(frame);

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

        frame = get_main_frame(web_page);
        if (frame) {
            js_context = webkit_frame_get_js_context(frame);

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
