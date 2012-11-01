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

#include "setting.h"
#include "util.h"

static gboolean setting_webkit(const Setting* s);

static Setting default_settings[] = {
    {"auto-load-images", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"auto-shrink-images", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"cursive-font-family", TYPE_CHAR, setting_webkit, {.s = "serif"}},
    {"cursive-font-family", TYPE_CHAR, setting_webkit, {.s = "serif"}},
    {"default-encoding", TYPE_CHAR, setting_webkit, {.s = "utf-8"}},
    {"default-font-family", TYPE_CHAR, setting_webkit, {.s = "sans-serif"}},
    {"default-font-size", TYPE_INTEGER, setting_webkit, {.i = 12}},
    {"default-monospace-font-size", TYPE_INTEGER, setting_webkit, {.i = 10}},
    {"enable-caret-browsing", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"enable-default-context-menu", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"enable-developer-extras", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"enable-dns-prefetching", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"enable-dom-paste", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"enable-frame-flattening", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"enable-file-access-from-file-uris", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"enable-html5-database", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"enable-html5-local-storage", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"enable-java-applet", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"enable-offline-web-application-cache", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"enable-page-cache", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"enable-plugins", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"enable-private-browsing", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"enable-scripts", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"enable-site-specific-quirks", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"enable-spatial-navigation", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"enable-spell-checking", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"enable-universal-access-from-file-uris", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"enable-xss-auditor", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"enforce-96-dpi", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"fantasy-font-family", TYPE_CHAR, setting_webkit, {.s = "serif"}},
    {"javascript-can-access-clipboard", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"javascript-can-open-windows-automatically", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"minimum-font-size", TYPE_INTEGER, setting_webkit, {.i = 5}},
    {"minimum-logical-font-size", TYPE_INTEGER, setting_webkit, {.i = 5}},
    {"monospace-font-family", TYPE_CHAR, setting_webkit, {.s = "monospace"}},
    {"print-backgrounds", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"resizable-text-areas", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"sans-serif-font-family", TYPE_CHAR, setting_webkit, {.s = "sans-serif"}},
    {"serif-font-family", TYPE_CHAR, setting_webkit, {.s = "serif"}},
    {"spell-checking-languages", TYPE_CHAR, setting_webkit, {.s = NULL}},
    {"tab-key-cycles-through-elements", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"user-agent", TYPE_CHAR, setting_webkit, {.s = PROJECT "/" VERSION " (X11; Linux i686) AppleWebKit/535.22+ Compatible (Safari)"}},
    {"user-stylesheet-uri", TYPE_CHAR, setting_webkit, {.s = NULL}},
    {"zoom-step", TYPE_DOUBLE, setting_webkit, {.i = 100}},
};

static GHashTable* settings = NULL;


void setting_init(void)
{
    Setting* s;
    guint i;
    settings = g_hash_table_new(g_str_hash, g_str_equal);

    for (i = 0; i < LENGTH(default_settings); i++) {
        s = &default_settings[i];
        g_hash_table_insert(settings, (gpointer)s->name, s);

        /* set the default settings */
        s->func(s);
    }
}

gboolean setting_run(const gchar* name, const gchar* param)
{
    gboolean result = FALSE;
    Setting* s      = g_hash_table_lookup(settings, name);
    if (!s) {
        vp_echo(VP_MSG_ERROR, "Config '%s' not found", name);
        return FALSE;
    }

    if (!param) {
        return s->func(s);
    }

    /* if string param is given convert it to the required data type and set
     * it to the arg of the setting */
    switch (s->type) {
        case TYPE_BOOLEAN:
            s->arg.i = util_atob(param) ? 1 : 0;
            result = s->func(s);
            break;

        case TYPE_INTEGER:
            s->arg.i = g_ascii_strtoull(param, (gchar**)NULL, 10);
            result = s->func(s);
            break;

        case TYPE_DOUBLE:
            s->arg.i = (1000 * g_ascii_strtod(param, (gchar**)NULL));
            result = s->func(s);
            break;

        case TYPE_CHAR:
        case TYPE_COLOR:
            s->arg.s = g_strdup(param);
            result = s->func(s);
            g_free(s->arg.s);
            break;
    }

    return result;
}

static gboolean setting_webkit(const Setting* s)
{
    WebKitWebSettings* web_setting = webkit_web_view_get_settings(vp.gui.webview);

    switch (s->type) {
        case TYPE_BOOLEAN:
            g_object_set(G_OBJECT(web_setting), s->name, s->arg.i ? TRUE : FALSE, NULL);
            break;

        case TYPE_INTEGER:
            g_object_set(G_OBJECT(web_setting), s->name, s->arg.i, NULL);
            break;

        case TYPE_DOUBLE:
            g_object_set(G_OBJECT(web_setting), s->name, (s->arg.i / 1000.0), NULL);
            break;

        case TYPE_CHAR:
        case TYPE_COLOR:
            g_object_set(G_OBJECT(web_setting), s->name, s->arg.s, NULL);
            break;
    }
    return TRUE;
}
