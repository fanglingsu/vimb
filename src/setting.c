/**
 * vimp - a webkit based vim like browser.
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

#include "setting.h"
#include "util.h"

static Arg* setting_char_to_arg(const char* str, const Type type);
static void setting_print_value(const Setting* s, void* value);
static gboolean setting_webkit(const Setting* s, const SettingType type);
static gboolean setting_cookie_timeout(const Setting* s, const SettingType type);
static gboolean setting_scrollstep(const Setting* s, const SettingType type);
static gboolean setting_status_color_bg(const Setting* s, const SettingType type);
static gboolean setting_status_color_fg(const Setting* s, const SettingType type);
static gboolean setting_status_font(const Setting* s, const SettingType type);
static gboolean setting_input_style(const Setting* s, const SettingType type);
static gboolean setting_completion_style(const Setting* s, const SettingType type);
static gboolean setting_hint_style(const Setting* s, const SettingType type);
static gboolean setting_strict_ssl(const Setting* s, const SettingType type);
static gboolean setting_ca_bundle(const Setting* s, const SettingType type);
static gboolean setting_home_page(const Setting* s, const SettingType type);
static gboolean setting_download_path(const Setting* s, const SettingType type);
static gboolean setting_proxy(const Setting* s, const SettingType type);
static gboolean setting_user_style(const Setting* s, const SettingType type);
static gboolean setting_history_max_items(const Setting* s, const SettingType type);

static Setting default_settings[] = {
    /* webkit settings */
    {"images", "auto-load-images", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"shrinkimages", "auto-shrink-images", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"cursivfont", "cursive-font-family", TYPE_CHAR, setting_webkit, {.s = "serif"}},
    {"defaultencondig", "default-encoding", TYPE_CHAR, setting_webkit, {.s = "utf-8"}},
    {"defaultfont", "default-font-family", TYPE_CHAR, setting_webkit, {.s = "sans-serif"}},
    {"fontsize", "default-font-size", TYPE_INTEGER, setting_webkit, {.i = 11}},
    {"monofontsize", "default-monospace-font-size", TYPE_INTEGER, setting_webkit, {.i = 11}},
    {"caret", "enable-caret-browsing", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"webinspector", "enable-developer-extras", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"dnsprefetching", "enable-dns-prefetching", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"dompaste", "enable-dom-paste", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"frameflattening", "enable-frame-flattening", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {NULL, "enable-file-access-from-file-uris", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {NULL, "enable-html5-database", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {NULL, "enable-html5-local-storage", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"javaapplet", "enable-java-applet", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"offlinecache", "enable-offline-web-application-cache", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"pagecache", "enable-page-cache", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"plugins", "enable-plugins", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {"scripts", "enable-scripts", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
    {NULL, "enable-site-specific-quirks", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {NULL, "enable-spatial-navigation", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"spell", "enable-spell-checking", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {NULL, "enable-universal-access-from-file-uris", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {NULL, "enable-webgl", TYPE_BOOLEAN, setting_webkit, {.i = 0}},
    {"xssauditor", "enable-xss-auditor", TYPE_BOOLEAN, setting_webkit, {.i = 1}},
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
    {"zoomstep", "zoom-step", TYPE_FLOAT, setting_webkit, {.i = 100000}},
    /* internal variables */
    {NULL, "proxy", TYPE_BOOLEAN, setting_proxy, {.i = 1}},
    {NULL, "cookie-timeout", TYPE_INTEGER, setting_cookie_timeout, {.i = 4800}},
    {NULL, "scrollstep", TYPE_INTEGER, setting_scrollstep, {.i = 40}},

    {NULL, "status-color-bg", TYPE_CHAR, setting_status_color_bg, {.s = "#000"}},
    {NULL, "status-color-fg", TYPE_CHAR, setting_status_color_fg, {.s = "#fff"}},
    {NULL, "status-font", TYPE_FONT, setting_status_font, {.s = "monospace bold 8"}},
    {NULL, "status-ssl-color-bg", TYPE_CHAR, setting_status_color_bg, {.s = "#95e454"}},
    {NULL, "status-ssl-color-fg", TYPE_CHAR, setting_status_color_fg, {.s = "#000"}},
    {NULL, "status-ssl-font", TYPE_FONT, setting_status_font, {.s = "monospace bold 8"}},
    {NULL, "status-sslinvalid-color-bg", TYPE_CHAR, setting_status_color_bg, {.s = "#f08080"}},
    {NULL, "status-sslinvalid-color-fg", TYPE_CHAR, setting_status_color_fg, {.s = "#000"}},
    {NULL, "status-sslinvalid-font", TYPE_FONT, setting_status_font, {.s = "monospace bold 8"}},

    {NULL, "input-bg-normal", TYPE_COLOR, setting_input_style, {.s = "#fff"}},
    {NULL, "input-bg-error", TYPE_COLOR, setting_input_style, {.s = "#f00"}},
    {NULL, "input-fg-normal", TYPE_COLOR, setting_input_style, {.s = "#000"}},
    {NULL, "input-fg-error", TYPE_COLOR, setting_input_style, {.s = "#000"}},
    {NULL, "input-font-normal", TYPE_FONT, setting_input_style, {.s = "monospace normal 8"}},
    {NULL, "input-font-error", TYPE_FONT, setting_input_style, {.s = "monospace bold 8"}},
    {NULL, "completion-font", TYPE_FONT, setting_completion_style, {.s = "monospace normal 8"}},
    {NULL, "completion-fg-normal", TYPE_COLOR, setting_completion_style, {.s = "#f6f3e8"}},
    {NULL, "completion-fg-active", TYPE_COLOR, setting_completion_style, {.s = "#fff"}},
    {NULL, "completion-bg-normal", TYPE_COLOR, setting_completion_style, {.s = "#656565"}},
    {NULL, "completion-bg-active", TYPE_COLOR, setting_completion_style, {.s = "#777777"}},
    {NULL, "max-completion-items", TYPE_INTEGER, setting_completion_style, {.i = 15}},
    {NULL, "hint-bg", TYPE_CHAR, setting_hint_style, {.s = "#ff0"}},
    {NULL, "hint-bg-focus", TYPE_CHAR, setting_hint_style, {.s = "#8f0"}},
    {NULL, "hint-fg", TYPE_CHAR, setting_hint_style, {.s = "#000"}},
    {NULL, "hint-style", TYPE_CHAR, setting_hint_style, {.s = "position:absolute;z-index:100000;font-family:monospace;font-weight:bold;font-size:10px;color:#000;background-color:#fff;margin:0;padding:0px 1px;border:1px solid #444;opacity:0.7;"}},
    {NULL, "strict-ssl", TYPE_BOOLEAN, setting_strict_ssl, {.i = 1}},
    {NULL, "ca-bundle", TYPE_CHAR, setting_ca_bundle, {.s = "/etc/ssl/certs/ca-certificates.crt"}},
    {NULL, "home-page", TYPE_CHAR, setting_home_page, {.s = "https://github.com/fanglingsu/vimp"}},
    {NULL, "download-path", TYPE_CHAR, setting_download_path, {.s = "/tmp/vimp"}},
    {NULL, "stylesheet", TYPE_BOOLEAN, setting_user_style, {.i = 1}},
    {NULL, "history-max-items", TYPE_INTEGER, setting_history_max_items, {.i = 500}},
};


void setting_init(void)
{
    Setting* s;
    guint i;
    core.settings = g_hash_table_new(g_str_hash, g_str_equal);

    for (i = 0; i < LENGTH(default_settings); i++) {
        s = &default_settings[i];
        /* use alias as key if available */
        g_hash_table_insert(core.settings, (gpointer)s->alias != NULL ? s->alias : s->name, s);

        /* set the default settings */
        s->func(s, FALSE);
    }
}

void setting_cleanup(void)
{
    if (core.settings) {
        g_hash_table_destroy(core.settings);
    }
}

gboolean setting_run(char* name, const char* param)
{
    Arg* a          = NULL;
    gboolean result = FALSE;
    gboolean get    = FALSE;
    SettingType type = SETTING_SET;


    /* determine the type to names last char and param */
    int len = strlen(name);
    if (name[len - 1] == '?') {
        name[len - 1] = '\0';
        type = SETTING_GET;
    } else if (name[len - 1] == '!') {
        name[len - 1] = '\0';
        type = SETTING_TOGGLE;
    } else if (!param) {
        type = SETTING_GET;
    }

    Setting* s = g_hash_table_lookup(core.settings, name);
    if (!s) {
        vp_echo(VP_MSG_ERROR, TRUE, "Config '%s' not found", name);
        return FALSE;
    }

    if (type == SETTING_SET) {
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

    if (type == SETTING_GET) {
        result = s->func(s, type);
        if (!result) {
            vp_echo(VP_MSG_ERROR, TRUE, "Could not get %s", s->alias ? s->alias : s->name);
        }

        return result;
    }

    /* toggle bolean vars */
    if (s->type != TYPE_BOOLEAN) {
        vp_echo(VP_MSG_ERROR, TRUE, "Could not toggle none boolean %s", s->alias ? s->alias : s->name);

        return FALSE;
    }

    result = s->func(s, type);
    if (!result) {
        vp_echo(VP_MSG_ERROR, TRUE, "Could not toggle %s", s->alias ? s->alias : s->name);
    }

    return result;
}

/**
 * Converts string representing also given data type into and Arg.
 */
static Arg* setting_char_to_arg(const char* str, const Type type)
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
            arg->i = g_ascii_strtoull(str, (char**)NULL, 10);
            break;

        case TYPE_FLOAT:
            arg->i = (1000000 * g_ascii_strtod(str, (char**)NULL));
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
    const char* name = s->alias ? s->alias : s->name;
    char* string = NULL;

    switch (s->type) {
        case TYPE_BOOLEAN:
            vp_echo(VP_MSG_NORMAL, FALSE, "  %s=%s", name, *(gboolean*)value ? "true" : "false");
            break;

        case TYPE_INTEGER:
            vp_echo(VP_MSG_NORMAL, FALSE, "  %s=%d", name, *(int*)value);
            break;

        case TYPE_FLOAT:
            vp_echo(VP_MSG_NORMAL, FALSE, "  %s=%g", name, *(gfloat*)value);
            break;

        case TYPE_CHAR:
            vp_echo(VP_MSG_NORMAL, FALSE, "  %s=%s", name, (char*)value);
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

static gboolean setting_webkit(const Setting* s, const SettingType type)
{
    WebKitWebSettings* web_setting = webkit_web_view_get_settings(vp.gui.webview);

    switch (s->type) {
        case TYPE_BOOLEAN:
            /* acts for type toggle and get */
            if (type != SETTING_SET) {
                gboolean value;
                g_object_get(G_OBJECT(web_setting), s->name, &value, NULL);

                /* toggle the variable */
                if (type == SETTING_TOGGLE) {
                    value = !value;
                    g_object_set(G_OBJECT(web_setting), s->name, value, NULL);
                }

                /* print the new value */
                setting_print_value(s, &value);
            } else {
                g_object_set(G_OBJECT(web_setting), s->name, s->arg.i ? TRUE : FALSE, NULL);
            }
            break;

        case TYPE_INTEGER:
            if (type == SETTING_GET) {
                int value;
                g_object_get(G_OBJECT(web_setting), s->name, &value, NULL);
                setting_print_value(s, &value);
            } else {
                g_object_set(G_OBJECT(web_setting), s->name, s->arg.i, NULL);
            }
            break;

        case TYPE_FLOAT:
            if (type == SETTING_GET) {
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
            if (type == SETTING_GET) {
                char* value = NULL;
                g_object_get(G_OBJECT(web_setting), s->name, &value, NULL);
                setting_print_value(s, value);
            } else {
                g_object_set(G_OBJECT(web_setting), s->name, s->arg.s, NULL);
            }
            break;
    }

    return TRUE;
}
static gboolean setting_cookie_timeout(const Setting* s, const SettingType type)
{
    if (type == SETTING_GET) {
        setting_print_value(s, &core.config.cookie_timeout);
    } else {
        core.config.cookie_timeout = s->arg.i;
    }

    return TRUE;
}

static gboolean setting_scrollstep(const Setting* s, const SettingType type)
{
    if (type == SETTING_GET) {
        setting_print_value(s, &core.config.scrollstep);
    } else {
        core.config.scrollstep = s->arg.i;
    }

    return TRUE;
}

static gboolean setting_status_color_bg(const Setting* s, const SettingType type)
{
    StatusType stype;
    if (g_str_has_prefix(s->name, "status-sslinvalid")) {
        stype = VP_STATUS_SSL_INVALID;
    } else if (g_str_has_prefix(s->name, "status-ssl")) {
        stype = VP_STATUS_SSL_VALID;
    } else {
        stype = VP_STATUS_NORMAL;
    }

    if (type == SETTING_GET) {
        setting_print_value(s, &core.style.status_bg[stype]);
    } else {
        VP_COLOR_PARSE(&core.style.status_bg[stype], s->arg.s);
        vp_update_status_style();
    }

    return TRUE;
}

static gboolean setting_status_color_fg(const Setting* s, const SettingType type)
{
    StatusType stype;
    if (g_str_has_prefix(s->name, "status-sslinvalid")) {
        stype = VP_STATUS_SSL_INVALID;
    } else if (g_str_has_prefix(s->name, "status-ssl")) {
        stype = VP_STATUS_SSL_VALID;
    } else {
        stype = VP_STATUS_NORMAL;
    }

    if (type == SETTING_GET) {
        setting_print_value(s, &core.style.status_fg[stype]);
    } else {
        VP_COLOR_PARSE(&core.style.status_fg[stype], s->arg.s);
        vp_update_status_style();
    }

    return TRUE;
}

static gboolean setting_status_font(const Setting* s, const SettingType type)
{
    StatusType stype;
    if (g_str_has_prefix(s->name, "status-sslinvalid")) {
        stype = VP_STATUS_SSL_INVALID;
    } else if (g_str_has_prefix(s->name, "status-ssl")) {
        stype = VP_STATUS_SSL_VALID;
    } else {
        stype = VP_STATUS_NORMAL;
    }

    if (type == SETTING_GET) {
        setting_print_value(s, core.style.status_font[stype]);
    } else {
        if (core.style.status_font[stype]) {
            /* free previous font description */
            pango_font_description_free(core.style.status_font[stype]);
        }
        core.style.status_font[stype] = pango_font_description_from_string(s->arg.s);

        vp_update_status_style();
    }

    return TRUE;
}

static gboolean setting_input_style(const Setting* s, const SettingType type)
{
    Style* style = &core.style;
    MessageType itype = g_str_has_suffix(s->name, "normal") ? VP_MSG_NORMAL : VP_MSG_ERROR;

    if (s->type == TYPE_FONT) {
        /* input font */
        if (type == SETTING_GET) {
            setting_print_value(s, style->input_font[itype]);
        } else {
            if (style->input_font[itype]) {
                pango_font_description_free(style->input_font[itype]);
            }
            style->input_font[itype] = pango_font_description_from_string(s->arg.s);
        }
    } else {
        VpColor* color = NULL;
        if (g_str_has_prefix(s->name, "input-bg")) {
            /* background color */
            color = &style->input_bg[itype];
        } else {
            /* foreground color */
            color = &style->input_fg[itype];
        }

        if (type == SETTING_GET) {
            setting_print_value(s, color);
        } else {
            VP_COLOR_PARSE(color, s->arg.s);
        }
    }
    if (type != SETTING_GET) {
        /* echo already visible input text to apply the new style to input box */
        vp_echo(VP_MSG_NORMAL, FALSE, GET_TEXT());
    }

    return TRUE;
}

static gboolean setting_completion_style(const Setting* s, const SettingType type)
{
    Style* style = &core.style;
    CompletionStyle ctype = g_str_has_suffix(s->name, "normal") ? VP_COMP_NORMAL : VP_COMP_ACTIVE;

    if (s->type == TYPE_INTEGER) {
        /* max completion items */
        if (type == SETTING_GET) {
            setting_print_value(s, &core.config.max_completion_items);
        } else {
            core.config.max_completion_items = s->arg.i;
        }
    } else if (s->type == TYPE_FONT) {
        if (type == SETTING_GET) {
            setting_print_value(s, style->comp_font);
        } else {
            if (style->comp_font) {
                pango_font_description_free(style->comp_font);
            }
            style->comp_font = pango_font_description_from_string(s->arg.s);
        }
    } else {
        VpColor* color = NULL;
        if (g_str_has_prefix(s->name, "completion-bg")) {
            /* completion background color */
            color = &style->comp_bg[ctype];
        } else {
            /* completion foreground color */
            color = &style->comp_fg[ctype];
        }

        if (type == SETTING_GET) {
            setting_print_value(s, color);
        } else {
            VP_COLOR_PARSE(color, s->arg.s);
        }
    }

    return TRUE;
}

static gboolean setting_hint_style(const Setting* s, const SettingType type)
{
    Style* style = &core.style;
    if (!g_strcmp0(s->name, "hint-bg")) {
        if (type == SETTING_GET) {
            setting_print_value(s, style->hint_bg);
        } else {
            OVERWRITE_STRING(style->hint_bg, s->arg.s)
        }
    } else if (!g_strcmp0(s->name, "hint-bg-focus")) {
        if (type == SETTING_GET) {
            setting_print_value(s, style->hint_bg_focus);
        } else {
            OVERWRITE_STRING(style->hint_bg_focus, s->arg.s)
        }
    } else if (!g_strcmp0(s->name, "hint-fg")) {
        if (type == SETTING_GET) {
            setting_print_value(s, style->hint_fg);
        } else {
            OVERWRITE_STRING(style->hint_fg, s->arg.s)
        }
    } else {
        if (type == SETTING_GET) {
            setting_print_value(s, style->hint_style);
        } else {
            OVERWRITE_STRING(style->hint_style, s->arg.s);
        }
    }

    return TRUE;
}

static gboolean setting_strict_ssl(const Setting* s, const SettingType type)
{
    gboolean value;
    if (type != SETTING_SET) {
        g_object_get(core.soup_session, "ssl-strict", &value, NULL);
        if (type == SETTING_GET) {
            setting_print_value(s, &value);

            return TRUE;
        }
    }

    value = type == SETTING_TOGGLE ? !value : (s->arg.i ? TRUE : FALSE);

    g_object_set(core.soup_session, "ssl-strict", value, NULL);

    return TRUE;
}

static gboolean setting_ca_bundle(const Setting* s, const SettingType type)
{
    if (type == SETTING_GET) {
        char* value = NULL;
        g_object_get(core.soup_session, "ssl-ca-file", &value, NULL);
        setting_print_value(s, value);
        g_free(value);
    } else {
        g_object_set(core.soup_session, "ssl-ca-file", s->arg.s, NULL);
    }

    return TRUE;
}

static gboolean setting_home_page(const Setting* s, const SettingType type)
{
    if (type == SETTING_GET) {
        setting_print_value(s, core.config.home_page);
    } else {
        OVERWRITE_STRING(core.config.home_page, s->arg.s);
    }

    return TRUE;
}

static gboolean setting_download_path(const Setting* s, const SettingType type)
{
    if (type == SETTING_GET) {
        setting_print_value(s, core.config.download_dir);
    } else {
        if (core.config.download_dir) {
            g_free(core.config.download_dir);
            core.config.download_dir = NULL;
        }
        /* if path is not absolute create it in the home directory */
        if (s->arg.s[0] != '/') {
            core.config.download_dir = g_build_filename(util_get_home_dir(), s->arg.s, NULL);
        } else {
            core.config.download_dir = g_strdup(s->arg.s);
        }
        /* create the path if it does not exist */
        util_create_dir_if_not_exists(core.config.download_dir);
    }

    return TRUE;
}

static gboolean setting_proxy(const Setting* s, const SettingType type)
{
    gboolean enabled;
    SoupURI* proxy_uri = NULL;

    /* get the current status */
    if (type != SETTING_SET) {
        g_object_get(core.soup_session, "proxy-uri", &proxy_uri, NULL);
        enabled = (proxy_uri != NULL);

        if (type == SETTING_GET) {
            setting_print_value(s, &enabled);

            return TRUE;
        }
    }

    if (type == SETTING_TOGGLE) {
        enabled = !enabled;
        /* print the new value */
        setting_print_value(s, &enabled);
    } else {
        enabled = s->arg.i;
    }

    if (enabled) {
        char* proxy = (char *)g_getenv("http_proxy");
        if (proxy != NULL && strlen(proxy)) {
            char* proxy_new = g_strrstr(proxy, "http://")
                ? g_strdup(proxy)
                : g_strdup_printf("http://%s", proxy);
            proxy_uri = soup_uri_new(proxy_new);

            g_object_set(core.soup_session, "proxy-uri", proxy_uri, NULL);

            soup_uri_free(proxy_uri);
            g_free(proxy_new);
        }
    } else {
        g_object_set(core.soup_session, "proxy-uri", NULL, NULL);
    }

    return TRUE;
}

static gboolean setting_user_style(const Setting* s, const SettingType type)
{
    gboolean enabled = FALSE;
    char* uri = NULL;
    WebKitWebSettings* web_setting = webkit_web_view_get_settings(vp.gui.webview);
    if (type != SETTING_SET) {
        g_object_get(web_setting, "user-stylesheet-uri", &uri, NULL);
        enabled = (uri != NULL);

        if (type == SETTING_GET) {
            setting_print_value(s, &enabled);

            return TRUE;
        }
    }

    if (type == SETTING_TOGGLE) {
        enabled = !enabled;
        /* print the new value */
        setting_print_value(s, &enabled);
    } else {
        enabled = s->arg.i;
    }

    if (enabled) {
        uri = g_strconcat("file://", core.files[FILES_USER_STYLE], NULL);
        g_object_set(web_setting, "user-stylesheet-uri", uri, NULL);
        g_free(uri);
    } else {
        g_object_set(web_setting, "user-stylesheet-uri", NULL, NULL);
    }

    return TRUE;
}

static gboolean setting_history_max_items(const Setting* s, const SettingType type)
{
    if (type == SETTING_GET) {
        setting_print_value(s, &core.config.url_history_max);

        return TRUE;
    }

    core.config.url_history_max = s->arg.i;

    return TRUE;
}
