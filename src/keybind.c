/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2013 Daniel Carl
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

extern VbCore vb;

static GSList  *keys;
static GString *modkeys;

static void rebuild_modkeys(void);
static GSList *find(int mode, guint modkey, guint modmask, guint keyval);
static void string_to_keybind(char *str, Keybind *key);
static guint string_to_modmask(const char *str);
static guint string_to_value(const char *str);
static gboolean keypress_cb(WebKitWebView *webview, GdkEventKey *event);
static void free_keybind(Keybind *keybind);


void keybind_init(void)
{
    modkeys = g_string_new("");
    g_signal_connect(G_OBJECT(vb.gui.webview), "key-press-event", G_CALLBACK(keypress_cb), NULL);
    g_signal_connect(G_OBJECT(vb.gui.box), "key-press-event", G_CALLBACK(keypress_cb), NULL);
}

void keybind_cleanup(void)
{
    if (keys) {
        g_slist_free_full(keys, (GDestroyNotify)free_keybind);
    }
    if (modkeys) {
        g_string_free(modkeys, true);
    }
}

gboolean keybind_add_from_string(char *keystring, const char *command, const Mode mode)
{
    char **token = NULL;
    if (keystring == NULL || *keystring == '\0') {
        return false;
    }

    /* split the input string into command and parameter part */
    token = g_strsplit(command, " ", 2);
    if (!token[0] || !command_exists(token[0])) {
        g_strfreev(token);
        return false;
    }

    Keybind *keybind = g_new0(Keybind, 1);
    keybind->mode    = mode;
    keybind->command = g_strdup(token[0]);
    keybind->param   = g_strdup(token[1]);
    g_strfreev(token);

    string_to_keybind(keystring, keybind);

    /* add the keybinding to the list */
    keys = g_slist_prepend(keys, keybind);

    /* save the modkey also in the modkey string if not exists already */
    if (keybind->modkey && strchr(modkeys->str, keybind->modkey) == NULL) {
        g_string_append_c(modkeys, keybind->modkey);
    }

    return true;
}

gboolean keybind_remove_from_string(char *str, const Mode mode)
{
    Keybind keybind = {.mode = mode};

    if (str == NULL || *str == '\0') {
        return false;
    }
    g_strstrip(str);

    /* fill the keybind with data from given string */
    string_to_keybind(str, &keybind);

    GSList *link = find(keybind.mode, keybind.modkey, keybind.modmask, keybind.keyval);
    if (link) {
        free_keybind((Keybind*)link->data);
        keys = g_slist_delete_link(keys, link);
    }

    if (keybind.modkey && strchr(modkeys->str, keybind.modkey) != NULL) {
        /* remove eventually no more used modkeys */
        rebuild_modkeys();
    }
    return true;
}

/**
 * Discard all knows modkeys and walk through all keybindings to aggregate
 * them new.
 */
static void rebuild_modkeys(void)
{
    GSList *link;
    /* remove previous modkeys */
    if (modkeys) {
        g_string_free(modkeys, true);
        modkeys = g_string_new("");
    }

    /* regenerate the modekeys */
    for (link = keys; link != NULL; link = link->next) {
        Keybind *keybind = (Keybind*)link->data;
        /* if not not exists - add it */
        if (keybind->modkey && strchr(modkeys->str, keybind->modkey) == NULL) {
            g_string_append_c(modkeys, keybind->modkey);
        }
    }
}

static GSList *find(int mode, guint modkey, guint modmask, guint keyval)
{
    GSList *link;
    for (link = keys; link != NULL; link = link->next) {
        Keybind *keybind = (Keybind*)link->data;
        if (keybind->keyval == keyval
            && keybind->modmask == modmask
            && keybind->modkey == modkey
            && keybind->mode == mode
        ) {
            return link;
        }
    }

    return NULL;
}

/**
 * Configures the given keybind by also given string.
 */
static void string_to_keybind(char *str, Keybind *keybind)
{
    char **string = NULL;
    guint len = 0;

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
            keybind->modmask = string_to_modmask(string[1]);
            keybind->keyval = string_to_value(string[2]);
        } else if (len == 3) {
            /* key without modmask like <tab> */
            keybind->keyval = string_to_value(string[1]);
        }
        g_strfreev(string);
    }

    /* set the shift mask for uppercase keys like 'G' */
    guint32 ukval = gdk_keyval_to_unicode(keybind->keyval);
    if (g_unichar_isgraph(ukval)
        /* ignore SHIFT if key is not subject to case */
        && (gdk_keyval_is_upper(keybind->keyval) && !gdk_keyval_is_lower(keybind->keyval))
    ) {
        keybind->modmask = GDK_SHIFT_MASK;
    }

    /* post process the keybinding */
    /* special handling for shift tab */
    if (keybind->keyval == GDK_Tab && keybind->modmask == GDK_SHIFT_MASK) {
        keybind->keyval = GDK_ISO_Left_Tab;
    }
}

static guint string_to_modmask(const char *str)
{
    if (g_ascii_strcasecmp(str, "ctrl") == 0) {
        return GDK_CONTROL_MASK;
    }
    if (g_ascii_strcasecmp(str, "shift") == 0) {
        return GDK_SHIFT_MASK;
    }

    return 0;
}

static guint string_to_value(const char *str)
{
    if (!strcmp(str, "tab")) {
        return GDK_Tab;
    } else if (!strcmp(str, "up")) {
        return GDK_Up;
    } else if (!strcmp(str, "down")) {
        return GDK_Down;
    } else if (!strcmp(str, "left")) {
        return GDK_Left;
    } else if (!strcmp(str, "right")) {
        return GDK_Right;
    }

    return str[0];
}

static gboolean keypress_cb(WebKitWebView *webview, GdkEventKey *event)
{
    guint keyval = event->keyval;
    guint state  = CLEAN_STATE_WITH_SHIFT(event);

    /* check for escape or modkeys or counts */
    if (IS_ESCAPE_KEY(keyval, state)) {
        /* switch to normal mode and clear the input box */
        vb_set_mode(VB_MODE_NORMAL, true);

        return true;
    }
    /* allow mode keys and counts only in normal mode */
    if ((VB_MODE_SEARCH | VB_MODE_NORMAL) & vb.state.mode) {
        if (vb.state.modkey == 0 && ((keyval >= GDK_1 && keyval <= GDK_9)
                || (keyval == GDK_0 && vb.state.count))) {
            /* append the new entered count to previous one */
            vb.state.count = (vb.state.count ? vb.state.count * 10 : 0) + (keyval - GDK_0);
            vb_update_statusbar();

            return true;
        }
        if (strchr(modkeys->str, keyval) && vb.state.modkey != keyval) {
            vb.state.modkey = (char)keyval;
            vb_update_statusbar();

            return true;
        }
    }

    /* check for keybinding */
    GSList *link = find(CLEAN_MODE(vb.state.mode), vb.state.modkey, state, keyval);

    if (link) {
        Keybind *keybind = (Keybind*)link->data;
        command_run(keybind->command, keybind->param);

        return true;
    }

    return false;
}

static void free_keybind(Keybind *keybind)
{
    g_free(keybind->command);
    g_free(keybind->param);
    g_free(keybind);
}
