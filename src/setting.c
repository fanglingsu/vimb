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

#include "setting.h"
#include "util.h"

extern VbCore vb;

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
    /* alias,  name,               type,         func,           arg */
    {"images", "auto-load-images", TYPE_BOOLEAN, setting_webkit, {0}},
    {"cursivfont", "cursive-font-family", TYPE_CHAR, setting_webkit, {0}},
    {"defaultencondig", "default-encoding", TYPE_CHAR, setting_webkit, {0}},
    {"defaultfont", "default-font-family", TYPE_CHAR, setting_webkit, {0}},
    {"fontsize", "default-font-size", TYPE_INTEGER, setting_webkit, {0}},
    {"monofontsize", "default-monospace-font-size", TYPE_INTEGER, setting_webkit, {0}},
    {"caret", "enable-caret-browsing", TYPE_BOOLEAN, setting_webkit, {0}},
    {"webinspector", "enable-developer-extras", TYPE_BOOLEAN, setting_webkit, {0}},
    {"offlinecache", "enable-offline-web-application-cache", TYPE_BOOLEAN, setting_webkit, {0}},
    {"pagecache", "enable-page-cache", TYPE_BOOLEAN, setting_webkit, {0}},
    {"plugins", "enable-plugins", TYPE_BOOLEAN, setting_webkit, {0}},
    {"scripts", "enable-scripts", TYPE_BOOLEAN, setting_webkit, {0}},
    {"xssauditor", "enable-xss-auditor", TYPE_BOOLEAN, setting_webkit, {0}},
    {"minimumfontsize", "minimum-font-size", TYPE_INTEGER, setting_webkit, {0}},
    {"monofont", "monospace-font-family", TYPE_CHAR, setting_webkit, {0}},
    {"backgrounds", "print-backgrounds", TYPE_BOOLEAN, setting_webkit, {0}},
    {"sansfont", "sans-serif-font-family", TYPE_CHAR, setting_webkit, {0}},
    {"seriffont", "serif-font-family", TYPE_CHAR, setting_webkit, {0}},
    {"useragent", "user-agent", TYPE_CHAR, setting_webkit, {0}},

    /* internal variables */
    {NULL, "stylesheet", TYPE_BOOLEAN, setting_user_style, {0}},

    {NULL, "proxy", TYPE_BOOLEAN, setting_proxy, {0}},
    {NULL, "cookie-timeout", TYPE_INTEGER, setting_cookie_timeout, {0}},
    {NULL, "strict-ssl", TYPE_BOOLEAN, setting_strict_ssl, {0}},

    {NULL, "scrollstep", TYPE_INTEGER, setting_scrollstep, {0}},
    {NULL, "status-color-bg", TYPE_COLOR, setting_status_color_bg, {0}},
    {NULL, "status-color-fg", TYPE_COLOR, setting_status_color_fg, {0}},
    {NULL, "status-font", TYPE_FONT, setting_status_font, {0}},
    {NULL, "status-ssl-color-bg", TYPE_COLOR, setting_status_color_bg, {0}},
    {NULL, "status-ssl-color-fg", TYPE_COLOR, setting_status_color_fg, {0}},
    {NULL, "status-ssl-font", TYPE_FONT, setting_status_font, {0}},
    {NULL, "status-sslinvalid-color-bg", TYPE_COLOR, setting_status_color_bg, {0}},
    {NULL, "status-sslinvalid-color-fg", TYPE_COLOR, setting_status_color_fg, {0}},
    {NULL, "status-sslinvalid-font", TYPE_FONT, setting_status_font, {0}},
    {NULL, "input-bg-normal", TYPE_COLOR, setting_input_style, {0}},
    {NULL, "input-bg-error", TYPE_COLOR, setting_input_style, {0}},
    {NULL, "input-fg-normal", TYPE_COLOR, setting_input_style, {0}},
    {NULL, "input-fg-error", TYPE_COLOR, setting_input_style, {0}},
    {NULL, "input-font-normal", TYPE_FONT, setting_input_style, {0}},
    {NULL, "input-font-error", TYPE_FONT, setting_input_style, {0}},
    {NULL, "completion-font", TYPE_FONT, setting_completion_style, {0}},
    {NULL, "completion-fg-normal", TYPE_COLOR, setting_completion_style, {0}},
    {NULL, "completion-fg-active", TYPE_COLOR, setting_completion_style, {0}},
    {NULL, "completion-bg-normal", TYPE_COLOR, setting_completion_style, {0}},
    {NULL, "completion-bg-active", TYPE_COLOR, setting_completion_style, {0}},
    {NULL, "max-completion-items", TYPE_INTEGER, setting_completion_style, {0}},
    {NULL, "hint-bg", TYPE_CHAR, setting_hint_style, {0}},
    {NULL, "hint-bg-focus", TYPE_CHAR, setting_hint_style, {0}},
    {NULL, "hint-fg", TYPE_CHAR, setting_hint_style, {0}},
    {NULL, "hint-style", TYPE_CHAR, setting_hint_style, {0}},
    {NULL, "ca-bundle", TYPE_CHAR, setting_ca_bundle, {0}},
    {NULL, "home-page", TYPE_CHAR, setting_home_page, {0}},
    {NULL, "download-path", TYPE_CHAR, setting_download_path, {0}},
    {NULL, "history-max-items", TYPE_INTEGER, setting_history_max_items, {0}},
};

void setting_init(void)
{
    Setting* s;
    unsigned int i;
    vb.settings = g_hash_table_new(g_str_hash, g_str_equal);

    for (i = 0; i < LENGTH(default_settings); i++) {
        s = &default_settings[i];
        /* use alias as key if available */
        g_hash_table_insert(vb.settings, (gpointer)s->alias != NULL ? s->alias : s->name, s);
    }
}

void setting_cleanup(void)
{
    if (vb.settings) {
        g_hash_table_destroy(vb.settings);
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

    Setting* s = g_hash_table_lookup(vb.settings, name);
    if (!s) {
        vb_echo(VB_MSG_ERROR, TRUE, "Config '%s' not found", name);
        return FALSE;
    }

    if (type == SETTING_SET) {
        /* if string param is given convert it to the required data type and set
         * it to the arg of the setting */
        a = setting_char_to_arg(param, s->type);
        if (a == NULL) {
            vb_echo(VB_MSG_ERROR, TRUE, "No valid value");
            return FALSE;
        }

        s->arg = *a;
        result = s->func(s, get);
        if (a->s) {
            g_free(a->s);
        }
        g_free(a);

        if (!result) {
            vb_echo(VB_MSG_ERROR, TRUE, "Could not set %s", s->alias ? s->alias : s->name);
        }

        return result;
    }

    if (type == SETTING_GET) {
        result = s->func(s, type);
        if (!result) {
            vb_echo(VB_MSG_ERROR, TRUE, "Could not get %s", s->alias ? s->alias : s->name);
        }

        return result;
    }

    /* toggle bolean vars */
    if (s->type != TYPE_BOOLEAN) {
        vb_echo(VB_MSG_ERROR, TRUE, "Could not toggle none boolean %s", s->alias ? s->alias : s->name);

        return FALSE;
    }

    result = s->func(s, type);
    if (!result) {
        vb_echo(VB_MSG_ERROR, TRUE, "Could not toggle %s", s->alias ? s->alias : s->name);
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
            vb_echo(VB_MSG_NORMAL, FALSE, "  %s=%s", name, *(gboolean*)value ? "true" : "false");
            break;

        case TYPE_INTEGER:
            vb_echo(VB_MSG_NORMAL, FALSE, "  %s=%d", name, *(int*)value);
            break;

        case TYPE_FLOAT:
            vb_echo(VB_MSG_NORMAL, FALSE, "  %s=%g", name, *(gfloat*)value);
            break;

        case TYPE_CHAR:
            vb_echo(VB_MSG_NORMAL, FALSE, "  %s=%s", name, (char*)value);
            break;

        case TYPE_COLOR:
            string = VB_COLOR_TO_STRING((VbColor*)value);
            vb_echo(VB_MSG_NORMAL, FALSE, "  %s=%s", name, string);
            g_free(string);
            break;

        case TYPE_FONT:
            string = pango_font_description_to_string((PangoFontDescription*)value);
            vb_echo(VB_MSG_NORMAL, FALSE, "  %s=%s", name, string);
            g_free(string);
            break;
    }
}

static gboolean setting_webkit(const Setting* s, const SettingType type)
{
    WebKitWebSettings* web_setting = webkit_web_view_get_settings(vb.gui.webview);

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
        setting_print_value(s, &vb.config.cookie_timeout);
    } else {
        vb.config.cookie_timeout = s->arg.i;
    }

    return TRUE;
}

static gboolean setting_scrollstep(const Setting* s, const SettingType type)
{
    if (type == SETTING_GET) {
        setting_print_value(s, &vb.config.scrollstep);
    } else {
        vb.config.scrollstep = s->arg.i;
    }

    return TRUE;
}

static gboolean setting_status_color_bg(const Setting* s, const SettingType type)
{
    StatusType stype;
    if (g_str_has_prefix(s->name, "status-sslinvalid")) {
        stype = VB_STATUS_SSL_INVALID;
    } else if (g_str_has_prefix(s->name, "status-ssl")) {
        stype = VB_STATUS_SSL_VALID;
    } else {
        stype = VB_STATUS_NORMAL;
    }

    if (type == SETTING_GET) {
        setting_print_value(s, &vb.style.status_bg[stype]);
    } else {
        VB_COLOR_PARSE(&vb.style.status_bg[stype], s->arg.s);
        vb_update_status_style();
    }

    return TRUE;
}

static gboolean setting_status_color_fg(const Setting* s, const SettingType type)
{
    StatusType stype;
    if (g_str_has_prefix(s->name, "status-sslinvalid")) {
        stype = VB_STATUS_SSL_INVALID;
    } else if (g_str_has_prefix(s->name, "status-ssl")) {
        stype = VB_STATUS_SSL_VALID;
    } else {
        stype = VB_STATUS_NORMAL;
    }

    if (type == SETTING_GET) {
        setting_print_value(s, &vb.style.status_fg[stype]);
    } else {
        VB_COLOR_PARSE(&vb.style.status_fg[stype], s->arg.s);
        vb_update_status_style();
    }

    return TRUE;
}

static gboolean setting_status_font(const Setting* s, const SettingType type)
{
    StatusType stype;
    if (g_str_has_prefix(s->name, "status-sslinvalid")) {
        stype = VB_STATUS_SSL_INVALID;
    } else if (g_str_has_prefix(s->name, "status-ssl")) {
        stype = VB_STATUS_SSL_VALID;
    } else {
        stype = VB_STATUS_NORMAL;
    }

    if (type == SETTING_GET) {
        setting_print_value(s, vb.style.status_font[stype]);
    } else {
        if (vb.style.status_font[stype]) {
            /* free previous font description */
            pango_font_description_free(vb.style.status_font[stype]);
        }
        vb.style.status_font[stype] = pango_font_description_from_string(s->arg.s);
        vb_update_status_style();
    }

    return TRUE;
}

static gboolean setting_input_style(const Setting* s, const SettingType type)
{
    Style* style = &vb.style;
    MessageType itype = g_str_has_suffix(s->name, "normal") ? VB_MSG_NORMAL : VB_MSG_ERROR;

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
        VbColor* color = NULL;
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
            VB_COLOR_PARSE(color, s->arg.s);
        }
    }
    if (type != SETTING_GET) {
        /* vb_update_input_style seems to take no immediatly effect */
        vb_echo(VB_MSG_NORMAL, FALSE, GET_TEXT());
    }

    return TRUE;
}

static gboolean setting_completion_style(const Setting* s, const SettingType type)
{
    Style* style = &vb.style;
    CompletionStyle ctype = g_str_has_suffix(s->name, "normal") ? VB_COMP_NORMAL : VB_COMP_ACTIVE;

    if (s->type == TYPE_INTEGER) {
        /* max completion items */
        if (type == SETTING_GET) {
            setting_print_value(s, &vb.config.max_completion_items);
        } else {
            vb.config.max_completion_items = s->arg.i;
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
        VbColor* color = NULL;
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
            VB_COLOR_PARSE(color, s->arg.s);
        }
    }

    return TRUE;
}

static gboolean setting_hint_style(const Setting* s, const SettingType type)
{
    Style* style = &vb.style;
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
        g_object_get(vb.soup_session, "ssl-strict", &value, NULL);
        if (type == SETTING_GET) {
            setting_print_value(s, &value);

            return TRUE;
        }
    }

    value = type == SETTING_TOGGLE ? !value : (s->arg.i ? TRUE : FALSE);

    g_object_set(vb.soup_session, "ssl-strict", value, NULL);

    return TRUE;
}

static gboolean setting_ca_bundle(const Setting* s, const SettingType type)
{
    if (type == SETTING_GET) {
        char* value = NULL;
        g_object_get(vb.soup_session, "ssl-ca-file", &value, NULL);
        setting_print_value(s, value);
        g_free(value);
    } else {
        g_object_set(vb.soup_session, "ssl-ca-file", s->arg.s, NULL);
    }

    return TRUE;
}

static gboolean setting_home_page(const Setting* s, const SettingType type)
{
    if (type == SETTING_GET) {
        setting_print_value(s, vb.config.home_page);
    } else {
        OVERWRITE_STRING(vb.config.home_page, s->arg.s);
    }

    return TRUE;
}

static gboolean setting_download_path(const Setting* s, const SettingType type)
{
    if (type == SETTING_GET) {
        setting_print_value(s, vb.config.download_dir);
    } else {
        if (vb.config.download_dir) {
            g_free(vb.config.download_dir);
            vb.config.download_dir = NULL;
        }
        /* if path is not absolute create it in the home directory */
        if (s->arg.s[0] != '/') {
            vb.config.download_dir = g_build_filename(util_get_home_dir(), s->arg.s, NULL);
        } else {
            vb.config.download_dir = g_strdup(s->arg.s);
        }
        /* create the path if it does not exist */
        util_create_dir_if_not_exists(vb.config.download_dir);
    }

    return TRUE;
}

static gboolean setting_proxy(const Setting* s, const SettingType type)
{
    gboolean enabled;
    SoupURI* proxy_uri = NULL;

    /* get the current status */
    if (type != SETTING_SET) {
        g_object_get(vb.soup_session, "proxy-uri", &proxy_uri, NULL);
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

            g_object_set(vb.soup_session, "proxy-uri", proxy_uri, NULL);

            soup_uri_free(proxy_uri);
            g_free(proxy_new);
        }
    } else {
        g_object_set(vb.soup_session, "proxy-uri", NULL, NULL);
    }

    return TRUE;
}

static gboolean setting_user_style(const Setting* s, const SettingType type)
{
    gboolean enabled = FALSE;
    char* uri = NULL;
    WebKitWebSettings* web_setting = webkit_web_view_get_settings(vb.gui.webview);
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
        uri = g_strconcat("file://", vb.files[FILES_USER_STYLE], NULL);
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
        setting_print_value(s, &vb.config.url_history_max);

        return TRUE;
    }
    vb.config.url_history_max = s->arg.i;

    return TRUE;
}
