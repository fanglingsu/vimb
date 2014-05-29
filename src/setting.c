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
#include "shortcut.h"
#include "handlers.h"
#include "util.h"
#include "completion.h"
#include "js.h"
#ifdef FEATURE_HSTS
#include "hsts.h"
#endif

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
static SettingStatus fullscreen(const Setting *s, const SettingType type);
#ifdef FEATURE_HSTS
static SettingStatus hsts(const Setting *s, const SettingType type);
#endif

static gboolean validate_js_regexp_list(const char *pattern);

static Setting default_settings[] = {
    /* webkit settings */
    /* alias,  name,               type,         func,           arg */
    {"images", "auto-load-images", TYPE_BOOLEAN, webkit, {.i = 1}},
    {"cursivfont", "cursive-font-family", TYPE_CHAR, webkit, {.s = "serif"}},
    {"defaultencoding", "default-encoding", TYPE_CHAR, webkit, {.s = "utf-8"}},
    {"defaultfont", "default-font-family", TYPE_CHAR, webkit, {.s = "sans-serif"}},
    {"fontsize", "default-font-size", TYPE_INTEGER, webkit, {.i = SETTING_DEFAULT_FONT_SIZE}},
    {"monofontsize", "default-monospace-font-size", TYPE_INTEGER, webkit, {.i = SETTING_DEFAULT_FONT_SIZE}},
    {"caret", "enable-caret-browsing", TYPE_BOOLEAN, webkit, {.i = 0}},
    {"webinspector", "enable-developer-extras", TYPE_BOOLEAN, webkit, {.i = 0}},
    {"offlinecache", "enable-offline-web-application-cache", TYPE_BOOLEAN, webkit, {.i = 1}},
    {"pagecache", "enable-page-cache", TYPE_BOOLEAN, webkit, {.i = 1}},
    {"plugins", "enable-plugins", TYPE_BOOLEAN, webkit, {.i = 1}},
    {"scripts", "enable-scripts", TYPE_BOOLEAN, webkit, {.i = 1}},
    {"xssauditor", "enable-xss-auditor", TYPE_BOOLEAN, webkit, {.i = 1}},
    {"minimumfontsize", "minimum-font-size", TYPE_INTEGER, webkit, {.i = 5}},
    {"monofont", "monospace-font-family", TYPE_CHAR, webkit, {.s = "monospace"}},
    {NULL, "print-backgrounds", TYPE_BOOLEAN, webkit, {.i = 1}},
    {"sansfont", "sans-serif-font-family", TYPE_CHAR, webkit, {.s = "sans-serif"}},
    {"seriffont", "serif-font-family", TYPE_CHAR, webkit, {.s = "serif"}},
    {"useragent", "user-agent", TYPE_CHAR, webkit, {.s = PROJECT "/" VERSION " (X11; Linux i686) AppleWebKit/535.22+ Compatible (Safari)"}},
    {"spacial-navigation", "enable-spatial-navigation", TYPE_BOOLEAN, webkit, {.i = 0}},
#if WEBKIT_CHECK_VERSION(2, 0, 0)
    {"insecure-content-show", "enable-display-of-insecure-content", TYPE_BOOLEAN, webkit, {.i = 0}},
    {"insecure-content-run", "enable-running-of-insecure-content", TYPE_BOOLEAN, webkit, {.i = 0}},
#endif

    /* internal variables */
    {NULL, "stylesheet", TYPE_BOOLEAN, user_style, {.i = 1}},

    {NULL, "proxy", TYPE_BOOLEAN, proxy, {.i = 1}},
#ifdef FEATURE_COOKIE
    {NULL, "cookie-accept", TYPE_CHAR, cookie_accept, {.s = "always"}},
    {NULL, "cookie-timeout", TYPE_INTEGER, cookie_timeout, {.i = 4800}},
#endif
    {NULL, "strict-ssl", TYPE_BOOLEAN, strict_ssl, {.i = 1}},
    {NULL, "strict-focus", TYPE_BOOLEAN, strict_focus, {.i = 0}},

    {NULL, "scrollstep", TYPE_INTEGER, scrollstep, {.i = 40}},
    {NULL, "status-color-bg", TYPE_COLOR, status_color_bg, {.s = "#000"}},
    {NULL, "status-color-fg", TYPE_COLOR, status_color_fg, {.s = "#fff"}},
    {NULL, "status-font", TYPE_FONT, status_font, {.s = SETTING_GUI_FONT_EMPH}},
    {NULL, "status-ssl-color-bg", TYPE_COLOR, status_color_bg, {.s = "#95e454"}},
    {NULL, "status-ssl-color-fg", TYPE_COLOR, status_color_fg, {.s = "#000"}},
    {NULL, "status-ssl-font", TYPE_FONT, status_font, {.s = SETTING_GUI_FONT_EMPH}},
    {NULL, "status-sslinvalid-color-bg", TYPE_COLOR, status_color_bg, {.s = "#f77"}},
    {NULL, "status-sslinvalid-color-fg", TYPE_COLOR, status_color_fg, {.s = "#000"}},
    {NULL, "status-sslinvalid-font", TYPE_FONT, status_font, {.s = SETTING_GUI_FONT_EMPH}},
    {NULL, "timeoutlen", TYPE_INTEGER, timeoutlen, {.i = 1000}},
    {NULL, "input-bg-normal", TYPE_COLOR, input_style, {.s = "#fff"}},
    {NULL, "input-bg-error", TYPE_COLOR, input_style, {.s = "#f77"}},
    {NULL, "input-fg-normal", TYPE_COLOR, input_style, {.s = "#000"}},
    {NULL, "input-fg-error", TYPE_COLOR, input_style, {.s = "#000"}},
    {NULL, "input-font-normal", TYPE_FONT, input_style, {.s = SETTING_GUI_FONT_NORMAL}},
    {NULL, "input-font-error", TYPE_FONT, input_style, {.s = SETTING_GUI_FONT_EMPH}},
    {NULL, "completion-font", TYPE_FONT, completion_style, {.s = SETTING_GUI_FONT_NORMAL}},
    {NULL, "completion-fg-normal", TYPE_COLOR, completion_style, {.s = "#f6f3e8"}},
    {NULL, "completion-fg-active", TYPE_COLOR, completion_style, {.s = "#fff"}},
    {NULL, "completion-bg-normal", TYPE_COLOR, completion_style, {.s = "#656565"}},
    {NULL, "completion-bg-active", TYPE_COLOR, completion_style, {.s = "#777"}},
    {NULL, "ca-bundle", TYPE_CHAR, ca_bundle, {.s = "/etc/ssl/certs/ca-certificates.crt"}},
    {NULL, "home-page", TYPE_CHAR, home_page, {.s = SETTING_HOME_PAGE}},
    {NULL, "download-path", TYPE_CHAR, download_path, {.s = ""}},
    {NULL, "history-max-items", TYPE_INTEGER, history_max_items, {.i = 2000}},
    {NULL, "editor-command", TYPE_CHAR, editor_command, {.s = "x-terminal-emulator -e vi %s"}},
    {NULL, "header", TYPE_CHAR, headers, {.s = ""}},
    {NULL, "nextpattern", TYPE_CHAR, nextpattern, {.s = "/\\bnext\\b/i,/^(>\\|>>\\|»)$/,/^(>\\|>>\\|»)/,/(>\\|>>\\|»)$/,/\\bmore\\b/i"}},
    {NULL, "previouspattern", TYPE_CHAR, nextpattern, {.s = "/\\bprev\\|previous\\b/i,/^(<\\|<<\\|«)$/,/^(<\\|<<\\|«)/,/(<\\|<<\\|«)$/"}},
    {NULL, "fullscreen", TYPE_BOOLEAN, fullscreen, {.i = 0}},
#ifdef FEATURE_HSTS
    {NULL, "hsts", TYPE_BOOLEAN, hsts, {.i = 1}},
#endif
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
        /* set the default settings */
        s->func(s, false);
    }

    /* initialize the shortcuts and set the default shortcuts */
    shortcut_init();
    shortcut_add("dl", "https://duckduckgo.com/html/?q=$0");
    shortcut_add("dd", "https://duckduckgo.com/?q=$0");
    shortcut_set_default("dl");

    /* initialize the handlers */
    handlers_init();
    handler_add("magnet", "xdg-open %s");
}

void setting_cleanup(void)
{
    if (settings) {
        g_hash_table_destroy(settings);
    }
    shortcut_cleanup();
    handlers_cleanup();
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
    GList *src = g_hash_table_get_keys(settings);
    gboolean found = util_fill_completion(store, input, src);
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
    char *expanded;
    GError *error = NULL;
    if (type == SETTING_GET) {
        print_value(s, vb.config.cafile);
    } else {
        /* expand the given file and set it to the file database */
        expanded         = util_expand(s->arg.s);
        vb.config.tls_db = g_tls_file_database_new(expanded, &error);
        g_free(expanded);
        if (error) {
            g_warning("Could not load ssl database '%s': %s", s->arg.s, error->message);
            g_error_free(error);

            return SETTING_ERROR;
        }

        /* there is no function to get the file back from tls file database so
         * it's saved as seperate configuration */
        OVERWRITE_STRING(vb.config.cafile, s->arg.s);
        g_object_set(vb.session, "tls-database", vb.config.tls_db, NULL);
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
        OVERWRITE_STRING(vb.config.download_dir, s->arg.s);
    }

    return SETTING_OK;
}

static SettingStatus proxy(const Setting *s, const SettingType type)
{
    gboolean enabled;
#if SOUP_CHECK_VERSION(2, 42, 2)
    GProxyResolver *proxy = NULL;
#else
    SoupURI *proxy = NULL;
#endif

    /* get the current status */
    if (type != SETTING_SET) {
#if SOUP_CHECK_VERSION(2, 42, 2)
        g_object_get(vb.session, "proxy-resolver", &proxy, NULL);
#else
        g_object_get(vb.session, "proxy-uri", &proxy, NULL);
#endif

        enabled = (proxy != NULL);

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
        const char *http_proxy = g_getenv("http_proxy");

        if (http_proxy != NULL && *http_proxy != '\0') {
            char *proxy_new = g_str_has_prefix(http_proxy, "http://")
                ? g_strdup(http_proxy)
                : g_strconcat("http://", http_proxy, NULL);

#if SOUP_CHECK_VERSION(2, 42, 2)
            const char *no_proxy;
            char **ignored_hosts = NULL;
            /* check for no_proxy environment variable that contains comma
             * seperated domains or ip addresses to skip from proxy */
            if ((no_proxy = g_getenv("no_proxy"))) {
                ignored_hosts = g_strsplit(no_proxy, ",", 0);
            }

            proxy = g_simple_proxy_resolver_new(proxy_new, ignored_hosts);
            if (proxy) {
                g_object_set(vb.session, "proxy-resolver", proxy, NULL);
            }
            g_strfreev(ignored_hosts);
            g_object_unref(proxy);
#else
            proxy = soup_uri_new(proxy_new);
            if (proxy && SOUP_URI_VALID_FOR_HTTP(proxy)) {
                g_object_set(vb.session, "proxy-uri", proxy, NULL);
            }
            soup_uri_free(proxy);
#endif
            g_free(proxy_new);
        }
    } else {
        /* disable the proxy */
#if SOUP_CHECK_VERSION(2, 42, 2)
        g_object_set(vb.session, "proxy-resolver", NULL, NULL);
#else
        g_object_set(vb.session, "proxy-uri", NULL, NULL);
#endif
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
            print_value(s, *(str->str) == ',' ? str->str + 1 : str->str);
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

static SettingStatus fullscreen(const Setting *s, const SettingType type)
{
    if (type == SETTING_GET) {
        print_value(s, &vb.config.fullscreen);

        return SETTING_OK;
    }

    if (type == SETTING_SET) {
        vb.config.fullscreen = s->arg.i ? true : false;
    } else {
        vb.config.fullscreen = !vb.config.fullscreen;
        print_value(s, &vb.config.fullscreen);
    }

    /* apply the new set or toggled value */
    if (vb.config.fullscreen) {
        gtk_window_fullscreen(GTK_WINDOW(vb.gui.window));
    } else {
        gtk_window_unfullscreen(GTK_WINDOW(vb.gui.window));
    }

    return SETTING_OK;
}

#ifdef FEATURE_HSTS
static SettingStatus hsts(const Setting *s, const SettingType type)
{
    gboolean active;
    if (type == SETTING_GET) {
        active = (soup_session_get_feature(vb.session, HSTS_TYPE_PROVIDER) != NULL);
        print_value(s, &active);

        return SETTING_OK;
    }

    if (type == SETTING_TOGGLE) {
        active = (soup_session_get_feature(vb.session, HSTS_TYPE_PROVIDER) == NULL);
        print_value(s, &active);
    } else {
        active = (s->arg.i != 0);
    }

    if (active) {
        soup_session_add_feature(vb.session, SOUP_SESSION_FEATURE(vb.config.hsts_provider));
    } else {
        soup_session_remove_feature(vb.session, SOUP_SESSION_FEATURE(vb.config.hsts_provider));
    }
    return SETTING_OK;
}
#endif

/**
 * Validated syntax given list of JavaScript RegExp patterns.
 * If validation fails, the error is shown to the user.
 */
static gboolean validate_js_regexp_list(const char *pattern)
{
    gboolean result;
    char *js, *value = NULL;

    js     = g_strdup_printf("var i;for(i=0;i<[%s].length;i++);", pattern);
    result = js_eval(webkit_web_view_get_main_frame(vb.gui.webview), js, NULL, &value);
    g_free(js);
    if (!result) {
        vb_echo(VB_MSG_ERROR, true, "%s", value);
        g_free(value);

        return false;
    }
    return true;
}
