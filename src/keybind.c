/**
 * vimp - a webkit based vim like browser.
 *
 * Copyright (C) 2012 Daniel Carl
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

#include "main.h"
#include "keybind.h"
#include "command.h"
#include "completion.h"

static GSList* keys = NULL;
static GString* modkeys = NULL;

static GSList* keybind_find(int mode, guint modkey, guint modmask, guint keyval);
static void keybind_str_to_keybind(gchar* str, Keybind* key);
static guint keybind_str_to_modmask(const gchar* str);
static guint keybind_str_to_value(const gchar* str);
static gboolean keybind_keypress_callback(WebKitWebView* webview, GdkEventKey* event);


void keybind_init(void)
{
    modkeys = g_string_new("");
    g_signal_connect(G_OBJECT(vp.gui.window), "key-press-event", G_CALLBACK(keybind_keypress_callback), NULL);
}

void keybind_cleanup(void)
{
    if (keys) {
        g_slist_free(keys);
    }
    if (modkeys) {
        g_string_free(modkeys, TRUE);
    }
}

gboolean keybind_add_from_string(const gchar* str, const Mode mode)
{
    if (str == NULL || *str == '\0') {
        return FALSE;
    }
    gboolean result;
    gchar* line = g_strdup(str);
    g_strstrip(line);

    /* split into keybinding and command */
    char **string = g_strsplit(line, " ", 2);

    guint len = g_strv_length(string);
    if (len == 2 && command_exists(string[1])) {
        Keybind* keybind = g_new0(Keybind, 1);
        keybind->mode    = mode;
        keybind->command = g_strdup(string[1]);

        keybind_str_to_keybind(string[0], keybind);

        /* add the keybinding to the list */
        keys = g_slist_prepend(keys, keybind);

        /* save the modkey also in the modkey string */
        if (keybind->modkey) {
            g_string_append_c(modkeys, keybind->modkey);
        }
        result = TRUE;
    } else {
        result = FALSE;
    }

    g_strfreev(string);
    g_free(line);

    return result;
}

gboolean keybind_remove_from_string(const gchar* str, const Mode mode)
{
    gchar* line = NULL;
    Keybind keybind = {0};

    if (str == NULL || *str == '\0') {
        return FALSE;
    }
    line = g_strdup(str);
    g_strstrip(line);

    /* fill the keybind with data from given string */
    keybind_str_to_keybind(line, &keybind);

    GSList* link = keybind_find(keybind.mode, keybind.modkey, keybind.modmask, keybind.keyval);
    if (link) {
        keys = g_slist_delete_link(keys, link);
    }
    /* TODO remove eventually no more used modkeys */
    return TRUE;
}

static GSList* keybind_find(int mode, guint modkey, guint modmask, guint keyval)
{
    GSList* link;
    for (link = keys; link != NULL; link = link->next) {
        Keybind* keybind = (Keybind*)link->data;

        if (keybind->keyval == keyval
                && keybind->modmask == modmask
                && keybind->modkey == modkey
                && keybind->mode == mode) {
            return link;
        }
    }

    return NULL;
}

/**
 * Configures the given keybind by also given string.
 */
static void keybind_str_to_keybind(gchar* str, Keybind* keybind)
{
    gchar** string = NULL;
    guint len = 0;

    if (str == NULL || *str == '\0') {
        return;
    }
    g_strstrip(str);

    /* [modkey]keyval
     * - modkey is a single char
     * - keval can be a single char, <tab> , <ctrl-tab>, <shift-a> */
    keybind->modkey = 0;
    if (strlen(str) == 1) {
        /* only a single simple key set like "q" */
        keybind->keyval = str[0];
    } else if (strlen(str) == 2) {
        /* modkey with simple key given like "gf" */
        keybind->modkey = str[0];
        keybind->keyval = str[1];
    } else {
        /* special handling for shift tab */
        /* TODO find a better solution for such cases */
        if (g_ascii_strcasecmp(str, "<shift-tab>") == 0) {
            keybind->keyval = GDK_ISO_Left_Tab;
            return;
        }
        /* keybind has keys like "<tab>" or <ctrl-a> */
        if (str[0] == '<') {
            /* no modkey set */
            string = g_strsplit_set(str, "<->", 4);
        } else {
            keybind->modkey = str[0];
            string = g_strsplit_set(str + 1, "<->", 4);
        }
        len = g_strv_length(string);
        if (len == 4) {
            /* key with modmask like <ctrl-tab> */
            keybind->modmask = keybind_str_to_modmask(string[1]);
            keybind->keyval = keybind_str_to_value(string[2]);
        } else if (len == 3) {
            /* key without modmask like <tab> */
            keybind->keyval = keybind_str_to_value(string[1]);
        }
        g_strfreev(string);
    }
}

static guint keybind_str_to_modmask(const gchar* str)
{
    if (g_ascii_strcasecmp(str, "ctrl") == 0) {
        return GDK_CONTROL_MASK;
    }
    if (g_ascii_strcasecmp(str, "shift") == 0) {
        return GDK_SHIFT_MASK;
    }

    return 0;
}

static guint keybind_str_to_value(const gchar* str)
{
    if (g_ascii_strcasecmp(str, "tab") == 0) {
        return GDK_Tab;
    }

    return str[0];
}

static gboolean keybind_keypress_callback(WebKitWebView* webview, GdkEventKey* event)
{
    GdkModifierType irrelevant;
    guint keyval;
    static GdkKeymap *keymap;

    keymap = gdk_keymap_get_default();

    /* Get a mask of modifiers that shouldn't be considered for this event.
     * E.g.: It shouldn't matter whether ';' is shifted or not. */
    gdk_keymap_translate_keyboard_state(keymap, event->hardware_keycode,
            event->state, event->group, &keyval, NULL, NULL, &irrelevant);

    /* check for escape or modkeys or counts */
    if ((CLEAN(event->state) & ~irrelevant) == 0) {
        if (IS_ESCAPE(event)) {
            completion_clean();
            /* switch to normal mode and clear the input box */
            Arg a = {VP_MODE_NORMAL, ""};
            vp_set_mode(&a);

            return TRUE;
        }
        /* allow mode keys and counts only in normal mode */
        if (VP_MODE_NORMAL == vp.state.mode) {
            if (vp.state.modkey == 0 && ((event->keyval >= GDK_1 && event->keyval <= GDK_9)
                    || (event->keyval == GDK_0 && vp.state.count))) {
                /* append the new entered count to previous one */
                vp.state.count = (vp.state.count ? vp.state.count * 10 : 0) + (event->keyval - GDK_0);
                vp_update_statusbar();

                return TRUE;
            }
            if (strchr(modkeys->str, event->keyval) && vp.state.modkey != event->keyval) {
                vp.state.modkey = (gchar)event->keyval;
                vp_update_statusbar();

                return TRUE;
            }
        }
    }

#if 0
    /* TODO should we use a command for that too? */
    if (CLEAN_MODE(vp.state.mode) == VP_MODE_COMMAND
        && (event->keyval == GDK_Tab || event->keyval == GDK_ISO_Left_Tab)
    ) {
        completion_complete(event->keyval == GDK_ISO_Left_Tab);
        return TRUE;
    }
#endif

    /* check for keybinding */
    GSList* link = keybind_find(CLEAN_MODE(vp.state.mode), vp.state.modkey,
            (CLEAN(event->state) & ~irrelevant), keyval);

    if (link) {
        Keybind* keybind = (Keybind*)link->data;
        command_run(keybind->command, NULL);

        return TRUE;
    }

    return FALSE;
}
