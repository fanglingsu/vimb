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

static Arg* setting_char_to_arg(const gchar* str, const Type type);
static void setting_print_value(const Setting* s, void* value);
static gboolean setting_webkit(const Setting* s, const gboolean get);
static gboolean setting_cookie_timeout(const Setting* s, const gboolean get);
static gboolean setting_scrollstep(const Setting* s, const gboolean get);
static gboolean setting_status_color_bg(const Setting* s, const gboolean get);
static gboolean setting_status_color_fg(const Setting* s, const gboolean get);
static gboolean setting_status_font(const Setting* s, const gboolean get);
static gboolean setting_input_style(const Setting* s, const gboolean get);
static gboolean setting_completion_style(const Setting* s, const gboolean get);
static gboolean setting_hint_style(const Setting* s, const gboolean get);

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
    {"useragent", "user-agent", TYPE_CHAR, setting_webkit, {.s = "vimp/" VERSION " (X11; Linux i686) AppleWebKit/535.22+ Compatible (Safari)"}},
    {"stylesheet", "user-stylesheet-uri", TYPE_CHAR, setting_webkit, {.s = NULL}},
    {"zoomstep", "zoom-step", TYPE_FLOAT, setting_webkit, {.i = 100000}},
    /* internal variables */
    {NULL, "cookie-timeout", TYPE_INTEGER, setting_cookie_timeout, {.i = 4800}},
    {NULL, "scrollstep", TYPE_INTEGER, setting_scrollstep, {.i = 40}},
    {NULL, "status-color-bg", TYPE_CHAR, setting_status_color_bg, {.s = "#000"}},
    {NULL, "status-color-fg", TYPE_CHAR, setting_status_color_fg, {.s = "#fff"}},
    {NULL, "status-font", TYPE_FONT, setting_status_font, {.s = "monospace bold 8"}},
    {NULL, "input-bg-normal", TYPE_COLOR, setting_input_style, {.s = "#fff"}},
    {NULL, "input-bg-error", TYPE_COLOR, setting_input_style, {.s = "#f00"}},
    {NULL, "input-fg-normal", TYPE_COLOR, setting_input_style, {.s = "#000"}},
    {NULL, "input-fg-error", TYPE_COLOR, setting_input_style, {.s = "#000"}},
    {NULL, "input-font-normal", TYPE_FONT, setting_input_style, {.s = "monospace normal 8"}},
    {NULL, "input-font-error", TYPE_FONT, setting_input_style, {.s = "monospace bold 8"}},
    {NULL, "completion-font-normal", TYPE_FONT, setting_completion_style, {.s = "monospace normal 8"}},
    {NULL, "completion-font-active", TYPE_FONT, setting_completion_style, {.s = "monospace bold 8"}},
    {NULL, "completion-fg-normal", TYPE_COLOR, setting_completion_style, {.s = "#f6f3e8"}},
    {NULL, "completion-fg-active", TYPE_COLOR, setting_completion_style, {.s = "#fff"}},
    {NULL, "completion-bg-normal", TYPE_COLOR, setting_completion_style, {.s = "#656565"}},
    {NULL, "completion-bg-active", TYPE_COLOR, setting_completion_style, {.s = "#777777"}},
    {NULL, "max-completion-items", TYPE_INTEGER, setting_completion_style, {.i = 15}},
    {NULL, "hint-bg", TYPE_CHAR, setting_hint_style, {.s = "#ff0"}},
    {NULL, "hint-bg-focus", TYPE_CHAR, setting_hint_style, {.s = "#8f0"}},
    {NULL, "hint-fg", TYPE_CHAR, setting_hint_style, {.s = "#000"}},
    {NULL, "hint-style", TYPE_CHAR, setting_hint_style, {.s = "font-family:monospace;font-weight:bold;color:#000;background-color:#fff;margin:0;padding:0px 1px;border:1px solid #444;opacity:0.7;"}},
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
        s->func(s, FALSE);
    }
}

void setting_cleanup(void)
{
    if (vp.settings) {
        g_hash_table_destroy(vp.settings);
    }
}

gboolean setting_run(gchar* name, const gchar* param)
{
    Arg* a          = NULL;
    gboolean result = FALSE;
    gboolean get    = FALSE;

    /* check if we should display current value */
    gint len = strlen(name);
    if (name[len - 1] == '?') {
        /* remove the last ? char from setting name */
        name[len - 1] = '\0';
        get = TRUE;
    }

    Setting* s = g_hash_table_lookup(vp.settings, name);
    if (!s) {
        vp_echo(VP_MSG_ERROR, TRUE, "Config '%s' not found", name);
        return FALSE;
    }

    if (get || !param) {
        result = s->func(s, get);
        if (!result) {
            vp_echo(VP_MSG_ERROR, TRUE, "Could not get %s", s->alias ? s->alias : s->name);
        }
        return result;
    }

    /* if string param is given convert it to the required data type and set
     * it to the arg of the setting */
    a = setting_char_to_arg(param, s->type);
    if (a == NULL) {
        vp_echo(VP_MSG_ERROR, TRUE, "No valid value");
        return FALSE;
    }

    s->arg = *a;
    result = s->func(s, get);
    if (a->s) {
        g_free(a->s);
    }
    g_free(a);

    if (!result) {
        vp_echo(VP_MSG_ERROR, TRUE, "Could not set %s", s->alias ? s->alias : s->name);
    }
    return result;
}

/**
 * Converts string representing also given data type into and Arg.
 */
static Arg* setting_char_to_arg(const gchar* str, const Type type)
{
    if (!str) {
        return NULL;
    }

    Arg* arg = g_new0(Arg, 1);
    switch (type) {
        case TYPE_BOOLEAN:
            arg->i = g_ascii_strncasecmp(str, "true", 4) == 0
                || g_ascii_strncasecmp(str, "on", 2) == 0 ? 1 : 0;
            break;

        case TYPE_INTEGER:
            arg->i = g_ascii_strtoull(str, (gchar**)NULL, 10);
            break;

        case TYPE_FLOAT:
            arg->i = (1000000 * g_ascii_strtod(str, (gchar**)NULL));
            break;

        case TYPE_CHAR:
        case TYPE_COLOR:
        case TYPE_FONT:
            arg->s = g_strdup(str);
            break;
    }

    return arg;
}

/**
 * Print the setting value to the input box.
 */
static void setting_print_value(const Setting* s, void* value)
{
    const gchar* name = s->alias ? s->alias : s->name;
    gchar* string = NULL;

    switch (s->type) {
        case TYPE_BOOLEAN:
            vp_echo(VP_MSG_NORMAL, FALSE, "  %s=%s", name, *(gboolean*)value ? "true" : "false");
            break;

        case TYPE_INTEGER:
            vp_echo(VP_MSG_NORMAL, FALSE, "  %s=%d", name, *(gint*)value);
            break;

        case TYPE_FLOAT:
            vp_echo(VP_MSG_NORMAL, FALSE, "  %s=%g", name, *(gfloat*)value);
            break;

        case TYPE_CHAR:
            vp_echo(VP_MSG_NORMAL, FALSE, "  %s=%s", name, (gchar*)value);
            break;

        case TYPE_COLOR:
            string = VP_COLOR_TO_STRING((VpColor*)value);
            vp_echo(VP_MSG_NORMAL, FALSE, "  %s=%s", name, string);
            g_free(string);
            break;

        case TYPE_FONT:
            string = pango_font_description_to_string((PangoFontDescription*)value);
            vp_echo(VP_MSG_NORMAL, FALSE, "  %s=%s", name, string);
            g_free(string);
            break;
    }
}

static gboolean setting_webkit(const Setting* s, const gboolean get)
{
    WebKitWebSettings* web_setting = webkit_web_view_get_settings(vp.gui.webview);

    switch (s->type) {
        case TYPE_BOOLEAN:
            if (get) {
                gboolean value;
                g_object_get(G_OBJECT(web_setting), s->name, &value, NULL);
                setting_print_value(s, &value);
            } else {
                g_object_set(G_OBJECT(web_setting), s->name, s->arg.i ? TRUE : FALSE, NULL);
            }
            break;

        case TYPE_INTEGER:
            if (get) {
                gint value;
                g_object_get(G_OBJECT(web_setting), s->name, &value, NULL);
                setting_print_value(s, &value);
            } else {
                g_object_set(G_OBJECT(web_setting), s->name, s->arg.i, NULL);
            }
            break;

        case TYPE_FLOAT:
            if (get) {
                gfloat value;
                g_object_get(G_OBJECT(web_setting), s->name, &value, NULL);
                setting_print_value(s, &value);
            } else {
                g_object_set(G_OBJECT(web_setting), s->name, (gfloat)(s->arg.i / 1000000.0), NULL);
            }
            break;

        case TYPE_CHAR:
        case TYPE_COLOR:
        case TYPE_FONT:
            if (get) {
                gchar* value = NULL;
                g_object_get(G_OBJECT(web_setting), s->name, value, NULL);
                setting_print_value(s, value);
            } else {
                g_object_set(G_OBJECT(web_setting), s->name, s->arg.s, NULL);
            }
            break;
    }

    return TRUE;
}

static gboolean setting_cookie_timeout(const Setting* s, const gboolean get)
{
    if (get) {
        setting_print_value(s, &vp.config.cookie_timeout);
    } else {
        vp.config.cookie_timeout = s->arg.i;
    }

    return TRUE;
}

static gboolean setting_scrollstep(const Setting* s, const gboolean get)
{
    if (get) {
        setting_print_value(s, &vp.config.scrollstep);
    } else {
        vp.config.scrollstep = s->arg.i;
    }

    return TRUE;
}

static gboolean setting_status_color_bg(const Setting* s, const gboolean get)
{
    if (get) {
        setting_print_value(s, &vp.style.status_bg);
    } else {
        VP_COLOR_PARSE(&vp.style.status_bg, s->arg.s);
        VP_WIDGET_OVERRIDE_BACKGROUND(vp.gui.eventbox, GTK_STATE_NORMAL, &vp.style.status_bg);
        VP_WIDGET_OVERRIDE_BACKGROUND(GTK_WIDGET(vp.gui.statusbar.left), GTK_STATE_NORMAL, &vp.style.status_bg);
        VP_WIDGET_OVERRIDE_BACKGROUND(GTK_WIDGET(vp.gui.statusbar.right), GTK_STATE_NORMAL, &vp.style.status_bg);
    }

    return TRUE;
}

static gboolean setting_status_color_fg(const Setting* s, const gboolean get)
{
    if (get) {
        setting_print_value(s, &vp.style.status_fg);
    } else {
        VP_COLOR_PARSE(&vp.style.status_fg, s->arg.s);
        VP_WIDGET_OVERRIDE_COLOR(vp.gui.eventbox, GTK_STATE_NORMAL, &vp.style.status_fg);
        VP_WIDGET_OVERRIDE_COLOR(GTK_WIDGET(vp.gui.statusbar.left), GTK_STATE_NORMAL, &vp.style.status_fg);
        VP_WIDGET_OVERRIDE_COLOR(GTK_WIDGET(vp.gui.statusbar.right), GTK_STATE_NORMAL, &vp.style.status_fg);
    }

    return TRUE;
}

static gboolean setting_status_font(const Setting* s, const gboolean get)
{
    if (get) {
        setting_print_value(s, vp.style.status_font);
    } else {
        if (vp.style.status_font) {
            /* free previous font description */
            pango_font_description_free(vp.style.status_font);
        }
        vp.style.status_font = pango_font_description_from_string(s->arg.s);

        VP_WIDGET_OVERRIDE_FONT(vp.gui.eventbox, vp.style.status_font);
        VP_WIDGET_OVERRIDE_FONT(GTK_WIDGET(vp.gui.statusbar.left), vp.style.status_font);
        VP_WIDGET_OVERRIDE_FONT(GTK_WIDGET(vp.gui.statusbar.right), vp.style.status_font);
    }

    return TRUE;
}

static gboolean setting_input_style(const Setting* s, const gboolean get)
{
    Style* style = &vp.style;
    MessageType type = g_str_has_suffix(s->name, "normal") ? VP_MSG_NORMAL : VP_MSG_ERROR;

    if (s->type == TYPE_FONT) {
        /* input font */
        if (get) {
            setting_print_value(s, style->input_font[type]);
        } else {
            if (style->input_font[type]) {
                pango_font_description_free(style->input_font[type]);
            }
            style->input_font[type] = pango_font_description_from_string(s->arg.s);
        }
    } else {
        VpColor* color = NULL;
        if (g_str_has_prefix(s->name, "input-bg")) {
            /* background color */
            color = &style->input_bg[type];
        } else {
            /* foreground color */
            color = &style->input_fg[type];
        }

        if (get) {
            setting_print_value(s, color);
        } else {
            VP_COLOR_PARSE(color, s->arg.s);
        }
    }
    if (!get) {
        /* echo already visible input text to apply the new style to input box */
        vp_echo(VP_MSG_NORMAL, FALSE, GET_TEXT());
    }

    return TRUE;
}

static gboolean setting_completion_style(const Setting* s, const gboolean get)
{
    Style* style = &vp.style;
    CompletionStyle type = g_str_has_suffix(s->name, "normal") ? VP_COMP_NORMAL : VP_COMP_ACTIVE;

    if (s->type == TYPE_INTEGER) {
        /* max completion items */
        if (get) {
            setting_print_value(s, &vp.config.max_completion_items);
        } else {
            vp.config.max_completion_items = s->arg.i;
        }
    } else if (s->type == TYPE_FONT) {
        if (get) {
            setting_print_value(s, style->comp_font[type]);
        } else {
            if (style->comp_font[type]) {
                pango_font_description_free(style->comp_font[type]);
            }
            style->comp_font[type] = pango_font_description_from_string(s->arg.s);
        }
    } else {
        VpColor* color = NULL;
        if (g_str_has_prefix(s->name, "completion-bg")) {
            /* completion background color */
            color = &style->comp_bg[type];
        } else {
            /* completion foreground color */
            color = &style->comp_fg[type];
        }

        if (get) {
            setting_print_value(s, color);
        } else {
            VP_COLOR_PARSE(color, s->arg.s);
        }
    }

    return TRUE;
}

static gboolean setting_hint_style(const Setting* s, const gboolean get)
{
    Style* style = &vp.style;
    if (!g_strcmp0(s->name, "hint-bg")) {
        if (get) {
            setting_print_value(s, style->hint_bg);
        } else {
            strncpy(style->hint_bg, s->arg.s, HEX_COLOR_LEN - 1);
            style->hint_bg[HEX_COLOR_LEN - 1] = '\0';
        }
    } else if (!g_strcmp0(s->name, "hint-bg-focus")) {
        if (get) {
            setting_print_value(s, style->hint_bg_focus);
        } else {
            strncpy(style->hint_bg_focus, s->arg.s, HEX_COLOR_LEN - 1);
            style->hint_bg_focus[HEX_COLOR_LEN - 1] = '\0';
        }
    } else if (!g_strcmp0(s->name, "hint-fg")) {
        if (get) {
            setting_print_value(s, style->hint_bg_focus);
        } else {
            strncpy(style->hint_fg, s->arg.s, HEX_COLOR_LEN - 1);
            style->hint_fg[HEX_COLOR_LEN - 1] = '\0';
        }
    } else if (!g_strcmp0(s->name, "hint-style")) {
        if (get) {
            setting_print_value(s, style->hint_style);
        } else {
            if (style->hint_style) {
                g_free(style->hint_style);
                style->hint_style = NULL;
            }
            style->hint_style = g_strdup(s->arg.s);
        }
    }

    return TRUE;
}
