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
static gboolean setting_hint_style(const Setting* s);

static Setting default_settings[] = {
    /* webkit settings */
    {"images", "auto-load-images", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"shrinkimages", "auto-shrink-images", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"cursivfont", "cursive-font-family", TYPE_CHAR, setting_webkit, {.s = "serif"}},
    {"defaultencondig", "default-encoding", TYPE_CHAR, setting_webkit, {.s = "utf-8"}},
    {"defaultfont", "default-font-family", TYPE_CHAR, setting_webkit, {.s = "sans-serif"}},
    {"fontsize", "default-font-size", TYPE_INTEGER, setting_webkit, {.i = 10}},
    {"monofontsize", "default-monospace-font-size", TYPE_INTEGER, setting_webkit, {.i = 10}},
    {"carret", "enable-caret-browsing", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {NULL, "enable-default-context-menu", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"webinspector", "enable-developer-extras", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"dnsprefetching", "enable-dns-prefetching", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"dompaste", "enable-dom-paste", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"frameflattening", "enable-frame-flattening", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {NULL, "enable-file-access-from-file-uris", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {NULL, "enable-html5-database", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {NULL, "enable-html5-local-storage", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"javaapplet", "enable-java-applet", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"offlinecache", "enable-offline-web-application-cache", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"pagecache", "enable-page-cache", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"plugins", "enable-plugins", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"privatebrowsing", "enable-private-browsing", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"scripts", "enable-scripts", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {NULL, "enable-site-specific-quirks", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {NULL, "enable-spatial-navigation", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"spell", "enable-spell-checking", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {NULL, "enable-universal-access-from-file-uris", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"xssauditor", "enable-xss-auditor", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {NULL, "enforce-96-dpi", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"fantasyfont", "fantasy-font-family", TYPE_CHAR, setting_webkit, {.s = "serif"}},
    {NULL, "javascript-can-access-clipboard", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {NULL, "javascript-can-open-windows-automatically", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"minimumfontsize", "minimum-font-size", TYPE_INTEGER, setting_webkit, {.i = 5}},
    {NULL, "minimum-logical-font-size", TYPE_INTEGER, setting_webkit, {.i = 5}},
    {"monofont", "monospace-font-family", TYPE_CHAR, setting_webkit, {.s = "monospace"}},
    {"backgrounds", "print-backgrounds", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"resizetextareas", "resizable-text-areas", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"sansfont", "sans-serif-font-family", TYPE_CHAR, setting_webkit, {.s = "sans-serif"}},
    {"seriffont", "serif-font-family", TYPE_CHAR, setting_webkit, {.s = "serif"}},
    {"spelllang", "spell-checking-languages", TYPE_CHAR, setting_webkit, {.s = NULL}},
    {NULL, "tab-key-cycles-through-elements", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"useragent", "user-agent", TYPE_CHAR, setting_webkit, {.s = PROJECT "/" VERSION " (X11; Linux i686) AppleWebKit/535.22+ Compatible (Safari)"}},
    {"stylesheet", "user-stylesheet-uri", TYPE_CHAR, setting_webkit, {.s = NULL}},
    {"zoomstep", "zoom-step", TYPE_DOUBLE, setting_webkit, {.i = 100}},
    /* internal variables */
#ifdef FEATURE_COOKIE
    {NULL, "cookie-timeout", TYPE_INTEGER, setting_cookie_timeout, {.i = 4800}},
#endif
    {NULL, "scrollstep", TYPE_INTEGER, setting_scrollstep, {.i = 40}},
    {NULL, "status-color-bg", TYPE_CHAR, setting_status_color_bg, {.s = "#000"}},
    {NULL, "status-color-fg", TYPE_CHAR, setting_status_color_fg, {.s = "#fff"}},
    {NULL, "status-font", TYPE_CHAR, setting_status_font, {.s = "monospace bold 8"}},
    {NULL, "input-bg-normal", TYPE_CHAR, setting_input_style, {.s = "#fff"}},
    {NULL, "input-bg-error", TYPE_CHAR, setting_input_style, {.s = "#f00"}},
    {NULL, "input-fg-normal", TYPE_CHAR, setting_input_style, {.s = "#000"}},
    {NULL, "input-fg-error", TYPE_CHAR, setting_input_style, {.s = "#000"}},
    {NULL, "input-font-normal", TYPE_CHAR, setting_input_style, {.s = "monospace normal 8"}},
    {NULL, "input-font-error", TYPE_CHAR, setting_input_style, {.s = "monospace bold 8"}},
    {NULL, "completion-font-normal", TYPE_CHAR, setting_completion_style, {.s = "monospace normal 8"}},
    {NULL, "completion-font-active", TYPE_CHAR, setting_completion_style, {.s = "monospace bold 8"}},
    {NULL, "completion-fg-normal", TYPE_CHAR, setting_completion_style, {.s = "#f6f3e8"}},
    {NULL, "completion-fg-active", TYPE_CHAR, setting_completion_style, {.s = "#fff"}},
    {NULL, "completion-bg-normal", TYPE_CHAR, setting_completion_style, {.s = "#656565"}},
    {NULL, "completion-bg-active", TYPE_CHAR, setting_completion_style, {.s = "#777777"}},
    {NULL, "hint-bg", TYPE_CHAR, setting_hint_style, {.s = "#ff0"}},
    {NULL, "hint-bg-focus", TYPE_CHAR, setting_hint_style, {.s = "#8f0"}},
    {NULL, "hint-fg", TYPE_CHAR, setting_hint_style, {.s = "#000"}},
};


void setting_init(void)
{
    Setting* s;
    guint i;
    vp.settings = g_hash_table_new(g_str_hash, g_str_equal);

    for (i = 0; i < LENGTH(default_settings); i++) {
        s = &default_settings[i];
        /* use alias as key if available */
        g_hash_table_insert(vp.settings, (gpointer)s->alias != NULL ? s->alias : s->name, s);

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
    VpColor color;

    VP_COLOR_PARSE(&color, s->arg.s);
    VP_WIDGET_OVERRIDE_BACKGROUND(vp.gui.eventbox, GTK_STATE_NORMAL, &color);
    VP_WIDGET_OVERRIDE_BACKGROUND(GTK_WIDGET(vp.gui.statusbar.left), GTK_STATE_NORMAL, &color);
    VP_WIDGET_OVERRIDE_BACKGROUND(GTK_WIDGET(vp.gui.statusbar.right), GTK_STATE_NORMAL, &color);

    return TRUE;
}

static gboolean setting_status_color_fg(const Setting* s)
{
    VpColor color;

    VP_COLOR_PARSE(&color, s->arg.s);
    VP_WIDGET_OVERRIDE_COLOR(vp.gui.eventbox, GTK_STATE_NORMAL, &color);
    VP_WIDGET_OVERRIDE_COLOR(GTK_WIDGET(vp.gui.statusbar.left), GTK_STATE_NORMAL, &color);
    VP_WIDGET_OVERRIDE_COLOR(GTK_WIDGET(vp.gui.statusbar.right), GTK_STATE_NORMAL, &color);

    return TRUE;
}

static gboolean setting_status_font(const Setting* s)
{
    PangoFontDescription* font;
    font = pango_font_description_from_string(s->arg.s);

    VP_WIDGET_OVERRIDE_FONT(vp.gui.eventbox, font);
    VP_WIDGET_OVERRIDE_FONT(GTK_WIDGET(vp.gui.statusbar.left), font);
    VP_WIDGET_OVERRIDE_FONT(GTK_WIDGET(vp.gui.statusbar.right), font);

    pango_font_description_free(font);

    return TRUE;
}

static gboolean setting_input_style(const Setting* s)
{
    Style* style = &vp.style;
    MessageType type = g_str_has_suffix(s->name, "normal") ? VP_MSG_NORMAL : VP_MSG_ERROR;

    if (g_str_has_prefix(s->name, "input-bg")) {
        VP_COLOR_PARSE(&style->input_bg[type], s->arg.s);
    } else if (g_str_has_prefix(s->name, "input-fg")) {
        VP_COLOR_PARSE(&style->input_fg[type], s->arg.s);
    } else if (g_str_has_prefix(s->name, "input-font")) {
        if (style->input_font[type]) {
            pango_font_description_free(style->input_font[type]);
        }
        style->input_font[type] = pango_font_description_from_string(s->arg.s);
    }
    /* echo already visible input text to apply the new style to input box */
    vp_echo(VP_MSG_NORMAL, FALSE, GET_TEXT());

    return TRUE;
}

static gboolean setting_completion_style(const Setting* s)
{
    Style* style = &vp.style;
    CompletionStyle type = g_str_has_suffix(s->name, "normal") ? VP_COMP_NORMAL : VP_COMP_ACTIVE;

    if (g_str_has_prefix(s->name, "completion-bg")) {
        VP_COLOR_PARSE(&style->comp_bg[type], s->arg.s);
    } else if (g_str_has_prefix(s->name, "completion-fg")) {
        VP_COLOR_PARSE(&style->comp_fg[type], s->arg.s);
    } else if (g_str_has_prefix(s->name, "completion-font")) {
        if (style->comp_font[type]) {
            pango_font_description_free(style->comp_font[type]);
        }
        style->comp_font[type] = pango_font_description_from_string(s->arg.s);
    }

    return TRUE;
}

static gboolean setting_hint_style(const Setting* s)
{
    Style* style = &vp.style;
    if (!g_strcmp0(s->name, "hint-bg")) {
        strncpy(style->hint_bg, s->arg.s, HEX_COLOR_LEN - 1);
        style->hint_bg[HEX_COLOR_LEN - 1] = '\0';
    } else if (!g_strcmp0(s->name, "hint-bg-focus")) {
        strncpy(style->hint_bg_focus, s->arg.s, HEX_COLOR_LEN - 1);
        style->hint_bg_focus[HEX_COLOR_LEN] = '\0';
    } else if (!g_strcmp0(s->name, "hint-fg")) {
        strncpy(style->hint_fg, s->arg.s, HEX_COLOR_LEN - 1);
        style->hint_fg[HEX_COLOR_LEN - 1] = '\0';
    }

    return TRUE;
}
