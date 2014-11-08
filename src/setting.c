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
#include <string.h>
#include "main.h"
#include "setting.h"
#include "shortcut.h"
#include "handlers.h"
#include "util.h"
#include "completion.h"
#include "js.h"
#ifdef FEATURE_HSTS
#include "hsts.h"
#endif
#include "arh.h"

typedef enum {
    TYPE_BOOLEAN,
    TYPE_INTEGER,
    TYPE_CHAR,
    TYPE_COLOR,
    TYPE_FONT,
} Type;

typedef enum {
    SETTING_SET,        /* :set option=value */
    SETTING_APPEND,     /* :set option+=value */
    SETTING_PREPEND,    /* :set option^=value */
    SETTING_REMOVE,     /* :set option-=value */
    SETTING_GET,        /* :set option? */
    SETTING_TOGGLE      /* :set option! */
} SettingType;

enum {
    FLAG_LIST  = (1<<1),    /* setting contains a ',' separated list of values */
    FLAG_NODUP = (1<<2),    /* don't allow duplicate strings within list values */
};

extern VbCore vb;

static int setting_set_value(Setting *prop, void *value, SettingType type);
static gboolean prepare_setting_value(Setting *prop, void *value, SettingType type, void **newvalue);
static gboolean setting_add(const char *name, int type, void *value,
    SettingFunction setter, int flags, void *data);
static void setting_print(Setting *s);
static void setting_free(Setting *s);
static int webkit(const char *name, int type, void *value, void *data);
static int pagecache(const char *name, int type, void *value, void *data);
static int soup(const char *name, int type, void *value, void *data);
static int internal(const char *name, int type, void *value, void *data);
static int input_autohide(const char *name, int type, void *value, void *data);
static int input_color(const char *name, int type, void *value, void *data);
static int statusbar(const char *name, int type, void *value, void *data);
static int status_color(const char *name, int type, void *value, void *data);
static int input_font(const char *name, int type, void *value, void *data);
static int status_font(const char *name, int type, void *value, void *data);
gboolean setting_fill_completion(GtkListStore *store, const char *input);
#ifdef FEATURE_COOKIE
static int cookie_accept(const char *name, int type, void *value, void *data);
#endif
static int ca_bundle(const char *name, int type, void *value, void *data);
static int proxy(const char *name, int type, void *value, void *data);
static int user_style(const char *name, int type, void *value, void *data);
static int headers(const char *name, int type, void *value, void *data);
#ifdef FEATURE_ARH
static int autoresponseheader(const char *name, int type, void *value, void *data);
#endif
static int prevnext(const char *name, int type, void *value, void *data);
static int fullscreen(const char *name, int type, void *value, void *data);
#ifdef FEATURE_HSTS
static int hsts(const char *name, int type, void *value, void *data);
#endif
#ifdef FEATURE_SOUP_CACHE
static int soup_cache(const char *name, int type, void *value, void *data);
#endif
static gboolean validate_js_regexp_list(const char *pattern);

void setting_init()
{
    int i;
    gboolean on = true, off = false;

    vb.config.settings = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, (GDestroyNotify)setting_free);
#if WEBKIT_CHECK_VERSION(1, 7, 5)
    setting_add("accelerated-compositing", TYPE_BOOLEAN, &off, webkit, 0, "enable-accelerated-compositing");
#endif
    setting_add("auto-resize-window", TYPE_BOOLEAN, &off, webkit, 0, "auto-resize-window");
    setting_add("auto-shrink-images", TYPE_BOOLEAN, &on, webkit, 0, "auto-shrink-images");
    setting_add("caret", TYPE_BOOLEAN, &off, webkit, 0, "enable-caret-browsing");
    setting_add("cursivfont", TYPE_CHAR, &"serif", webkit, 0, "cursive-font-family");
    setting_add("defaultencoding", TYPE_CHAR, &"utf-8", webkit, 0, "default-encoding");
    setting_add("defaultfont", TYPE_CHAR, &"sans-serif", webkit, 0, "default-font-family");
    setting_add("dns-prefetching", TYPE_BOOLEAN, &on, webkit, 0, "enable-dns-prefetching");
    setting_add("dom-paste", TYPE_BOOLEAN, &off, webkit, 0, "enable-dom-paste");
    setting_add("file-access-from-file-uris", TYPE_BOOLEAN, &off, webkit, 0, "enable-file-access-from-file-uris");
    i = SETTING_DEFAULT_FONT_SIZE;
    setting_add("fontsize", TYPE_INTEGER, &i, webkit, 0, "default-font-size");
    setting_add("frame-flattening", TYPE_BOOLEAN, &off, webkit, 0, "enable-frame-flattening");
    setting_add("html5-database", TYPE_BOOLEAN, &on, webkit, 0, "enable-html5-database");
    setting_add("html5-local-storage", TYPE_BOOLEAN, &on, webkit, 0, "enable-html5-local-storage");
    setting_add("hyperlink-auditing", TYPE_BOOLEAN, &off, webkit, 0, "enable-hyperlink-auditing");
    setting_add("images", TYPE_BOOLEAN, &on, webkit, 0, "auto-load-images");
#if WEBKIT_CHECK_VERSION(2, 0, 0)
    setting_add("insecure-content-show", TYPE_BOOLEAN, &off, webkit, 0, "enable-display-of-insecure-content");
    setting_add("insecure-content-run", TYPE_BOOLEAN, &off, webkit, 0, "enable-running-of-insecure-content");
#endif
    setting_add("java-applet", TYPE_BOOLEAN, &on, webkit, 0, "enable-java-applet");
    setting_add("javascript-can-access-clipboard", TYPE_BOOLEAN, &off, webkit, 0, "javascript-can-access-clipboard");
    setting_add("javascript-can-open-windows-automatically", TYPE_BOOLEAN, &off, webkit, 0, "javascript-can-open-windows-automatically");
    setting_add("media-playback-allows-inline", TYPE_BOOLEAN, &on, webkit, 0, "media-playback-allows-inline");
    setting_add("media-playback-requires-user-gesture", TYPE_BOOLEAN, &off, webkit, 0, "media-playback-requires-user-gesture");
#if WEBKIT_CHECK_VERSION(2, 4, 0)
    setting_add("media-stream", TYPE_BOOLEAN, &off, webkit, 0, "enable-media-stream");
    setting_add("mediasource", TYPE_BOOLEAN, &off, webkit, 0, "enable-mediasource");
#endif
    i = 5;
    setting_add("minimumfontsize", TYPE_INTEGER, &i, webkit, 0, "minimum-font-size");
    setting_add("monofont", TYPE_CHAR, &"monospace", webkit, 0, "monospace-font-family");
    i = SETTING_DEFAULT_FONT_SIZE;
    setting_add("monofontsize", TYPE_INTEGER, &i, webkit, 0, "default-monospace-font-size");
    setting_add("offlinecache", TYPE_BOOLEAN, &on, webkit, 0, "enable-offline-web-application-cache");
    setting_add("pagecache", TYPE_BOOLEAN, &on, pagecache, 0, "enable-page-cache");
    setting_add("plugins", TYPE_BOOLEAN, &on, webkit, 0, "enable-plugins");
    setting_add("print-backgrounds", TYPE_BOOLEAN, &on, webkit, 0, "print-backgrounds");
    setting_add("private-browsing", TYPE_BOOLEAN, &off, webkit, 0, "enable-private-browsing");
    setting_add("resizable-text-areas", TYPE_BOOLEAN, &on, webkit, 0, "resizable-text-areas");
    setting_add("respect-image-orientation", TYPE_BOOLEAN, &off, webkit, 0, "respect-image-orientation");
    setting_add("sansfont", TYPE_CHAR, &"sans-serif", webkit, 0, "sans-serif-font-family");
    setting_add("scripts", TYPE_BOOLEAN, &on, webkit, 0, "enable-scripts");
    setting_add("seriffont", TYPE_CHAR, &"serif", webkit, 0, "serif-font-family");
    setting_add("site-specific-quirks", TYPE_BOOLEAN, &off, webkit, 0, "enable-site-specific-quirks");
#if WEBKIT_CHECK_VERSION(1, 9, 0)
    setting_add("smooth-scrolling", TYPE_BOOLEAN, &off, webkit, 0, "enable-smooth-scrolling");
#endif
    setting_add("spacial-navigation", TYPE_BOOLEAN, &off, webkit, 0, "enable-spatial-navigation");
    setting_add("spell-checking", TYPE_BOOLEAN, &off, webkit, 0, "enable-spell-checking");
    setting_add("spell-checking-languages", TYPE_CHAR, NULL, webkit, 0, "spell-checking-languages");
    setting_add("tab-key-cycles-through-elements", TYPE_BOOLEAN, &on, webkit, 0, "tab-key-cycles-through-elements");
    setting_add("universal-access-from-file-uris", TYPE_BOOLEAN, &off, webkit, 0, "enable-universal-access-from-file-uris");
    setting_add("useragent", TYPE_CHAR, &"Mozilla/5.0 (X11; Linux i686) AppleWebKit/538.15+ (KHTML, like Gecko) " PROJECT "/" VERSION " Version/8.0 Safari/538.15", webkit, 0, "user-agent");
    setting_add("webaudio", TYPE_BOOLEAN, &off, webkit, 0, "enable-webaudio");
    setting_add("webgl", TYPE_BOOLEAN, &off, webkit, 0, "enable-webgl");
    setting_add("webinspector", TYPE_BOOLEAN, &off, webkit, 0, "enable-developer-extras");
    setting_add("xssauditor", TYPE_BOOLEAN, &on, webkit, 0, "enable-xss-auditor");

    /* internal variables */
    setting_add("stylesheet", TYPE_BOOLEAN, &on, user_style, 0, NULL);
    setting_add("proxy", TYPE_BOOLEAN, &on, proxy, 0, NULL);
#ifdef FEATURE_COOKIE
    setting_add("cookie-accept", TYPE_CHAR, &"always", cookie_accept, 0, NULL);
    i = 4800;
    setting_add("cookie-timeout", TYPE_INTEGER, &i, internal, 0, &vb.config.cookie_timeout);
    i = -1;
    setting_add("cookie-expire-time", TYPE_INTEGER, &i, internal, 0, &vb.config.cookie_expire_time);
#endif
    setting_add("strict-ssl", TYPE_BOOLEAN, &on, soup, 0, "ssl-strict");
    setting_add("strict-focus", TYPE_BOOLEAN, &off, internal, 0, &vb.config.strict_focus);
    i = 40;
    setting_add("scrollstep", TYPE_INTEGER, &i, internal, 0, &vb.config.scrollstep);
    setting_add("statusbar", TYPE_BOOLEAN, &on, statusbar, 0, NULL);
    setting_add("status-color-bg", TYPE_COLOR, &"#000000", status_color, 0, &vb.style.status_bg[VB_STATUS_NORMAL]);
    setting_add("status-color-fg", TYPE_COLOR, &"#ffffff", status_color, 0, &vb.style.status_fg[VB_STATUS_NORMAL]);
    setting_add("status-font", TYPE_FONT, &SETTING_GUI_FONT_EMPH, status_font, 0, &vb.style.status_font[VB_STATUS_NORMAL]);
    setting_add("status-ssl-color-bg", TYPE_COLOR, &"#95e454", status_color, 0, &vb.style.status_bg[VB_STATUS_SSL_VALID]);
    setting_add("status-ssl-color-fg", TYPE_COLOR, &"#000000", status_color, 0, &vb.style.status_fg[VB_STATUS_SSL_VALID]);
    setting_add("status-ssl-font", TYPE_FONT, &SETTING_GUI_FONT_EMPH, status_font, 0, &vb.style.status_font[VB_STATUS_SSL_VALID]);
    setting_add("status-sslinvalid-color-bg", TYPE_COLOR, &"#ff7777", status_color, 0, &vb.style.status_bg[VB_STATUS_SSL_INVALID]);
    setting_add("status-sslinvalid-color-fg", TYPE_COLOR, &"#000000", status_color, 0, &vb.style.status_fg[VB_STATUS_SSL_INVALID]);
    setting_add("status-sslinvalid-font", TYPE_FONT, &SETTING_GUI_FONT_EMPH, status_font, 0, &vb.style.status_font[VB_STATUS_SSL_INVALID]);
    i = 1000;
    setting_add("timeoutlen", TYPE_INTEGER, &i, internal, 0, &vb.config.timeoutlen);
    setting_add("input-autohide", TYPE_BOOLEAN, &off, input_autohide, 0, &vb.config.input_autohide);
    setting_add("input-bg-normal", TYPE_COLOR, &"#ffffff", input_color, 0, &vb.style.input_bg[VB_MSG_NORMAL]);
    setting_add("input-bg-error", TYPE_COLOR, &"#ff7777", input_color, 0, &vb.style.input_bg[VB_MSG_ERROR]);
    setting_add("input-fg-normal", TYPE_COLOR, &"#000000", input_color, 0, &vb.style.input_fg[VB_MSG_NORMAL]);
    setting_add("input-fg-error", TYPE_COLOR, &"#000000", input_color, 0, &vb.style.input_fg[VB_MSG_ERROR]);
    setting_add("input-font-normal", TYPE_FONT, &SETTING_GUI_FONT_NORMAL, input_font, 0, &vb.style.input_font[VB_MSG_NORMAL]);
    setting_add("input-font-error", TYPE_FONT, &SETTING_GUI_FONT_EMPH, input_font, 0, &vb.style.input_font[VB_MSG_ERROR]);
    setting_add("completion-font", TYPE_FONT, &SETTING_GUI_FONT_NORMAL, input_font, 0, &vb.style.comp_font);
    setting_add("completion-fg-normal", TYPE_COLOR, &"#f6f3e8", input_color, 0, &vb.style.comp_fg[VB_COMP_NORMAL]);
    setting_add("completion-fg-active", TYPE_COLOR, &"#ffffff", input_color, 0, &vb.style.comp_fg[VB_COMP_ACTIVE]);
    setting_add("completion-bg-normal", TYPE_COLOR, &"#656565", input_color, 0, &vb.style.comp_bg[VB_COMP_NORMAL]);
    setting_add("completion-bg-active", TYPE_COLOR, &"#777777", input_color, 0, &vb.style.comp_bg[VB_COMP_ACTIVE]);
    setting_add("ca-bundle", TYPE_CHAR, &SETTING_CA_BUNDLE, ca_bundle, 0, NULL);
    setting_add("home-page", TYPE_CHAR, &SETTING_HOME_PAGE, NULL, 0, NULL);
    i = 1000;
    setting_add("hint-timeout", TYPE_INTEGER, &i, NULL, 0, NULL);
    setting_add("download-path", TYPE_CHAR, &"", internal, 0, &vb.config.download_dir);
    i = 2000;
    setting_add("history-max-items", TYPE_INTEGER, &i, internal, 0, &vb.config.history_max);
    setting_add("editor-command", TYPE_CHAR, &"x-terminal-emulator -e -vi '%s'", NULL, 0, NULL);
    setting_add("header", TYPE_CHAR, &"", headers, FLAG_LIST|FLAG_NODUP, NULL);
#ifdef FEATURE_ARH
    setting_add("auto-response-header", TYPE_CHAR, &"", autoresponseheader, FLAG_LIST|FLAG_NODUP, NULL);
#endif
    setting_add("nextpattern", TYPE_CHAR, &"/\\bnext\\b/i,/^(>\\|>>\\|»)$/,/^(>\\|>>\\|»)/,/(>\\|>>\\|»)$/,/\\bmore\\b/i", prevnext, FLAG_LIST|FLAG_NODUP, NULL);
    setting_add("previouspattern", TYPE_CHAR, &"/\\bprev\\|previous\\b/i,/^(<\\|<<\\|«)$/,/^(<\\|<<\\|«)/,/(<\\|<<\\|«)$/", prevnext, FLAG_LIST|FLAG_NODUP, NULL);
    setting_add("fullscreen", TYPE_BOOLEAN, &off, fullscreen, 0, NULL);
    setting_add("download-command", TYPE_CHAR, &"/bin/sh -c \"curl -sLJOC - -A '$VIMB_USER_AGENT' -e '$VIMB_URI' -b '$VIMB_COOKIES' '%s'\"", NULL, 0, NULL);
    setting_add("download-use-external", TYPE_BOOLEAN, &off, NULL, 0, NULL);
#ifdef FEATURE_HSTS
    setting_add("hsts", TYPE_BOOLEAN, &on, hsts, 0, NULL);
#endif
#ifdef FEATURE_SOUP_CACHE
    i = 2000;
    setting_add("maximum-cache-size", TYPE_INTEGER, &i, soup_cache, 0, NULL);
#endif
    setting_add("x-hint-command", TYPE_CHAR, &":o <C-R>;", NULL, 0, NULL);

    /* initialize the shortcuts and set the default shortcuts */
    shortcut_init();
    shortcut_add("dl", "https://duckduckgo.com/html/?q=$0");
    shortcut_add("dd", "https://duckduckgo.com/?q=$0");
    shortcut_set_default("dl");

    /* initialize the handlers */
    handlers_init();
    handler_add("magnet", "xdg-open '%s'");
}

gboolean setting_run(char *name, const char *param)
{
    SettingType type = SETTING_SET;
    char modifier;
    int res, len;

    /* determine the type to names last char and param */
    len      = strlen(name);
    modifier = name[len - 1];
    if (modifier == '?') {
        name[len - 1] = '\0';
        type          = SETTING_GET;
    } else if (modifier == '+') {
        name[len - 1] = '\0';
        type          = SETTING_APPEND;
    } else if (modifier == '^') {
        name[len - 1] = '\0';
        type          = SETTING_PREPEND;
    } else if (modifier == '-') {
        name[len - 1] = '\0';
        type          = SETTING_REMOVE;
    } else if (modifier == '!') {
        name[len - 1] = '\0';
        type          = SETTING_TOGGLE;
    } else if (!param) {
        type = SETTING_GET;
    }

    /* lookup a matching setting */
    Setting *s = g_hash_table_lookup(vb.config.settings, name);
    if (!s) {
        vb_echo(VB_MSG_ERROR, true, "Config '%s' not found", name);
        return false;
    }

    if (type == SETTING_GET) {
        setting_print(s);
        return true;
    }

    if (type == SETTING_TOGGLE) {
        if (s->type != TYPE_BOOLEAN) {
            vb_echo(VB_MSG_ERROR, true, "Could not toggle none boolean %s", s->name);
            return false;
        }
        gboolean value = !s->value.b;
        res = setting_set_value(s, &value, SETTING_SET);
        setting_print(s);
    } else {
        if (!param) {
            vb_echo(VB_MSG_ERROR, true, "No valid value");

            return false;
        }

        /* convert sting value into internal used data type */
        gboolean boolvar;
        int intvar;
        switch (s->type) {
            case TYPE_BOOLEAN:
                boolvar = g_ascii_strncasecmp(param, "true", 4) == 0
                    || g_ascii_strncasecmp(param, "on", 2) == 0;
                res = setting_set_value(s, &boolvar, type);
                break;

            case TYPE_INTEGER:
                intvar = g_ascii_strtoull(param, (char**)NULL, 10);
                res = setting_set_value(s, &intvar, type);
                break;

            default:
                res = setting_set_value(s, (void*)param, type);
                break;
        }
    }
    if (res == SETTING_OK || res & SETTING_USER_NOTIFIED) {
        return true;
    }

    vb_echo(VB_MSG_ERROR, true, "Could not set %s", s->name);
    return false;
}

gboolean setting_fill_completion(GtkListStore *store, const char *input)
{
    GList *src = g_hash_table_get_keys(vb.config.settings);
    gboolean found = util_fill_completion(store, input, src);
    g_list_free(src);

    return found;
}

void setting_cleanup(void)
{
    if (vb.config.settings) {
        g_hash_table_destroy(vb.config.settings);
    }
    shortcut_cleanup();
    handlers_cleanup();
}

static int setting_set_value(Setting *prop, void *value, SettingType type)
{
    int res = SETTING_OK;
    /* by default given value is also the new value */
    void *newvalue = NULL;
    gboolean free_newvalue;

    /* get prepared value according to setting type */
    free_newvalue = prepare_setting_value(prop, value, type, &newvalue);

    /* if there is a setter defined - call this first to check if the value is
     * accepted */
    if (prop->setter) {
        res = prop->setter(prop->name, prop->type, newvalue, prop->data);
        /* break here on error and don't change the setting */
        if (res & SETTING_ERROR) {
            goto free;
        }
    }

    /* save the new value also in the setting */
    switch (prop->type) {
        case TYPE_BOOLEAN:
            prop->value.b = *((gboolean*)newvalue);
            break;

        case TYPE_INTEGER:
            prop->value.i = *((int*)newvalue);
            break;

        default:
            OVERWRITE_STRING(prop->value.s, newvalue);
            break;
    }

free:
    if (free_newvalue) {
        g_free(newvalue);
    }
    return res;
}

/**
 * Prepares the value for the setting for the different setting types.
 * Return value true indicates that the memory of newvalue must be freed by
 * the caller.
 */
static gboolean prepare_setting_value(Setting *prop, void *value, SettingType type, void **newvalue)
{
    gboolean islist, res = false;
    int vlen, i = 0;
    char *p = NULL;

    if ((type != SETTING_APPEND && type != SETTING_PREPEND && type != SETTING_REMOVE)
        || prop->type == TYPE_BOOLEAN
    ) {
        /* if type is not append, prepend or remove there is nothing to be done */
        *newvalue = value;
        return res;
    }

    /* perform arithmetic operation for integer values */
    if (prop->type == TYPE_INTEGER) {
        int *newint = g_malloc(sizeof(int));
        res         = true;
        if (type == SETTING_APPEND) {
            *newint = prop->value.i + *((int*)value);
        } else if (type == SETTING_PREPEND) {
            *newint = prop->value.i * *((int*)value);
        } else if (type == SETTING_REMOVE) {
            *newint = prop->value.i - *((int*)value);
        }
        *newvalue = (void*)newint;
        return res;
    }

    /* handle operations on currently empty value */
    if (!*prop->value.s) {
        if (type == SETTING_APPEND || type == SETTING_PREPEND) {
            *newvalue = value;
        } else {
            *newvalue = prop->value.s;
        }
        return res;
    }

    islist = (prop->flags & FLAG_LIST);
    vlen   = strlen((char*)value);

    /* check if value already exists in current set option */
    if (type == SETTING_REMOVE || prop->flags & FLAG_NODUP) {
        for (p = prop->value.s; *p; p++) {
            if ((!islist || p == prop->value.s || (p[-1] == ','))
                && strncmp(p, value, vlen) == 0
                && (!islist || p[vlen] == ',' || p[vlen] == '\0')
            ) {
                i = vlen;
                if (islist) {
                    if (p == prop->value.s) {
                        /* include the comma after the matched string */
                        if (p[vlen] == ',') {
                            i++;
                        }
                    } else {
                        /* include the comma before the string */
                        p--;
                        i++;
                    }
                }
                break;
            }
        }

        /* do not add the value if it already exists */
        if ((type == SETTING_APPEND || type == SETTING_PREPEND) && *p) {
            *newvalue = value;
            return res;
        }
    }

    if (type == SETTING_APPEND) {
        if (islist && *(char*)value) {
            /* don't append a comma if the value is empty */
            *newvalue = g_strconcat(prop->value.s, ",", value, NULL);
        } else {
            *newvalue = g_strconcat(prop->value.s, value, NULL);
        }
        res = true;
    } else if (type == SETTING_PREPEND) {
        if (islist && *(char*)value) {
            /* don't prepend a comma if the value is empty */
            *newvalue = g_strconcat(value, ",", prop->value.s, NULL);
        } else {
            *newvalue = g_strconcat(value, prop->value.s, NULL);
        }
        res = true;
    } else if (type == SETTING_REMOVE && p) {
        char *copy = g_strdup(prop->value.s);
        /* make p to point to the same position in the copy */
        p = copy + (p - prop->value.s);

        memmove(p, p + i, 1 + strlen(p + vlen));
        *newvalue = copy;
        res = true;
    }

    return res;
}

static gboolean setting_add(const char *name, int type, void *value,
    SettingFunction setter, int flags, void *data)
{
    Setting *prop = g_slice_new0(Setting);
    prop->name   = name;
    prop->type   = type;
    prop->setter = setter;
    prop->flags  = flags;
    prop->data   = data;

    setting_set_value(prop, value, SETTING_SET);

    g_hash_table_insert(vb.config.settings, (char*)name, prop);
    return true;
}

static void setting_print(Setting *s)
{
    switch (s->type) {
        case TYPE_BOOLEAN:
            vb_echo(VB_MSG_NORMAL, false, "  %s=%s", s->name, s->value.b ? "true" : "false");
            break;

        case TYPE_INTEGER:
            vb_echo(VB_MSG_NORMAL, false, "  %s=%d", s->name, s->value.i);
            break;

        default:
            vb_echo(VB_MSG_NORMAL, false, "  %s=%s", s->name, s->value.s);
            break;
    }
}

static void setting_free(Setting *s)
{
    if (s->type == TYPE_CHAR || s->type == TYPE_COLOR || s->type == TYPE_FONT) {
        g_free(s->value.s);
    }
    g_slice_free(Setting, s);
}

static int webkit(const char *name, int type, void *value, void *data)
{
    const char *property = (const char*)data;
    WebKitWebSettings *web_setting = webkit_web_view_get_settings(vb.gui.webview);

    switch (type) {
        case TYPE_BOOLEAN:
            g_object_set(G_OBJECT(web_setting), property, *((gboolean*)value), NULL);
            break;

        case TYPE_INTEGER:
            g_object_set(G_OBJECT(web_setting), property, *((int*)value), NULL);
            break;

        default:
            g_object_set(G_OBJECT(web_setting), property, (char*)value, NULL);
            break;
    }
    return SETTING_OK;
}

static int pagecache(const char *name, int type, void *value, void *data)
{
    int res;
    gboolean on = *((gboolean*)value);

    /* first set the setting on the web settings */
    res = webkit(name, type, value, data);

    if (res == SETTING_OK && on) {
        webkit_set_cache_model(WEBKIT_CACHE_MODEL_WEB_BROWSER);
    } else {
        /* reduce memory usage if caching is not used */
        webkit_set_cache_model(WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER);
    }

    return res;
}

static int soup(const char *name, int type, void *value, void *data)
{
    const char *property = (const char*)data;
    switch (type) {
        case TYPE_BOOLEAN:
            g_object_set(G_OBJECT(vb.session), property, *((gboolean*)value), NULL);
            break;

        case TYPE_INTEGER:
            g_object_set(G_OBJECT(vb.session), property, *((int*)value), NULL);
            break;

        default:
            g_object_set(G_OBJECT(vb.session), property, (char*)value, NULL);
            break;
    }
    return SETTING_OK;
}

static int internal(const char *name, int type, void *value, void *data)
{
    char **str;
    switch (type) {
        case TYPE_BOOLEAN:
            *(gboolean*)data = *(gboolean*)value;
            break;

        case TYPE_INTEGER:
            *(int*)data = *(int*)value;
            break;

        default:
            str = (char**)data;
            OVERWRITE_STRING(*str, (char*)value);
            break;
    }
    return SETTING_OK;
}

static int input_autohide(const char *name, int type, void *value, void *data)
{
    char *text;
    /* save selected value in internal variable */
    *(gboolean*)data = *(gboolean*)value;

    /* if autohide is on and inputbox contains no text - hide it now */
    if (*(gboolean*)value) {
        text = vb_get_input_text();
        if (!*text) {
            gtk_widget_set_visible(GTK_WIDGET(vb.gui.input), false);
        }
        g_free(text);
    } else {
        /* autohide is off - make sure the input box is shown */
        gtk_widget_set_visible(GTK_WIDGET(vb.gui.input), true);
    }

    return SETTING_OK;
}

static int input_color(const char *name, int type, void *value, void *data)
{
    VB_COLOR_PARSE((VbColor*)data, (char*)value);
    vb_update_input_style();

    return SETTING_OK;
}

static int statusbar(const char *name, int type, void *value, void *data)
{
    gtk_widget_set_visible(GTK_WIDGET(vb.gui.statusbar.box), *(gboolean*)value);

    return SETTING_OK;
}

static int status_color(const char *name, int type, void *value, void *data)
{
    VB_COLOR_PARSE((VbColor*)data, (char*)value);
    vb_update_status_style();

    return SETTING_OK;
}

static int input_font(const char *name, int type, void *value, void *data)
{
    PangoFontDescription **font = (PangoFontDescription**)data;
    if (*font) {
        /* free previous font description */
        pango_font_description_free(*font);
    }
    *font = pango_font_description_from_string((char*)value);
    vb_update_input_style();

    return SETTING_OK;
}

static int status_font(const char *name, int type, void *value, void *data)
{
    PangoFontDescription **font = (PangoFontDescription**)data;
    if (*font) {
        /* free previous font description */
        pango_font_description_free(*font);
    }
    *font = pango_font_description_from_string((char*)value);
    vb_update_status_style();

    return SETTING_OK;
}

#ifdef FEATURE_COOKIE
static int cookie_accept(const char *name, int type, void *value, void *data)
{
    char *policy = (char*)value;
    int i;
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

    for (i = 0; i < LENGTH(map); i++) {
        if (!strcmp(map[i].name, policy)) {
            g_object_set(jar, SOUP_COOKIE_JAR_ACCEPT_POLICY, map[i].policy, NULL);

            return SETTING_OK;
        }
    }
    vb_echo(VB_MSG_ERROR, true, "%s must be in [always, origin, never]", name);

    return SETTING_ERROR | SETTING_USER_NOTIFIED;
}
#endif

static int ca_bundle(const char *name, int type, void *value, void *data)
{
    char *expanded;
    GError *error = NULL;
    /* expand the given file and set it to the file database */
    expanded         = util_expand((char*)value, UTIL_EXP_TILDE|UTIL_EXP_DOLLAR);
    vb.config.tls_db = g_tls_file_database_new(expanded, &error);
    g_free(expanded);
    if (error) {
        g_warning("Could not load ssl database '%s': %s", (char*)value, error->message);
        g_error_free(error);

        return SETTING_ERROR;
    }

    /* there is no function to get the file back from tls file database so
     * it's saved as separate configuration */
    g_object_set(vb.session, "tls-database", vb.config.tls_db, NULL);

    return SETTING_OK;
}


static int proxy(const char *name, int type, void *value, void *data)
{
    gboolean enabled = *(gboolean*)value;
#if SOUP_CHECK_VERSION(2, 42, 2)
    GProxyResolver *proxy = NULL;
#else
    SoupURI *proxy = NULL;
#endif

    if (enabled) {
        const char *http_proxy = g_getenv("http_proxy");

        if (http_proxy != NULL && *http_proxy != '\0') {
            char *proxy_new = strstr(http_proxy, "://")
                ? g_strdup(http_proxy)
                : g_strconcat("http://", http_proxy, NULL);

#if SOUP_CHECK_VERSION(2, 42, 2)
            const char *no_proxy;
            char **ignored_hosts = NULL;
            /* check for no_proxy environment variable that contains comma
             * separated domains or ip addresses to skip from proxy */
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

static int user_style(const char *name, int type, void *value, void *data)
{
    gboolean enabled = *(gboolean*)value;
    WebKitWebSettings *web_setting = webkit_web_view_get_settings(vb.gui.webview);

    if (enabled) {
        char *uri = g_strconcat("file://", vb.files[FILES_USER_STYLE], NULL);
        g_object_set(web_setting, "user-stylesheet-uri", uri, NULL);
        g_free(uri);
    } else {
        g_object_set(web_setting, "user-stylesheet-uri", NULL, NULL);
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
static int headers(const char *name, int type, void *value, void *data)
{
    /* remove previous parsed headers */
    if (vb.config.headers) {
        soup_header_free_param_list(vb.config.headers);
        vb.config.headers = NULL;
    }
    vb.config.headers = soup_header_parse_param_list((char*)value);

    return SETTING_OK;
}

#ifdef FEATURE_ARH
static int autoresponseheader(const char *name, int type, void *value, void *data)
{
    const char *error = NULL;

    GSList *new = arh_parse((char *)value, &error);

    if (! error) {
        /* remove previous parsed headers */
        arh_free(vb.config.autoresponseheader);

        /* add the new one */
        vb.config.autoresponseheader = new;

        return SETTING_OK;

    } else {
        vb_echo(VB_MSG_ERROR, true, "auto-response-header: %s", error);
        return SETTING_ERROR | SETTING_USER_NOTIFIED;
    }
}
#endif

static int prevnext(const char *name, int type, void *value, void *data)
{
    if (validate_js_regexp_list((char*)value)) {
        if (*name == 'n') {
            OVERWRITE_STRING(vb.config.nextpattern, (char*)value);
        } else {
            OVERWRITE_STRING(vb.config.prevpattern, (char*)value);
        }
        return SETTING_OK;
    }

    return SETTING_ERROR | SETTING_USER_NOTIFIED;
}

static int fullscreen(const char *name, int type, void *value, void *data)
{
    if (*(gboolean*)value) {
        gtk_window_fullscreen(GTK_WINDOW(vb.gui.window));
    } else {
        gtk_window_unfullscreen(GTK_WINDOW(vb.gui.window));
    }

    return SETTING_OK;
}

#ifdef FEATURE_HSTS
static int hsts(const char *name, int type, void *value, void *data)
{
    if (*(gboolean*)value) {
        soup_session_add_feature(vb.session, SOUP_SESSION_FEATURE(vb.config.hsts_provider));
    } else {
        soup_session_remove_feature(vb.session, SOUP_SESSION_FEATURE(vb.config.hsts_provider));
    }
    return SETTING_OK;
}
#endif

#ifdef FEATURE_SOUP_CACHE
static int soup_cache(const char *name, int type, void *value, void *data)
{
    int kilobytes = *(int*)value;

    soup_cache_set_max_size(vb.config.soup_cache, kilobytes * 1000);

    /* clear the cache if maximum-cache-size is set to zero - note that this
     * will also effect other vimb instances */
    if (!kilobytes) {
        soup_cache_clear(vb.config.soup_cache);
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
    WebKitWebFrame *frame = webkit_web_view_get_main_frame(vb.gui.webview);

    js     = g_strdup_printf("var i;for(i=0;i<[%s].length;i++);", pattern);
    result = js_eval(webkit_web_frame_get_global_context(frame), js, NULL, &value);
    g_free(js);

    if (!result) {
        vb_echo(VB_MSG_ERROR, true, "%s", value);
    }
    g_free(value);
    return result;
}
