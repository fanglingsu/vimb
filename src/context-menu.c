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

#include "glib-object.h"
#include "glib.h"
#include "main.h"
#include "context-menu.h"

/* Structure to hold context menu action data */
typedef struct {
    char *uri;
    Client *client;
} ContextMenuData;

static void fix_open_in_new_window_stock_action(WebKitContextMenu *menu,
        WebKitContextMenuItem *menu_item, int menu_item_position, char *msgid,
        char *gaction_name, const char *uri, Client *c);
static void open_in_new_tab_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void context_menu_data_free(gpointer data, GClosure *closure);

/**
 * Callback for the webview context-menu signal.
 * WebKitGTK 6.0: GdkEvent parameter was removed from the signal.
 */
gboolean on_webview_context_menu(WebKitWebView *webview,
        WebKitContextMenu *menu,
        WebKitHitTestResult *hit_test_result, Client *c)
{
    GList* menu_item;
    GList* next;
    int position;
    WebKitContextMenuAction menu_action;

    menu_item = webkit_context_menu_get_items(menu);
    for (position = 0; menu_item != NULL; position++) {
        menu_action = webkit_context_menu_item_get_stock_action(menu_item->data);
        next = menu_item->next;

        switch (menu_action) {
            case WEBKIT_CONTEXT_MENU_ACTION_OPEN_LINK_IN_NEW_WINDOW:
                fix_open_in_new_window_stock_action(menu, menu_item->data,
                        position, "Open Link in New _Tab", "open-link-in-new-tab",
                        webkit_hit_test_result_get_link_uri(hit_test_result), c);
                break;
            case WEBKIT_CONTEXT_MENU_ACTION_OPEN_IMAGE_IN_NEW_WINDOW:
                fix_open_in_new_window_stock_action(menu, menu_item->data,
                        position, "Open _Image in New Tab", "open-image-in-new-tab",
                        webkit_hit_test_result_get_image_uri(hit_test_result), c);
                break;
            case WEBKIT_CONTEXT_MENU_ACTION_OPEN_AUDIO_IN_NEW_WINDOW:
                fix_open_in_new_window_stock_action(menu, menu_item->data,
                        position, "Open _Audio in New Tab", "open-audio-in-new-tab",
                        webkit_hit_test_result_get_media_uri(hit_test_result), c);
                break;
            case WEBKIT_CONTEXT_MENU_ACTION_OPEN_VIDEO_IN_NEW_WINDOW:
                fix_open_in_new_window_stock_action(menu, menu_item->data,
                        position, "Open _Video in New Tab", "open-video-in-new-tab",
                        webkit_hit_test_result_get_media_uri(hit_test_result), c);
                break;
            case WEBKIT_CONTEXT_MENU_ACTION_OPEN_FRAME_IN_NEW_WINDOW:
                // TODO figure out how to fix this action instead of removing
                webkit_context_menu_remove(menu, menu_item->data);
                position--;
                break;
            default:
                break;
        }

        menu_item = next;
    }

    return FALSE;
}

/**
 * Free the context menu data when the action is destroyed.
 */
static void context_menu_data_free(gpointer data, GClosure *closure)
{
    ContextMenuData *cmd = (ContextMenuData *)data;
    if (cmd) {
        g_free(cmd->uri);
        g_free(cmd);
    }
}

/**
 * Callback when context menu action is activated.
 */
static void open_in_new_tab_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    ContextMenuData *cmd = (ContextMenuData *)user_data;

    if (cmd && cmd->uri && cmd->client) {
        vb_load_uri(cmd->client, &(Arg){TARGET_TAB, cmd->uri});
    }
}

/**
 * When you click on "Open <something> in New Window" context menu buttons
 * WebKit creates a new window on its own, and then Vimb creates one more
 * window. In some cases, it leads to segmentation fault after closing windows.
 * To fix this issue, we can replace default context menu buttons with custom
 * buttons that open in a new tab instead.
 */
static void fix_open_in_new_window_stock_action(WebKitContextMenu *menu,
        WebKitContextMenuItem *menu_item, int menu_item_position, char *msgid,
        char *gaction_name, const char *uri, Client *c)
{
    WebKitContextMenuItem *new_item;
    GSimpleAction *action;
    ContextMenuData *data;

    /* Allocate data structure to pass to callback */
    data = g_new(ContextMenuData, 1);
    data->uri = g_strdup(uri);
    data->client = c;

    action = g_simple_action_new(gaction_name, NULL);
    /* Connect with destroy notify to free the data when action is destroyed */
    g_signal_connect_data(G_OBJECT(action), "activate",
            G_CALLBACK(open_in_new_tab_activated), data,
            context_menu_data_free, 0);

    webkit_context_menu_remove(menu, menu_item);
    new_item = webkit_context_menu_item_new_from_gaction(G_ACTION(action),
            msgid, NULL);
    webkit_context_menu_insert(menu, new_item, menu_item_position);

    g_object_unref(action);
}
