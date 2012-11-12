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
#ifdef FEATURE_COOKIE
static gboolean setting_cookie_timeout(const Setting* s);
#endif
static gboolean setting_scrollstep(const Setting* s);
static gboolean setting_status_color_bg(const Setting* s);
static gboolean setting_status_color_fg(const Setting* s);
static gboolean setting_status_font(const Setting* s);
static gboolean setting_input_style(const Setting* s);
static gboolean setting_completion_style(const Setting* s);

static Setting default_settings[] = {
    /* webkit settings */
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
    /* internal variables */
#ifdef FEATURE_COOKIE
    {"cookie-timeout", TYPE_INTEGER, setting_cookie_timeout, {.i = 4800}},
#endif
    {"scrollstep", TYPE_INTEGER, setting_scrollstep, {.i = 40}},
    {"status-color-bg", TYPE_CHAR, setting_status_color_bg, {.s = "#000"}},
    {"status-color-fg", TYPE_CHAR, setting_status_color_fg, {.s = "#fff"}},
    {"status-font", TYPE_CHAR, setting_status_font, {.s = "monospace bold 8"}},
    {"input-bg-normal", TYPE_CHAR, setting_input_style, {.s = "#fff"}},
    {"input-bg-error", TYPE_CHAR, setting_input_style, {.s = "#f00"}},
    {"input-fg-normal", TYPE_CHAR, setting_input_style, {.s = "#000"}},
    {"input-fg-error", TYPE_CHAR, setting_input_style, {.s = "#000"}},
    {"input-font-normal", TYPE_CHAR, setting_input_style, {.s = "monospace normal 8"}},
    {"input-font-error", TYPE_CHAR, setting_input_style, {.s = "monospace bold 8"}},
    {"completion-font-normal", TYPE_CHAR, setting_completion_style, {.s = "monospace normal 8"}},
    {"completion-font-active", TYPE_CHAR, setting_completion_style, {.s = "monospace bold 10"}},
    {"completion-fg-normal", TYPE_CHAR, setting_completion_style, {.s = "#f6f3e8"}},
    {"completion-fg-active", TYPE_CHAR, setting_completion_style, {.s = "#0f0"}},
    {"completion-bg-normal", TYPE_CHAR, setting_completion_style, {.s = "#656565"}},
    {"completion-bg-active", TYPE_CHAR, setting_completion_style, {.s = "#777777"}},
};


void setting_init(void)
{
    Setting* s;
    guint i;
    vp.settings = g_hash_table_new(g_str_hash, g_str_equal);

    for (i = 0; i < LENGTH(default_settings); i++) {
        s = &default_settings[i];
        g_hash_table_insert(vp.settings, (gpointer)s->name, s);

        /* set the default settings */
        s->func(s);
    }
}

void setting_cleanup(void)
{
    if (vp.settings) {
        g_hash_table_destroy(vp.settings);
    }
}

gboolean setting_run(const gchar* name, const gchar* param)
{
    Arg* a = NULL;
    gboolean result = FALSE;
    Setting* s      = g_hash_table_lookup(vp.settings, name);
    if (!s) {
        vp_echo(VP_MSG_ERROR, TRUE, "Config '%s' not found", name);
        return FALSE;
    }

    if (!param) {
        return s->func(s);
    }

    /* if string param is given convert it to the required data type and set
     * it to the arg of the setting */
    a = util_char_to_arg(param, s->type);
    if (a == NULL) {
        vp_echo(VP_MSG_ERROR, TRUE, "No valid value");
        return FALSE;
    }

    s->arg = *a;
    result = s->func(s);
    if (a->s) {
        g_free(a->s);
    }
    g_free(a);

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

#ifdef FEATURE_COOKIE
static gboolean setting_cookie_timeout(const Setting* s)
{
    vp.config.cookie_timeout = s->arg.i;

    return TRUE;
}
#endif

static gboolean setting_scrollstep(const Setting* s)
{
    vp.config.scrollstep = s->arg.i;

    return TRUE;
}

static gboolean setting_status_color_bg(const Setting* s)
{
    GdkColor color;

    gdk_color_parse(s->arg.s, &color);
    gtk_widget_modify_bg(vp.gui.eventbox, GTK_STATE_NORMAL, &color);
    gtk_widget_modify_bg(GTK_WIDGET(vp.gui.statusbar.left), GTK_STATE_NORMAL, &color);
    gtk_widget_modify_bg(GTK_WIDGET(vp.gui.statusbar.right), GTK_STATE_NORMAL, &color);

    return TRUE;
}

static gboolean setting_status_color_fg(const Setting* s)
{
    GdkColor color;

    gdk_color_parse(s->arg.s, &color);
    gtk_widget_modify_fg(vp.gui.eventbox, GTK_STATE_NORMAL, &color);
    gtk_widget_modify_fg(GTK_WIDGET(vp.gui.statusbar.left), GTK_STATE_NORMAL, &color);
    gtk_widget_modify_fg(GTK_WIDGET(vp.gui.statusbar.right), GTK_STATE_NORMAL, &color);

    return TRUE;
}

static gboolean setting_status_font(const Setting* s)
{
    PangoFontDescription* font;
    font = pango_font_description_from_string(s->arg.s);

    gtk_widget_modify_font(vp.gui.eventbox, font);
    gtk_widget_modify_font(GTK_WIDGET(vp.gui.statusbar.left), font);
    gtk_widget_modify_font(GTK_WIDGET(vp.gui.statusbar.right), font);

    pango_font_description_free(font);

    return TRUE;
}

static gboolean setting_input_style(const Setting* s)
{
    Style* style = &vp.style;
    MessageType type = g_str_has_suffix(s->name, "normal") ? VP_MSG_NORMAL : VP_MSG_ERROR;

    if (g_str_has_prefix(s->name, "input-bg")) {
        gdk_color_parse(s->arg.s, &style->input_bg[type]);
    } else if (g_str_has_prefix(s->name, "input-fg")) {
        gdk_color_parse(s->name, &style->input_fg[type]);
    } else if (g_str_has_prefix(s->arg.s, "input-font")) {
        if (style->input_font[type]) {
            pango_font_description_free(style->input_font[type]);
        }
        style->input_font[type] = pango_font_description_from_string(s->arg.s);
    }

    return TRUE;
}

static gboolean setting_completion_style(const Setting* s)
{
    Style* style = &vp.style;
    CompletionStyle type = g_str_has_suffix(s->name, "normal") ? VP_COMP_NORMAL : VP_COMP_ACTIVE;

    if (g_str_has_prefix(s->name, "completion-bg")) {
        gdk_color_parse(s->arg.s, &style->comp_bg[type]);
    } else if (g_str_has_prefix(s->name, "completion-fg")) {
        gdk_color_parse(s->name, &style->comp_fg[type]);
    } else if (g_str_has_prefix(s->arg.s, "completion-font")) {
        if (style->comp_font[type]) {
            pango_font_description_free(style->comp_font[type]);
        }
        style->comp_font[type] = pango_font_description_from_string(s->arg.s);
    }

    return TRUE;
}
