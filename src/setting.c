/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2018 Daniel Carl
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

#include <glib.h>
#include <string.h>

#include "../version.h"
#include "completion.h"
#include "config.h"
#include "ext-proxy.h"
#include "main.h"
#include "setting.h"
#include "scripts/scripts.h"
#include "shortcut.h"

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

static int setting_set_value(Client *c, Setting *prop, void *value, SettingType type);
static gboolean prepare_setting_value(Setting *prop, void *value, SettingType type, void **newvalue);
static gboolean setting_add(Client *c, const char *name, DataType type, void *value,
    SettingFunction setter, int flags, void *data);
static void setting_print(Client *c, Setting *s);
static void setting_free(Setting *s);

static int cookie_accept(Client *c, const char *name, DataType type, void *value, void *data);
static int dark_mode(Client *c, const char *name, DataType type, void *value, void *data);
static int default_zoom(Client *c, const char *name, DataType type, void *value, void *data);
static int fullscreen(Client *c, const char *name, DataType type, void *value, void *data);
static int geolocation(Client *c, const char *name, DataType type, void *value, void *data);
static int gui_style(Client *c, const char *name, DataType type, void *value, void *data);
static int hardware_acceleration_policy(Client *c, const char *name, DataType type, void *value, void *data);
static int input_autohide(Client *c, const char *name, DataType type, void *value, void *data);
static int internal(Client *c, const char *name, DataType type, void *value, void *data);
static int notification(Client *c, const char *name, DataType type, void *value, void *data);
static int headers(Client *c, const char *name, DataType type, void *value, void *data);
static int user_scripts(Client *c, const char *name, DataType type, void *value, void *data);
static int user_style(Client *c, const char *name, DataType type, void *value, void *data);
static int statusbar(Client *c, const char *name, DataType type, void *value, void *data);
static int tls_policy(Client *c, const char *name, DataType type, void *value, void *data);
static int webkit(Client *c, const char *name, DataType type, void *value, void *data);
static int webkit_spell_checking(Client *c, const char *name, DataType type, void *value, void *data);
static int webkit_spell_checking_language(Client *c, const char *name, DataType type, void *value, void *data);
static int window_decorate(Client *c, const char *name, DataType type, void *value, void *data);

extern struct Vimb vb;


void setting_init(Client *c)
{
    int i;
    gboolean on = TRUE, off = FALSE;

    c->config.settings = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, (GDestroyNotify)setting_free);
    setting_add(c, "user-agent", TYPE_CHAR, &"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/11.0 Safari/605.1.15 " PROJECT "/" VERSION, webkit, 0, "user-agent");
    /* TODO use the real names for webkit settings */
    i = 14;
    setting_add(c, "accelerated-2d-canvas", TYPE_BOOLEAN, &off, webkit, 0, "enable-accelerated-2d-canvas");
    setting_add(c, "allow-file-access-from-file-urls", TYPE_BOOLEAN, &off, webkit, 0, "allow-file-access-from-file-urls");
    setting_add(c, "allow-universal-access-from-file-urls", TYPE_BOOLEAN, &off, webkit, 0, "allow-universal-access-from-file-urls");
    setting_add(c, "caret", TYPE_BOOLEAN, &off, webkit, 0, "enable-caret-browsing");
    setting_add(c, "cursiv-font", TYPE_CHAR, &"serif", webkit, 0, "cursive-font-family");
    setting_add(c, "dark-mode", TYPE_BOOLEAN, &off, dark_mode, 0, NULL);
    setting_add(c, "default-charset", TYPE_CHAR, &"utf-8", webkit, 0, "default-charset");
    setting_add(c, "default-font", TYPE_CHAR, &"sans-serif", webkit, 0, "default-font-family");
    setting_add(c, "dns-prefetching", TYPE_BOOLEAN, &on, webkit, 0, "enable-dns-prefetching");
    i = SETTING_DEFAULT_FONT_SIZE;
    setting_add(c, "font-size", TYPE_INTEGER, &i, webkit, 0, "default-font-size");
    setting_add(c, "frame-flattening", TYPE_BOOLEAN, &off, webkit, 0, "enable-frame-flattening");
    setting_add(c, "geolocation", TYPE_CHAR, &"ask", geolocation, FLAG_NODUP, NULL);
    setting_add(c, "hardware-acceleration-policy", TYPE_CHAR, &"ondemand", hardware_acceleration_policy, FLAG_NODUP, NULL);
    setting_add(c, "header", TYPE_CHAR, &"", headers, FLAG_LIST|FLAG_NODUP, "header");
    i = 1000;
    setting_add(c, "hint-timeout", TYPE_INTEGER, &i, NULL, 0, NULL);
    setting_add(c, "hint-keys", TYPE_CHAR, &SETTING_HINT_KEYS, NULL, 0, NULL);
    setting_add(c, "hint-follow-last", TYPE_BOOLEAN, &on, NULL, 0, NULL);
    setting_add(c, "hint-keys-same-length", TYPE_BOOLEAN, &off, NULL, 0, NULL);
    setting_add(c, "hint-match-element", TYPE_BOOLEAN, &on, NULL, 0, NULL);
    setting_add(c, "html5-database", TYPE_BOOLEAN, &on, webkit, 0, "enable-html5-database");
    setting_add(c, "html5-local-storage", TYPE_BOOLEAN, &on, webkit, 0, "enable-html5-local-storage");
    setting_add(c, "hyperlink-auditing", TYPE_BOOLEAN, &off, webkit, 0, "enable-hyperlink-auditing");
    setting_add(c, "images", TYPE_BOOLEAN, &on, webkit, 0, "auto-load-images");
    setting_add(c, "javascript-can-access-clipboard", TYPE_BOOLEAN, &off, webkit, 0, "javascript-can-access-clipboard");
    setting_add(c, "javascript-can-open-windows-automatically", TYPE_BOOLEAN, &off, webkit, 0, "javascript-can-open-windows-automatically");
#if WEBKIT_CHECK_VERSION(2, 24, 0)
    setting_add(c, "javascript-enable-markup", TYPE_BOOLEAN, &on, webkit, 0, "enable-javascript-markup");
#endif
    setting_add(c, "media-playback-allows-inline", TYPE_BOOLEAN, &on, webkit, 0, "media-playback-allows-inline");
    setting_add(c, "media-playback-requires-user-gesture", TYPE_BOOLEAN, &off, webkit, 0, "media-playback-requires-user-gesture");
    setting_add(c, "media-stream", TYPE_BOOLEAN, &off, webkit, 0, "enable-media-stream");
    setting_add(c, "mediasource", TYPE_BOOLEAN, &off, webkit, 0, "enable-mediasource");
    i = 5;
    setting_add(c, "minimum-font-size", TYPE_INTEGER, &i, webkit, 0, "minimum-font-size");
    setting_add(c, "monospace-font", TYPE_CHAR, &"monospace", webkit, 0, "monospace-font-family");
    i = SETTING_DEFAULT_MONOSPACE_FONT_SIZE;
    setting_add(c, "monospace-font-size", TYPE_INTEGER, &i, webkit, 0, "default-monospace-font-size");
    setting_add(c, "notification", TYPE_CHAR, &"ask", notification, FLAG_NODUP, NULL);
    setting_add(c, "offline-cache", TYPE_BOOLEAN, &on, webkit, 0, "enable-offline-web-application-cache");
    setting_add(c, "plugins", TYPE_BOOLEAN, &on, webkit, 0, "enable-plugins");
    setting_add(c, "prevent-newwindow", TYPE_BOOLEAN, &off, internal, 0, &c->config.prevent_newwindow);
    setting_add(c, "print-backgrounds", TYPE_BOOLEAN, &on, webkit, 0, "print-backgrounds");
    setting_add(c, "sans-serif-font", TYPE_CHAR, &"sans-serif", webkit, 0, "sans-serif-font-family");
    setting_add(c, "scripts", TYPE_BOOLEAN, &on, webkit, 0, "enable-javascript");
    setting_add(c, "serif-font", TYPE_CHAR, &"serif", webkit, 0, "serif-font-family");
    setting_add(c, "site-specific-quirks", TYPE_BOOLEAN, &off, webkit, 0, "enable-site-specific-quirks");
    setting_add(c, "smooth-scrolling", TYPE_BOOLEAN, &off, webkit, 0, "enable-smooth-scrolling");
    setting_add(c, "spatial-navigation", TYPE_BOOLEAN, &off, webkit, 0, "enable-spatial-navigation");
    setting_add(c, "tabs-to-links", TYPE_BOOLEAN, &on, webkit, 0, "enable-tabs-to-links");
    setting_add(c, "webaudio", TYPE_BOOLEAN, &off, webkit, 0, "enable-webaudio");
    setting_add(c, "webgl", TYPE_BOOLEAN, &off, webkit, 0, "enable-webgl");
    setting_add(c, "webinspector", TYPE_BOOLEAN, &on, webkit, 0, "enable-developer-extras");
    setting_add(c, "xss-auditor", TYPE_BOOLEAN, &on, webkit, 0, "enable-xss-auditor");

    /* internal variables */
    setting_add(c, "stylesheet", TYPE_BOOLEAN, &on, user_style, 0, NULL);
    setting_add(c, "user-scripts", TYPE_BOOLEAN, &on, user_scripts, 0, NULL);
    setting_add(c, "cookie-accept", TYPE_CHAR, &SETTING_COOKIE_ACCEPT, cookie_accept, 0, NULL);
    i = 40;
    setting_add(c, "scroll-step", TYPE_INTEGER, &i, internal, 0, &c->config.scrollstep);
    i = 1;
    setting_add(c, "scroll-multiplier", TYPE_INTEGER, &i, internal, 0, &c->config.scrollmultiplier);
    setting_add(c, "home-page", TYPE_CHAR, &SETTING_HOME_PAGE, NULL, 0, NULL);
    i = 2000;
    setting_add(c, "status-bar-show-settings", TYPE_BOOLEAN, &off, internal, 0, &c->config.statusbar_show_settings);
    /* TODO should be global and not overwritten by a new client */
    setting_add(c, "history-max-items", TYPE_INTEGER, &i, internal, 0, &vb.config.history_max);
    setting_add(c, "editor-command", TYPE_CHAR, &"x-terminal-emulator -e -vi '%s'", NULL, 0, NULL);
    setting_add(c, "strict-ssl", TYPE_BOOLEAN, &on, tls_policy, 0, NULL);
    setting_add(c, "status-bar", TYPE_BOOLEAN, &on, statusbar, 0, NULL);
    i = 1000;
    setting_add(c, "timeoutlen", TYPE_INTEGER, &i, internal, 0, &c->map.timeoutlen);
    setting_add(c, "input-autohide", TYPE_BOOLEAN, &off, input_autohide, 0, &c->config.input_autohide);
    setting_add(c, "fullscreen", TYPE_BOOLEAN, &off, fullscreen, 0, NULL);
    setting_add(c, "show-titlebar", TYPE_BOOLEAN, &on, window_decorate, 0, NULL);
    i = 100;
    setting_add(c, "default-zoom", TYPE_INTEGER, &i, default_zoom, 0, NULL);
    setting_add(c, "download-path", TYPE_CHAR, &SETTING_DOWNLOAD_PATH, NULL, 0, NULL);
    setting_add(c, "download-command", TYPE_CHAR, &SETTING_DOWNLOAD_COMMAND, NULL, 0, NULL);
    setting_add(c, "download-use-external", TYPE_BOOLEAN, &off, NULL, 0, NULL);
    setting_add(c, "incsearch", TYPE_BOOLEAN, &off, internal, 0, &c->config.incsearch);
    i = 10;
    /* TODO should be global and not overwritten by a new client */
    setting_add(c, "closed-max-items", TYPE_INTEGER, &i, internal, 0, &vb.config.closed_max);
    setting_add(c, "x-hint-command", TYPE_CHAR, &":o <C-R>;", NULL, 0, NULL);
    setting_add(c, "spell-checking", TYPE_BOOLEAN, &off, webkit_spell_checking, 0, NULL);
    setting_add(c, "spell-checking-languages", TYPE_CHAR, &"en_US", webkit_spell_checking_language, FLAG_LIST|FLAG_NODUP, NULL);

    /* gui style settings vimb */
    setting_add(c, "completion-css", TYPE_CHAR, &SETTING_COMPLETION_CSS, gui_style, 0, NULL);
    setting_add(c, "completion-hover-css", TYPE_CHAR, &SETTING_COMPLETION_HOVER_CSS, gui_style, 0, NULL);
    setting_add(c, "completion-selected-css", TYPE_CHAR, &SETTING_COMPLETION_SELECTED_CSS, gui_style, 0, NULL);
    setting_add(c, "input-css", TYPE_CHAR, &SETTING_INPUT_CSS, gui_style, 0, NULL);
    setting_add(c, "input-error-css", TYPE_CHAR, &SETTING_INPUT_ERROR_CSS, gui_style, 0, NULL);
    setting_add(c, "status-css", TYPE_CHAR, &SETTING_STATUS_CSS, gui_style, 0, NULL);
    setting_add(c, "status-ssl-css", TYPE_CHAR, &SETTING_STATUS_SSL_CSS, gui_style, 0, NULL);
    setting_add(c, "status-ssl-invalid-css", TYPE_CHAR, &SETTING_STATUS_SSL_INVLID_CSS, gui_style, 0, NULL);

    /* initialize the shortcuts and set the default shortcuts */
    shortcut_add(c->config.shortcuts, "dl", "https://duckduckgo.com/html/?q=$0");
    shortcut_add(c->config.shortcuts, "dd", "https://duckduckgo.com/?q=$0");
    shortcut_set_default(c->config.shortcuts, "dl");
}

VbCmdResult setting_run(Client *c, char *name, const char *param)
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
    Setting *s = g_hash_table_lookup(c->config.settings, name);
    if (!s) {
        vb_echo(c, MSG_ERROR, TRUE, "Config '%s' not found", name);
        return CMD_ERROR | CMD_KEEPINPUT;
    }

    if (type == SETTING_GET) {
        setting_print(c, s);
        return CMD_SUCCESS | CMD_KEEPINPUT;
    }

    if (type == SETTING_TOGGLE) {
        if (s->type != TYPE_BOOLEAN) {
            vb_echo(c, MSG_ERROR, TRUE, "Could not toggle none boolean %s", s->name);

            return CMD_ERROR | CMD_KEEPINPUT;
        }
        gboolean value = !s->value.b;
        res = setting_set_value(c, s, &value, SETTING_SET);
        setting_print(c, s);

        /* make sure the new value set by the toggle keep visible */
        res |= CMD_KEEPINPUT;
    } else {
        if (!param) {
            vb_echo(c, MSG_ERROR, TRUE, "No valid value");

            return CMD_ERROR | CMD_KEEPINPUT;
        }

        /* convert sting value into internal used data type */
        gboolean boolvar;
        int intvar;
        switch (s->type) {
            case TYPE_BOOLEAN:
                boolvar = g_ascii_strncasecmp(param, "true", 4) == 0
                    || g_ascii_strncasecmp(param, "on", 2) == 0;
                res = setting_set_value(c, s, &boolvar, type);
                break;

            case TYPE_INTEGER:
                intvar = g_ascii_strtoull(param, (char**)NULL, 10);
                res = setting_set_value(c, s, &intvar, type);
                break;

            default:
                res = setting_set_value(c, s, (void*)param, type);
                break;
        }
    }

    if (res & (CMD_SUCCESS | CMD_KEEPINPUT)) {
        return res;
    }

    vb_echo(c, MSG_ERROR, TRUE, "Could not set %s", s->name);
    return CMD_ERROR | CMD_KEEPINPUT;
}

gboolean setting_fill_completion(Client *c, GtkListStore *store, const char *input)
{
    GtkTreeIter iter;
    gboolean found = FALSE;
    GList *src     = g_hash_table_get_keys(c->config.settings);

    /* If no filter input given - copy all entries into the data store. */
    if (!input || !*input) {
        for (GList *l = src; l; l = l->next) {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, COMPLETION_STORE_FIRST, l->data, -1);
            found = TRUE;
        }
        g_list_free(src);
        return found;
    }

    /* If filter input is given - copy matching list entires into data store.
     * Strings are compared by prefix matching. */
    for (GList *l = src; l; l = l->next) {
        char *value = (char*)l->data;
        if (g_str_has_prefix(value, input)) {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, COMPLETION_STORE_FIRST, l->data, -1);
            found = TRUE;
        }
    }

    g_list_free(src);
    return found;
}

void setting_cleanup(Client *c)
{
    if (c->config.settings) {
        g_hash_table_destroy(c->config.settings);
        c->config.settings = NULL;
    }
}

static int setting_set_value(Client *c, Setting *prop, void *value, SettingType type)
{
    int res = CMD_SUCCESS;
    /* by default given value is also the new value */
    void *newvalue = NULL;
    gboolean free_newvalue;

    /* get prepared value according to setting type */
    free_newvalue = prepare_setting_value(prop, value, type, &newvalue);

    /* if there is a setter defined - call this first to check if the value is
     * accepted */
    if (prop->setter) {
        res = prop->setter(c, prop->name, prop->type, newvalue, prop->data);
        /* break here on error and don't change the setting */
        if (res & CMD_ERROR) {
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
 * Return value TRUE indicates that the memory of newvalue must be freed by
 * the caller.
 */
static gboolean prepare_setting_value(Setting *prop, void *value, SettingType type, void **newvalue)
{
    gboolean islist, res = FALSE;
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
        res         = TRUE;
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
        res = TRUE;
    } else if (type == SETTING_PREPEND) {
        if (islist && *(char*)value) {
            /* don't prepend a comma if the value is empty */
            *newvalue = g_strconcat(value, ",", prop->value.s, NULL);
        } else {
            *newvalue = g_strconcat(value, prop->value.s, NULL);
        }
        res = TRUE;
    } else if (type == SETTING_REMOVE && p) {
        char *copy = g_strdup(prop->value.s);
        /* make p to point to the same position in the copy */
        p = copy + (p - prop->value.s);

        memmove(p, p + i, 1 + strlen(p + vlen));
        *newvalue = copy;
        res = TRUE;
    }

    return res;
}

static gboolean setting_add(Client *c, const char *name, DataType type, void *value,
        SettingFunction setter, int flags, void *data)
{
    Setting *prop = g_slice_new0(Setting);
    prop->name   = name;
    prop->type   = type;
    prop->setter = setter;
    prop->flags  = flags;
    prop->data   = data;

    setting_set_value(c, prop, value, SETTING_SET);

    g_hash_table_insert(c->config.settings, (char*)name, prop);
    return TRUE;
}

static void setting_print(Client *c, Setting *s)
{
    switch (s->type) {
        case TYPE_BOOLEAN:
            vb_echo(c, MSG_NORMAL, FALSE, "  %s=%s", s->name, s->value.b ? "true" : "false");
            break;

        case TYPE_INTEGER:
            vb_echo(c, MSG_NORMAL, FALSE, "  %s=%d", s->name, s->value.i);
            break;

        default:
            vb_echo(c, MSG_NORMAL, FALSE, "  %s=%s", s->name, s->value.s);
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

static int cookie_accept(Client *c, const char *name, DataType type, void *value, void *data)
{
    WebKitWebContext *ctx;
    WebKitCookieManager *cm;
    char *policy = (char*)value;

    ctx = webkit_web_view_get_context(c->webview);
    cm  = webkit_web_context_get_cookie_manager(ctx);
    if (strcmp("always", policy) == 0) {
        webkit_cookie_manager_set_accept_policy(cm, WEBKIT_COOKIE_POLICY_ACCEPT_ALWAYS);
    } else if (strcmp("origin", policy) == 0) {
        webkit_cookie_manager_set_accept_policy(cm, WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY);
    } else if (strcmp("never", policy) == 0) {
        webkit_cookie_manager_set_accept_policy(cm, WEBKIT_COOKIE_POLICY_ACCEPT_NEVER);
    } else {
        vb_echo(c, MSG_ERROR, TRUE, "%s must be in [always, origin, never]", name);

        return CMD_ERROR | CMD_KEEPINPUT;
    }

    return CMD_SUCCESS;
}

static int dark_mode(Client *c, const char *name, DataType type, void *value, void *data)
{
    g_object_set(gtk_widget_get_settings(GTK_WIDGET(c->window)), "gtk-application-prefer-dark-theme", *(gboolean*)value, NULL);

    return CMD_SUCCESS;
}

static int default_zoom(Client *c, const char *name, DataType type, void *value, void *data)
{
    /* Store the percent value in the client config. */
    c->config.default_zoom = *(int*)value;

    /* Apply the default zoom to the webview. */
    webkit_settings_set_zoom_text_only(webkit_web_view_get_settings(c->webview), FALSE);
    webkit_web_view_set_zoom_level(c->webview, c->config.default_zoom / 100.0);

    return CMD_SUCCESS;
}

static int fullscreen(Client *c, const char *name, DataType type, void *value, void *data)
{
    if (*(gboolean*)value) {
        gtk_window_fullscreen(GTK_WINDOW(c->window));
    } else {
        gtk_window_unfullscreen(GTK_WINDOW(c->window));
    }

    return CMD_SUCCESS;
}

static int geolocation(Client *c, const char *name, DataType type, void *value, void *data)
{
    char *policy = (char *)value;
    if (strcmp("always", policy) != 0 && strcmp("ask", policy) != 0 && strcmp("never", policy) != 0) {
        vb_echo(c, MSG_ERROR, FALSE, "%s must be in [always, ask, never]", name);
        return CMD_ERROR | CMD_KEEPINPUT;
    }
    return CMD_SUCCESS;
}

/* This needs to be called before the window is shown for the best chance of
 * success, but it may be called at any time.
 * Currently the setting file is read after the window has been shown, which
 * might mean the titlebar isn't hidden on certain environments. */
static int window_decorate(Client *c, const char *name, DataType type, void *value, void *data)
{
    gtk_window_set_decorated(GTK_WINDOW(c->window), *(gboolean*)value);

    return CMD_SUCCESS;
}

static int hardware_acceleration_policy(Client *c, const char *name, DataType type, void *value, void *data)
{
    WebKitSettings *settings = webkit_web_view_get_settings(c->webview);

    if (g_str_equal(value, "ondemand")) {
        webkit_settings_set_hardware_acceleration_policy(settings, WEBKIT_HARDWARE_ACCELERATION_POLICY_ON_DEMAND);
    } else if (g_str_equal(value, "always")) {
        webkit_settings_set_hardware_acceleration_policy(settings, WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS);
    } else if (g_str_equal(value, "never")) {
        webkit_settings_set_hardware_acceleration_policy(settings, WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER);
    } else {
        vb_echo(c, MSG_ERROR, TRUE, "%s must be in [ondemand, always, never]", name);
        return CMD_ERROR|CMD_KEEPINPUT;
    }

    return CMD_SUCCESS;
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
static int headers(Client *c, const char *name, DataType type, void *value, void *data)
{
    ext_proxy_set_header(c, (char*)value);

    return CMD_SUCCESS;
}

static int input_autohide(Client *c, const char *name, DataType type, void *value, void *data)
{
    char *text;

    /* save selected value in internal variable */
    *(gboolean*)data = *(gboolean*)value;

    /* if autohide is on and inputbox contains no text - hide it now */
    if (*(gboolean*)value) {
        text = vb_input_get_text(c);
        if (!*text) {
            gtk_widget_set_visible(GTK_WIDGET(c->input), FALSE);
        }
        g_free(text);
    } else {
        /* autohide is off - make sure the input box is shown */
        gtk_widget_set_visible(GTK_WIDGET(c->input), TRUE);
    }

    return CMD_SUCCESS;
}

static int internal(Client *c, const char *name, DataType type, void *value, void *data)
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
    return CMD_SUCCESS;
}

static int notification(Client *c, const char *name, DataType type, void *value, void *data)
{
    char *policy = (char *)value;
    if (strcmp("always", policy) != 0 && strcmp("ask", policy) != 0 && strcmp("never", policy) != 0) {
        vb_echo(c, MSG_ERROR, FALSE, "%s must be in [always, ask, never]", name);
        return CMD_ERROR | CMD_KEEPINPUT;
    }
    return CMD_SUCCESS;
}

static int user_scripts(Client *c, const char *name, DataType type, void *value, void *data)
{
    WebKitUserContentManager *ucm;
    WebKitUserScript *script;
    gchar *source;

    gboolean enabled = *(gboolean*)value;

    ucm = webkit_web_view_get_user_content_manager(c->webview);
    webkit_user_content_manager_remove_all_scripts(ucm);

    if (enabled) {
        if (vb.files[FILES_SCRIPT]
                && g_file_get_contents(vb.files[FILES_SCRIPT], &source, NULL, NULL)) {

            script = webkit_user_script_new(
                source, WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
                WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_END, NULL, NULL
            );

            webkit_user_content_manager_add_script(ucm, script);
            webkit_user_script_unref(script);
            g_free(source);
        }
    }

    /* Inject the global scripts. */
    script = webkit_user_script_new(JS_HINTS " " JS_SCROLL,
            WEBKIT_USER_CONTENT_INJECT_TOP_FRAME,
            WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_END, NULL, NULL);
    webkit_user_content_manager_add_script(ucm, script);
    webkit_user_script_unref(script);

    return CMD_SUCCESS;
}

static int user_style(Client *c, const char *name, DataType type, void *value, void *data)
{
    WebKitUserContentManager *ucm;
    WebKitUserStyleSheet *style;
    gchar *source;

    gboolean enabled = *(gboolean*)value;

    ucm = webkit_web_view_get_user_content_manager(c->webview);

    if (enabled) {
        if (g_file_get_contents(vb.files[FILES_USER_STYLE], &source, NULL, NULL)) {
            style = webkit_user_style_sheet_new(
                source, WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
                WEBKIT_USER_STYLE_LEVEL_USER, NULL, NULL
            );

            webkit_user_content_manager_add_style_sheet(ucm, style);
            webkit_user_style_sheet_unref(style);
            g_free(source);
        } else {
            g_message("Could not read style file: %s", vb.files[FILES_USER_STYLE]);
        }
    } else {
        webkit_user_content_manager_remove_all_style_sheets(ucm);
    }

    /* Inject the global styles with author level to allow restyling by user
     * style sheets. */
    style = webkit_user_style_sheet_new(CSS_HINTS,
            WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
            WEBKIT_USER_STYLE_LEVEL_AUTHOR, NULL, NULL);
    webkit_user_content_manager_add_style_sheet(ucm, style);
    webkit_user_style_sheet_unref(style);

    return CMD_SUCCESS;
}

static int statusbar(Client *c, const char *name, DataType type, void *value, void *data)
{
    gtk_widget_set_visible(GTK_WIDGET(c->statusbar.box), *(gboolean*)value);

    return CMD_SUCCESS;
}

static int gui_style(Client *c, const char *name, DataType type, void *value, void *data)
{
    vb_gui_style_update(c, name, (const char*)value);

    return CMD_SUCCESS;
}

static int tls_policy(Client *c, const char *name, DataType type, void *value, void *data)
{
    gboolean strict = *((gboolean*)value);

    webkit_web_context_set_tls_errors_policy(
        webkit_web_context_get_default(),
        strict ? WEBKIT_TLS_ERRORS_POLICY_FAIL : WEBKIT_TLS_ERRORS_POLICY_IGNORE);

    return CMD_SUCCESS;
}

static int webkit(Client *c, const char *name, DataType type, void *value, void *data)
{
    const char *property = (const char*)data;
    WebKitSettings *web_setting = webkit_web_view_get_settings(c->webview);

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
    return CMD_SUCCESS;
}

static int webkit_spell_checking(Client *c, const char *name, DataType type, void *value, void *data)
{
    gboolean enabled = *((gboolean*)value);

    webkit_web_context_set_spell_checking_enabled(
            webkit_web_context_get_default(),
            enabled);

    return CMD_SUCCESS;
}

static int webkit_spell_checking_language(Client *c, const char *name, DataType type, void *value, void *data)
{
    char **languages = g_strsplit((char*)value, ",", -1);

    webkit_web_context_set_spell_checking_languages(
            webkit_web_context_get_default(),
            (const char * const *)languages);
    g_strfreev(languages);

    return CMD_SUCCESS;
}
