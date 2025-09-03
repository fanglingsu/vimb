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

static void fix_open_in_new_window_stock_action(WebKitContextMenu *menu,
        WebKitContextMenuItem *menu_item, int menu_item_position, char *msgid,
        char *gaction_name, GVariant *data);
static void open_in_new_window(GVariant *data);

/**
 * Callback for the webview context-menu signal.
 */
gboolean on_webview_context_menu(WebKitWebView *webview,
        WebKitContextMenu *menu, GdkEvent *event,
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
                        position, "Open Link in New _Window", "open-link-in-new-window",
                        g_variant_new("(st)", webkit_hit_test_result_get_link_uri(hit_test_result), c));
                break;
            case WEBKIT_CONTEXT_MENU_ACTION_OPEN_IMAGE_IN_NEW_WINDOW:
                fix_open_in_new_window_stock_action(menu, menu_item->data,
                        position, "Open _Image in New Window", "open-image-in-new-window",
                        g_variant_new("(st)", webkit_hit_test_result_get_image_uri(hit_test_result), c));
                break;
            case WEBKIT_CONTEXT_MENU_ACTION_OPEN_AUDIO_IN_NEW_WINDOW:
                fix_open_in_new_window_stock_action(menu, menu_item->data,
                        position, "Open _Audio in New Window", "open-audio-in-new-window",
                        g_variant_new("(st)", webkit_hit_test_result_get_media_uri(hit_test_result), c));
                break;
            case WEBKIT_CONTEXT_MENU_ACTION_OPEN_VIDEO_IN_NEW_WINDOW:
                fix_open_in_new_window_stock_action(menu, menu_item->data,
                        position, "Open _Video in New Window", "open-video-in-new-window",
                        g_variant_new("(st)", webkit_hit_test_result_get_media_uri(hit_test_result), c));
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
 * When you click on "Open <something> in New Window" context menu buttons
 * WebKit creates a new window on its own, and then Vimb creates one more
 * window. In some cases, it leads to segmentation fault after closing windows.
 * To fix this issue, we can replace default context menu buttons with custom
 * buttons.
 */
static void fix_open_in_new_window_stock_action(WebKitContextMenu *menu,
        WebKitContextMenuItem *menu_item, int menu_item_position, char *msgid,
        char *gaction_name, GVariant *data)
{
    WebKitContextMenuItem *new_item;
    GSimpleAction *action;

    action = g_simple_action_new(gaction_name, NULL);
    g_signal_connect_swapped(G_OBJECT(action), "activate", G_CALLBACK(open_in_new_window), data);

    webkit_context_menu_remove(menu, menu_item);
    new_item = webkit_context_menu_item_new_from_gaction(G_ACTION(action),
            g_dgettext("WebKitGTK-4.1", msgid), NULL);
    webkit_context_menu_insert(menu, new_item, menu_item_position);

    g_object_unref(action);
}

static void open_in_new_window(GVariant *data)
{
    Client *c;
    char* uri;

    g_variant_get(data, "(st)", &uri, &c);

    vb_load_uri(c, &(Arg){TARGET_NEW, uri});
}
