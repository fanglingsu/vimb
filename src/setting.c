/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2014 Daniel Carl
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

#include "config.h"
#include "setting.h"
#include "util.h"
#include "completion.h"

static GHashTable *settings;

extern VbCore vb;

static Arg *char_to_arg(const char *str, const Type type);
static void print_value(const Setting *s, void *value);
static SettingStatus webkit(const Setting *s, const SettingType type);
#ifdef FEATURE_COOKIE
static SettingStatus cookie_accept(const Setting *s, const SettingType type);
static SettingStatus cookie_timeout(const Setting *s, const SettingType type);
#endif
static SettingStatus scrollstep(const Setting *s, const SettingType type);
static SettingStatus status_color_bg(const Setting *s, const SettingType type);
static SettingStatus status_color_fg(const Setting *s, const SettingType type);
static SettingStatus status_font(const Setting *s, const SettingType type);
static SettingStatus input_style(const Setting *s, const SettingType type);
static SettingStatus completion_style(const Setting *s, const SettingType type);
static SettingStatus strict_ssl(const Setting *s, const SettingType type);
static SettingStatus strict_focus(const Setting *s, const SettingType type);
static SettingStatus ca_bundle(const Setting *s, const SettingType type);
static SettingStatus home_page(const Setting *s, const SettingType type);
static SettingStatus download_path(const Setting *s, const SettingType type);
static SettingStatus proxy(const Setting *s, const SettingType type);
static SettingStatus user_style(const Setting *s, const SettingType type);
static SettingStatus history_max_items(const Setting *s, const SettingType type);
static SettingStatus editor_command(const Setting *s, const SettingType type);
static SettingStatus timeoutlen(const Setting *s, const SettingType type);
static SettingStatus headers(const Setting *s, const SettingType type);
static SettingStatus nextpattern(const Setting *s, const SettingType type);

static gboolean validate_js_regexp_list(const char *pattern);

static Setting default_settings[] = {
    /* webkit settings */
    /* alias,  name,               type,         func,           arg */
    {"images", "auto-load-images", TYPE_BOOLEAN, webkit, {0}},
    {"cursivfont", "cursive-font-family", TYPE_CHAR, webkit, {0}},
    {"defaultencoding", "default-encoding", TYPE_CHAR, webkit, {0}},
    {"defaultfont", "default-font-family", TYPE_CHAR, webkit, {0}},
    {"fontsize", "default-font-size", TYPE_INTEGER, webkit, {0}},
    {"monofontsize", "default-monospace-font-size", TYPE_INTEGER, webkit, {0}},
    {"caret", "enable-caret-browsing", TYPE_BOOLEAN, webkit, {0}},
    {"webinspector", "enable-developer-extras", TYPE_BOOLEAN, webkit, {0}},
    {"offlinecache", "enable-offline-web-application-cache", TYPE_BOOLEAN, webkit, {0}},
    {"pagecache", "enable-page-cache", TYPE_BOOLEAN, webkit, {0}},
    {"plugins", "enable-plugins", TYPE_BOOLEAN, webkit, {0}},
    {"scripts", "enable-scripts", TYPE_BOOLEAN, webkit, {0}},
    {"xssauditor", "enable-xss-auditor", TYPE_BOOLEAN, webkit, {0}},
    {"minimumfontsize", "minimum-font-size", TYPE_INTEGER, webkit, {0}},
    {"monofont", "monospace-font-family", TYPE_CHAR, webkit, {0}},
    {NULL, "print-backgrounds", TYPE_BOOLEAN, webkit, {0}},
    {"sansfont", "sans-serif-font-family", TYPE_CHAR, webkit, {0}},
    {"seriffont", "serif-font-family", TYPE_CHAR, webkit, {0}},
    {"useragent", "user-agent", TYPE_CHAR, webkit, {0}},
    {"spacial-navigation", "enable-spatial-navigation", TYPE_BOOLEAN, webkit, {0}},
#if WEBKIT_CHECK_VERSION(2, 0, 0)
    {"insecure-content-show", "enable-display-of-insecure-content", TYPE_BOOLEAN, webkit, {0}},
    {"insecure-content-run", "enable-running-of-insecure-content", TYPE_BOOLEAN, webkit, {0}},
#endif

    /* internal variables */
    {NULL, "stylesheet", TYPE_BOOLEAN, user_style, {0}},

    {NULL, "proxy", TYPE_BOOLEAN, proxy, {0}},
#ifdef FEATURE_COOKIE
    {NULL, "cookie-accept", TYPE_CHAR, cookie_accept, {0}},
    {NULL, "cookie-timeout", TYPE_INTEGER, cookie_timeout, {0}},
#endif
    {NULL, "strict-ssl", TYPE_BOOLEAN, strict_ssl, {0}},
    {NULL, "strict-focus", TYPE_BOOLEAN, strict_focus, {0}},

    {NULL, "scrollstep", TYPE_INTEGER, scrollstep, {0}},
    {NULL, "status-color-bg", TYPE_COLOR, status_color_bg, {0}},
    {NULL, "status-color-fg", TYPE_COLOR, status_color_fg, {0}},
    {NULL, "status-font", TYPE_FONT, status_font, {0}},
    {NULL, "status-ssl-color-bg", TYPE_COLOR, status_color_bg, {0}},
    {NULL, "status-ssl-color-fg", TYPE_COLOR, status_color_fg, {0}},
    {NULL, "status-ssl-font", TYPE_FONT, status_font, {0}},
    {NULL, "status-sslinvalid-color-bg", TYPE_COLOR, status_color_bg, {0}},
    {NULL, "status-sslinvalid-color-fg", TYPE_COLOR, status_color_fg, {0}},
    {NULL, "status-sslinvalid-font", TYPE_FONT, status_font, {0}},
    {NULL, "timeoutlen", TYPE_INTEGER, timeoutlen, {0}},
    {NULL, "input-bg-normal", TYPE_COLOR, input_style, {0}},
    {NULL, "input-bg-error", TYPE_COLOR, input_style, {0}},
    {NULL, "input-fg-normal", TYPE_COLOR, input_style, {0}},
    {NULL, "input-fg-error", TYPE_COLOR, input_style, {0}},
    {NULL, "input-font-normal", TYPE_FONT, input_style, {0}},
    {NULL, "input-font-error", TYPE_FONT, input_style, {0}},
    {NULL, "completion-font", TYPE_FONT, completion_style, {0}},
    {NULL, "completion-fg-normal", TYPE_COLOR, completion_style, {0}},
    {NULL, "completion-fg-active", TYPE_COLOR, completion_style, {0}},
    {NULL, "completion-bg-normal", TYPE_COLOR, completion_style, {0}},
    {NULL, "completion-bg-active", TYPE_COLOR, completion_style, {0}},
    {NULL, "ca-bundle", TYPE_CHAR, ca_bundle, {0}},
    {NULL, "home-page", TYPE_CHAR, home_page, {0}},
    {NULL, "download-path", TYPE_CHAR, download_path, {0}},
    {NULL, "history-max-items", TYPE_INTEGER, history_max_items, {0}},
    {NULL, "editor-command", TYPE_CHAR, editor_command, {0}},
    {NULL, "header", TYPE_CHAR, headers, {0}},
    {NULL, "nextpattern", TYPE_CHAR, nextpattern, {0}},
    {NULL, "previouspattern", TYPE_CHAR, nextpattern, {0}},
};

void setting_init(void)
{
    Setting *s;
    unsigned int i;
    settings = g_hash_table_new(g_str_hash, g_str_equal);

    for (i = 0; i < LENGTH(default_settings); i++) {
        s = &default_settings[i];
        /* use alias as key if available */
        g_hash_table_insert(settings, (gpointer)s->alias != NULL ? s->alias : s->name, s);
    }
}

void setting_cleanup(void)
{
    if (settings) {
        g_hash_table_destroy(settings);
    }
}

gboolean setting_run(char *name, const char *param)
{
    Arg *a = NULL;
    gboolean get = false;
    SettingStatus result = SETTING_ERROR;
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

    Setting *s = g_hash_table_lookup(settings, name);
    if (!s) {
        vb_echo(VB_MSG_ERROR, true, "Config '%s' not found", name);
        return false;
    }

    if (type == SETTING_SET) {
        /* if string param is given convert it to the required data type and set
         * it to the arg of the setting */
        a = char_to_arg(param, s->type);
        if (a == NULL) {
            vb_echo(VB_MSG_ERROR, true, "No valid value");
            return false;
        }

        s->arg = *a;
        result = s->func(s, get);
        if (a->s) {
            g_free(a->s);
        }
        g_free(a);

        if (result == SETTING_OK || result & SETTING_USER_NOTIFIED) {
            return true;
        }

        vb_echo(VB_MSG_ERROR, true, "Could not set %s", s->alias ? s->alias : s->name);
        return false;
    }

    if (type == SETTING_GET) {
        result = s->func(s, type);
        if (result == SETTING_OK || result & SETTING_USER_NOTIFIED) {
            return true;
        }

        vb_echo(VB_MSG_ERROR, true, "Could not get %s", s->alias ? s->alias : s->name);
        return false;
    }

    /* toggle bolean vars */
    if (s->type != TYPE_BOOLEAN) {
        vb_echo(VB_MSG_ERROR, true, "Could not toggle none boolean %s", s->alias ? s->alias : s->name);

        return false;
    }

    result = s->func(s, type);
    if (result == SETTING_OK || result & SETTING_USER_NOTIFIED) {
        return true;
    }

    vb_echo(VB_MSG_ERROR, true, "Could not toggle %s", s->alias ? s->alias : s->name);
    return false;
}

gboolean setting_fill_completion(GtkListStore *store, const char *input)
{
    gboolean found = false;
    GtkTreeIter iter;
    GList *src = g_hash_table_get_keys(settings);

    if (!input || !*input) {
        for (GList *l = src; l; l = l->next) {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, COMPLETION_STORE_FIRST, l->data, -1);
            found = true;
        }
    } else {
        for (GList *l = src; l; l = l->next) {
            char *value = (char*)l->data;
            if (g_str_has_prefix(value, input)) {
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter, COMPLETION_STORE_FIRST, l->data, -1);
                found = true;
            }
        }
    }
    g_list_free(src);

    return found;
}

/**
 * Converts string representing also given data type into and Arg.
 *
 * Returned Arg must be freed with g_free.
 */
static Arg *char_to_arg(const char *str, const Type type)
{
    if (!str) {
        return NULL;
    }

    Arg *arg = g_new0(Arg, 1);
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
static void print_value(const Setting *s, void *value)
{
    const char *name = s->alias ? s->alias : s->name;
    char *string = NULL;

    switch (s->type) {
        case TYPE_BOOLEAN:
            vb_echo(VB_MSG_NORMAL, false, "  %s=%s", name, *(gboolean*)value ? "true" : "false");
            break;

        case TYPE_INTEGER:
            vb_echo(VB_MSG_NORMAL, false, "  %s=%d", name, *(int*)value);
            break;

        case TYPE_FLOAT:
            vb_echo(VB_MSG_NORMAL, false, "  %s=%g", name, *(gfloat*)value);
            break;

        case TYPE_CHAR:
            vb_echo(VB_MSG_NORMAL, false, "  %s=%s", name, (char*)value);
            break;

        case TYPE_COLOR:
            string = VB_COLOR_TO_STRING((VbColor*)value);
            vb_echo(VB_MSG_NORMAL, false, "  %s=%s", name, string);
            g_free(string);
            break;

        case TYPE_FONT:
            string = pango_font_description_to_string((PangoFontDescription*)value);
            vb_echo(VB_MSG_NORMAL, false, "  %s=%s", name, string);
            g_free(string);
            break;
    }
}

static SettingStatus webkit(const Setting *s, const SettingType type)
{
    WebKitWebSettings *web_setting = webkit_web_view_get_settings(vb.gui.webview);

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
                print_value(s, &value);
            } else {
                g_object_set(G_OBJECT(web_setting), s->name, s->arg.i ? true : false, NULL);
            }
            break;

        case TYPE_INTEGER:
            if (type == SETTING_GET) {
                int value;
                g_object_get(G_OBJECT(web_setting), s->name, &value, NULL);
                print_value(s, &value);
            } else {
                g_object_set(G_OBJECT(web_setting), s->name, s->arg.i, NULL);
            }
            break;

        case TYPE_FLOAT:
            if (type == SETTING_GET) {
                gfloat value;
                g_object_get(G_OBJECT(web_setting), s->name, &value, NULL);
                print_value(s, &value);
            } else {
                g_object_set(G_OBJECT(web_setting), s->name, (gfloat)(s->arg.i / 1000000.0), NULL);
            }
            break;

        case TYPE_CHAR:
        case TYPE_COLOR:
        case TYPE_FONT:
            if (type == SETTING_GET) {
                char *value = NULL;
                g_object_get(G_OBJECT(web_setting), s->name, &value, NULL);
                print_value(s, value);
            } else {
                g_object_set(G_OBJECT(web_setting), s->name, s->arg.s, NULL);
            }
            break;
    }

    return SETTING_OK;
}

#ifdef FEATURE_COOKIE
static SettingStatus cookie_accept(const Setting *s, const SettingType type)
{
    int i, policy;
    SoupCookieJar *jar;
    static struct {
        SoupCookieJarAcceptPolicy policy;
        char*                     name;
    } map[] = {
        {SOUP_COOKIE_JAR_ACCEPT_ALWAYS,         "always"},
        {SOUP_COOKIE_JAR_ACCEPT_NEVER,          "never"},
        {SOUP_COOKIE_JAR_ACCEPT_NO_THIRD_PARTY, "origin"},
    };

    jar = (SoupCookieJar*)soup_session_get_feature(vb.session, SOUP_TYPE_COOKIE_JAR);

    if (type == SETTING_GET) {
        g_object_get(jar, SOUP_COOKIE_JAR_ACCEPT_POLICY, &policy, NULL);
        for (i = 0; i < LENGTH(map); i++) {
            if (policy == map[i].policy) {
                print_value(s, map[i].name);

                return SETTING_OK;
            }
        }
    } else {
        for (i = 0; i < LENGTH(map); i++) {
            if (!strcmp(map[i].name, s->arg.s)) {
                g_object_set(jar, SOUP_COOKIE_JAR_ACCEPT_POLICY, map[i].policy, NULL);

                return SETTING_OK;
            }
        }
        vb_echo(VB_MSG_ERROR, true, "%s must be in [always, origin, never]", s->name);

        return SETTING_ERROR | SETTING_USER_NOTIFIED;
    }

    return SETTING_ERROR;
}

static SettingStatus cookie_timeout(const Setting *s, const SettingType type)
{
    if (type == SETTING_GET) {
        print_value(s, &vb.config.cookie_timeout);
    } else {
        vb.config.cookie_timeout = s->arg.i;
    }

    return SETTING_OK;
}
#endif

static SettingStatus scrollstep(const Setting *s, const SettingType type)
{
    if (type == SETTING_GET) {
        print_value(s, &vb.config.scrollstep);
    } else {
        vb.config.scrollstep = s->arg.i;
    }

    return SETTING_OK;
}

static SettingStatus status_color_bg(const Setting *s, const SettingType type)
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
        print_value(s, &vb.style.status_bg[stype]);
    } else {
        VB_COLOR_PARSE(&vb.style.status_bg[stype], s->arg.s);
        vb_update_status_style();
    }

    return SETTING_OK;
}

static SettingStatus status_color_fg(const Setting *s, const SettingType type)
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
        print_value(s, &vb.style.status_fg[stype]);
    } else {
        VB_COLOR_PARSE(&vb.style.status_fg[stype], s->arg.s);
        vb_update_status_style();
    }

    return SETTING_OK;
}

static SettingStatus status_font(const Setting *s, const SettingType type)
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
        print_value(s, vb.style.status_font[stype]);
    } else {
        if (vb.style.status_font[stype]) {
            /* free previous font description */
            pango_font_description_free(vb.style.status_font[stype]);
        }
        vb.style.status_font[stype] = pango_font_description_from_string(s->arg.s);
        vb_update_status_style();
    }

    return SETTING_OK;
}

static SettingStatus input_style(const Setting *s, const SettingType type)
{
    Style *style = &vb.style;
    MessageType itype = g_str_has_suffix(s->name, "normal") ? VB_MSG_NORMAL : VB_MSG_ERROR;

    if (s->type == TYPE_FONT) {
        /* input font */
        if (type == SETTING_GET) {
            print_value(s, style->input_font[itype]);
        } else {
            if (style->input_font[itype]) {
                pango_font_description_free(style->input_font[itype]);
            }
            style->input_font[itype] = pango_font_description_from_string(s->arg.s);
        }
    } else {
        VbColor *color = NULL;
        if (g_str_has_prefix(s->name, "input-bg")) {
            /* background color */
            color = &style->input_bg[itype];
        } else {
            /* foreground color */
            color = &style->input_fg[itype];
        }

        if (type == SETTING_GET) {
            print_value(s, color);
        } else {
            VB_COLOR_PARSE(color, s->arg.s);
        }
    }
    if (type != SETTING_GET) {
        vb_update_input_style();
    }

    return SETTING_OK;
}

static SettingStatus completion_style(const Setting *s, const SettingType type)
{
    Style *style = &vb.style;
    CompletionStyle ctype = g_str_has_suffix(s->name, "normal") ? VB_COMP_NORMAL : VB_COMP_ACTIVE;

    if (s->type == TYPE_FONT) {
        if (type == SETTING_GET) {
            print_value(s, style->comp_font);
        } else {
            if (style->comp_font) {
                pango_font_description_free(style->comp_font);
            }
            style->comp_font = pango_font_description_from_string(s->arg.s);
        }
    } else {
        VbColor *color = NULL;
        if (g_str_has_prefix(s->name, "completion-bg")) {
            /* completion background color */
            color = &style->comp_bg[ctype];
        } else {
            /* completion foreground color */
            color = &style->comp_fg[ctype];
        }

        if (type == SETTING_GET) {
            print_value(s, color);
        } else {
            VB_COLOR_PARSE(color, s->arg.s);
        }
    }

    return SETTING_OK;
}

static SettingStatus strict_ssl(const Setting *s, const SettingType type)
{
    gboolean value;
    if (type != SETTING_SET) {
        g_object_get(vb.session, "ssl-strict", &value, NULL);
        if (type == SETTING_GET) {
            print_value(s, &value);

            return SETTING_OK;
        }
    }

    if (type == SETTING_TOGGLE) {
        value = !value;
        print_value(s, &value);
    } else {
        value = s->arg.i;
    }

    g_object_set(vb.session, "ssl-strict", value, NULL);

    return SETTING_OK;
}

static SettingStatus strict_focus(const Setting *s, const SettingType type)
{
    if (type != SETTING_SET) {
        if (type == SETTING_TOGGLE) {
            vb.config.strict_focus = !vb.config.strict_focus;
        }
        print_value(s, &vb.config.strict_focus);
    } else {
        vb.config.strict_focus = s->arg.i ? true : false;
    }

    return SETTING_OK;
}

static SettingStatus ca_bundle(const Setting *s, const SettingType type)
{
    char *value;
    if (type == SETTING_GET) {
        g_object_get(vb.session, "ssl-ca-file", &value, NULL);
        print_value(s, value);
        g_free(value);
    } else {
        g_object_set(vb.session, "ssl-ca-file", s->arg.s, NULL);
    }

    return SETTING_OK;
}

static SettingStatus home_page(const Setting *s, const SettingType type)
{
    if (type == SETTING_GET) {
        print_value(s, vb.config.home_page);
    } else {
        OVERWRITE_STRING(
            vb.config.home_page,
            *(s->arg.s) == '\0' ? "about:blank" : s->arg.s
        );
    }

    return SETTING_OK;
}

static SettingStatus download_path(const Setting *s, const SettingType type)
{
    if (type == SETTING_GET) {
        print_value(s, vb.config.download_dir);
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

    return SETTING_OK;
}

static SettingStatus proxy(const Setting *s, const SettingType type)
{
    gboolean enabled;
    char *proxy, *proxy_new;
    SoupURI *proxy_uri = NULL;

    /* get the current status */
    if (type != SETTING_SET) {
        g_object_get(vb.session, "proxy-uri", &proxy_uri, NULL);
        enabled = (proxy_uri != NULL);

        if (type == SETTING_GET) {
            print_value(s, &enabled);

            return SETTING_OK;
        }
    }

    if (type == SETTING_TOGGLE) {
        enabled = !enabled;
        /* print the new value */
        print_value(s, &enabled);
    } else {
        enabled = s->arg.i;
    }

    if (enabled) {
        proxy = (char *)g_getenv("http_proxy");
        if (proxy != NULL && strlen(proxy)) {
            proxy_new = g_str_has_prefix(proxy, "http://")
                ? g_strdup(proxy)
                : g_strconcat("http://", proxy, NULL);
            proxy_uri = soup_uri_new(proxy_new);

            g_object_set(vb.session, "proxy-uri", proxy_uri, NULL);

            soup_uri_free(proxy_uri);
            g_free(proxy_new);
        }
    } else {
        g_object_set(vb.session, "proxy-uri", NULL, NULL);
    }

    return SETTING_OK;
}

static SettingStatus user_style(const Setting *s, const SettingType type)
{
    gboolean enabled = false;
    char *uri = NULL;
    WebKitWebSettings *web_setting = webkit_web_view_get_settings(vb.gui.webview);
    if (type != SETTING_SET) {
        g_object_get(web_setting, "user-stylesheet-uri", &uri, NULL);
        enabled = (uri != NULL);

        if (type == SETTING_GET) {
            print_value(s, &enabled);

            return SETTING_OK;
        }
    }

    if (type == SETTING_TOGGLE) {
        enabled = !enabled;
        /* print the new value */
        print_value(s, &enabled);
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

    return SETTING_OK;
}

static SettingStatus history_max_items(const Setting *s, const SettingType type)
{
    if (type == SETTING_GET) {
        print_value(s, &vb.config.history_max);

        return SETTING_OK;
    }
    vb.config.history_max = s->arg.i;

    return SETTING_OK;
}

static SettingStatus editor_command(const Setting *s, const SettingType type)
{
    if (type == SETTING_GET) {
        print_value(s, vb.config.editor_command);

        return SETTING_OK;
    }

    OVERWRITE_STRING(vb.config.editor_command, s->arg.s);

    return SETTING_OK;
}

static SettingStatus timeoutlen(const Setting *s, const SettingType type)
{
    if (type == SETTING_GET) {
        print_value(s, &vb.config.timeoutlen);
    } else {
        vb.config.timeoutlen = abs(s->arg.i);
    }

    return SETTING_OK;
}

/**
 * Allow to set user defined http headers.
 *
 * :set header=NAME1=VALUE!,NAME2=,NAME3
 *
 * Note that these headers will replace already existing headers. If there is
 * no '=' after the header name, than the complete header will be removed from
 * the request (NAME3), if the '=' is present means that the header value is
 * set to empty value.
 */
static SettingStatus headers(const Setting *s, const SettingType type)
{
    if (type == SETTING_GET) {
        char *key, *value;
        GHashTableIter iter;
        GString *str;

        if (vb.config.headers) {
            str = g_string_new("");
            /* build a list woth the header values */
            g_hash_table_iter_init(&iter, vb.config.headers);
            while (g_hash_table_iter_next(&iter, (gpointer*)&key, (gpointer*)&value)) {
                g_string_append_c(str, ',');
                soup_header_g_string_append_param(str, key, value);
            }

            /* skip the first ',' we put into the headers string */
            print_value(s, str->str + 1);
            g_string_free(str, true);
        } else {
            print_value(s, &"");
        }
    } else {
        /* remove previous parsed headers */
        if (vb.config.headers) {
            soup_header_free_param_list(vb.config.headers);
            vb.config.headers = NULL;
        }
        vb.config.headers = soup_header_parse_param_list(s->arg.s);
    }

    return SETTING_OK;
}

static SettingStatus nextpattern(const Setting *s, const SettingType type)
{
    if (type == SETTING_GET) {
        print_value(s, s->name[0] == 'n' ? vb.config.nextpattern : vb.config.prevpattern);

        return SETTING_OK;
    }

    if (validate_js_regexp_list(s->arg.s)) {
        if (*s->name == 'n') {
            OVERWRITE_STRING(vb.config.nextpattern, s->arg.s);
        } else {
            OVERWRITE_STRING(vb.config.prevpattern, s->arg.s);
        }

        return SETTING_OK;
    }

    return SETTING_ERROR | SETTING_USER_NOTIFIED;
}

/**
 * Validated syntax given list of JavaScript RegExp patterns.
 * If validation fails, the error is shown to the user.
 */
static gboolean validate_js_regexp_list(const char *pattern)
{
    gboolean result;
    char *js, *value = NULL;

    js     = g_strdup_printf("var i;for(i=0;i<[%s].length;i++);", pattern);
    result = vb_eval_script(
        webkit_web_view_get_main_frame(vb.gui.webview), js, NULL, &value
    );
    g_free(js);
    if (!result) {
        vb_echo(VB_MSG_ERROR, true, "%s", value);
        g_free(value);

        return false;
    }
    return true;
}
