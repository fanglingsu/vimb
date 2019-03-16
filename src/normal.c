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

#include <gdk/gdkkeysyms.h>
#include <string.h>

#include "ascii.h"
#include "command.h"
#include "config.h"
#include "hints.h"
#include "ext-proxy.h"
#include "main.h"
#include "normal.h"
#include "scripts/scripts.h"
#include "util.h"
#include "ext-proxy.h"

typedef enum {
    PHASE_START,
    PHASE_KEY2,
    PHASE_KEY3,
    PHASE_REG,
    PHASE_COMPLETE,
} Phase;

typedef struct NormalCmdInfo_s {
    int count;   /* count used for the command */
    char key;    /* command key */
    char key2;   /* second command key (optional) */
    char key3;   /* third command key only for hinting */
    char reg;    /* char for the yank register */
    Phase phase; /* current parsing phase */
} NormalCmdInfo;

static NormalCmdInfo info = {0, '\0', '\0', PHASE_START};

typedef VbResult (*NormalCommand)(Client *c, const NormalCmdInfo *info);

static VbResult normal_clear_input(Client *c, const NormalCmdInfo *info);
static VbResult normal_descent(Client *c, const NormalCmdInfo *info);
static VbResult normal_ex(Client *c, const NormalCmdInfo *info);
static VbResult normal_fire(Client *c, const NormalCmdInfo *info);
static VbResult normal_g_cmd(Client *c, const NormalCmdInfo *info);
static VbResult normal_hint(Client *c, const NormalCmdInfo *info);
static VbResult normal_do_hint(Client *c, const char *prompt);
static VbResult normal_increment_decrement(Client *c, const NormalCmdInfo *info);
static VbResult normal_input_open(Client *c, const NormalCmdInfo *info);
static VbResult normal_mark(Client *c, const NormalCmdInfo *info);
static VbResult normal_navigate(Client *c, const NormalCmdInfo *info);
static VbResult normal_open_clipboard(Client *c, const NormalCmdInfo *info);
static VbResult normal_open(Client *c, const NormalCmdInfo *info);
static VbResult normal_pass(Client *c, const NormalCmdInfo *info);
static VbResult normal_prevnext(Client *c, const NormalCmdInfo *info);
static VbResult normal_queue(Client *c, const NormalCmdInfo *info);
static VbResult normal_quit(Client *c, const NormalCmdInfo *info);
static VbResult normal_scroll(Client *c, const NormalCmdInfo *info);
static VbResult normal_search(Client *c, const NormalCmdInfo *info);
static VbResult normal_search_selection(Client *c, const NormalCmdInfo *info);
static VbResult normal_view_inspector(Client *c, const NormalCmdInfo *info);
static VbResult normal_view_source(Client *c, const NormalCmdInfo *info);
static void normal_view_source_loaded(WebKitWebResource *resource, GAsyncResult *res, Client *c);
static VbResult normal_yank(Client *c, const NormalCmdInfo *info);
static VbResult normal_zoom(Client *c, const NormalCmdInfo *info);

static struct {
    NormalCommand func;
} commands[] = {
/* NUL 0x00 */ {NULL},
/* ^A  0x01 */ {normal_increment_decrement},
/* ^B  0x02 */ {normal_scroll},
/* ^C  0x03 */ {normal_navigate},
/* ^D  0x04 */ {normal_scroll},
/* ^E  0x05 */ {NULL},
/* ^F  0x06 */ {normal_scroll},
/* ^G  0x07 */ {NULL},
/* ^H  0x08 */ {NULL},
/* ^I  0x09 */ {normal_navigate},
/* ^J  0x0a */ {NULL},
/* ^K  0x0b */ {NULL},
/* ^L  0x0c */ {NULL},
/* ^M  0x0d */ {normal_fire},
/* ^N  0x0e */ {NULL},
/* ^O  0x0f */ {normal_navigate},
/* ^P  0x10 */ {normal_queue},
/* ^Q  0x11 */ {normal_quit},
/* ^R  0x12 */ {NULL},
/* ^S  0x13 */ {NULL},
/* ^T  0x14 */ {NULL},
/* ^U  0x15 */ {normal_scroll},
/* ^V  0x16 */ {NULL},
/* ^W  0x17 */ {NULL},
/* ^X  0x18 */ {normal_increment_decrement},
/* ^Y  0x19 */ {NULL},
/* ^Z  0x1a */ {normal_pass},
/* ^[  0x1b */ {normal_clear_input},
/* ^\  0x1c */ {NULL},
/* ^]  0x1d */ {NULL},
/* ^^  0x1e */ {NULL},
/* ^_  0x1f */ {NULL},
/* SPC 0x20 */ {NULL},
/* !   0x21 */ {NULL},
/* "   0x22 */ {NULL},
/* #   0x23 */ {normal_search_selection},
/* $   0x24 */ {normal_scroll},
/* %   0x25 */ {NULL},
/* &   0x26 */ {NULL},
/* '   0x27 */ {normal_mark},
/* (   0x28 */ {NULL},
/* )   0x29 */ {NULL},
/* *   0x2a */ {normal_search_selection},
/* +   0x2b */ {NULL},
/* ,   0x2c */ {NULL},
/* -   0x2d */ {NULL},
/* .   0x2e */ {NULL},
/* /   0x2f */ {normal_ex},
/* 0   0x30 */ {normal_scroll},
/* 1   0x31 */ {NULL},
/* 2   0x32 */ {NULL},
/* 3   0x33 */ {NULL},
/* 4   0x34 */ {NULL},
/* 5   0x35 */ {NULL},
/* 6   0x36 */ {NULL},
/* 7   0x37 */ {NULL},
/* 8   0x38 */ {NULL},
/* 9   0x39 */ {NULL},
/* :   0x3a */ {normal_ex},
/* ;   0x3b */ {normal_hint},
/* <   0x3c */ {NULL},
/* =   0x3d */ {NULL},
/* >   0x3e */ {NULL},
/* ?   0x3f */ {normal_ex},
/* @   0x40 */ {NULL},
/* A   0x41 */ {NULL},
/* B   0x42 */ {NULL},
/* C   0x43 */ {NULL},
/* D   0x44 */ {NULL},
/* E   0x45 */ {NULL},
/* F   0x46 */ {normal_ex},
/* G   0x47 */ {normal_scroll},
/* H   0x48 */ {NULL},
/* I   0x49 */ {NULL},
/* J   0x4a */ {NULL},
/* K   0x4b */ {NULL},
/* L   0x4c */ {NULL},
/* M   0x4d */ {NULL},
/* N   0x4e */ {normal_search},
/* O   0x4f */ {normal_input_open},
/* P   0x50 */ {normal_open_clipboard},
/* Q   0x51 */ {NULL},
/* R   0x52 */ {normal_navigate},
/* S   0x53 */ {NULL},
/* T   0x54 */ {normal_input_open},
/* U   0x55 */ {normal_open},
/* V   0x56 */ {NULL},
/* W   0x57 */ {NULL},
/* X   0x58 */ {NULL},
/* Y   0x59 */ {normal_yank},
/* Z   0x5a */ {NULL},
/* [   0x5b */ {normal_prevnext},
/* \   0x5c */ {NULL},
/* ]   0x5d */ {normal_prevnext},
/* ^   0x5e */ {NULL},
/* _   0x5f */ {NULL},
/* `   0x60 */ {NULL},
/* a   0x61 */ {NULL},
/* b   0x62 */ {NULL},
/* c   0x63 */ {NULL},
/* d   0x64 */ {NULL},
/* e   0x65 */ {NULL},
/* f   0x66 */ {normal_ex},
/* g   0x67 */ {normal_g_cmd},
/* h   0x68 */ {normal_scroll},
/* i   0x69 */ {NULL},
/* j   0x6a */ {normal_scroll},
/* k   0x6b */ {normal_scroll},
/* l   0x6c */ {normal_scroll},
/* m   0x6d */ {normal_mark},
/* n   0x6e */ {normal_search},
/* o   0x6f */ {normal_input_open},
/* p   0x70 */ {normal_open_clipboard},
/* q   0x71 */ {NULL},
/* r   0x72 */ {normal_navigate},
/* s   0x73 */ {NULL},
/* t   0x74 */ {normal_input_open},
/* u   0x75 */ {normal_open},
/* v   0x76 */ {NULL},
/* w   0x77 */ {NULL},
/* x   0x78 */ {NULL},
/* y   0x79 */ {normal_yank},
/* z   0x7a */ {normal_zoom},
/* {   0x7b */ {NULL},
/* |   0x7c */ {NULL},
/* }   0x7d */ {NULL},
/* ~   0x7e */ {NULL},
/* DEL 0x7f */ {NULL},
};

extern struct Vimb vb;

/**
 * Function called when vimb enters the normal mode.
 */
void normal_enter(Client *c)
{
    /* Make sure that when the browser area becomes visible, it will get mouse
     * and keyboard events */
    gtk_widget_grab_focus(GTK_WIDGET(c->webview));
    hints_clear(c);
}

/**
 * Called when the normal mode is left.
 */
void normal_leave(Client *c)
{
    command_search(c, &((Arg){0, NULL}), FALSE);
}

/**
 * Handles the keypress events from webview and inputbox.
 */
VbResult normal_keypress(Client *c, int key)
{
    VbResult res;

    switch (info.phase) {
        case PHASE_START:
            if (info.count == 0 && key == '0') {
                info.key   = key;
                info.phase = PHASE_COMPLETE;
            } else if (VB_IS_DIGIT(key)) {
                info.count = info.count * 10 + key - '0';
            } else if (strchr(";zg[]'m", key)) {
                /* handle commands that needs additional char */
                info.phase      = PHASE_KEY2;
                info.key        = key;
                c->mode->flags |= FLAG_NOMAP;
            } else if (key == '"') {
                info.phase      = PHASE_REG;
                c->mode->flags |= FLAG_NOMAP;
            } else {
                info.key   = key;
                info.phase = PHASE_COMPLETE;
            }
            break;

        case PHASE_KEY2:
            info.key2 = key;

            /* hinting g; mode requires a third key */
            if (info.key == 'g' && info.key2 == ';') {
                info.phase      = PHASE_KEY3;
                c->mode->flags |= FLAG_NOMAP;
            } else {
                info.phase = PHASE_COMPLETE;
            }
            break;

        case PHASE_KEY3:
            info.key3  = key;
            info.phase = PHASE_COMPLETE;
            break;

        case PHASE_REG:
            if (strchr(REG_CHARS, key)) {
                info.reg   = key;
                info.phase = PHASE_START;
            } else {
                info.phase = PHASE_COMPLETE;
            }
            break;

        case PHASE_COMPLETE:
            break;
    }

    if (info.phase == PHASE_COMPLETE) {
        /* TODO allow more commands - some that are looked up via command key
         * direct and those that are searched via binary search */
        if ((guchar)info.key <= LENGTH(commands) && commands[(guchar)info.key].func) {
            res = commands[(guchar)info.key].func(c, &info);
        } else {
            /* let gtk handle the keyevent if we have no command attached to it */
            c->state.processed_key = FALSE;
            res = RESULT_COMPLETE;
        }

        /* unset the info */
        info.key = info.key2 = info.key3 = info.count = info.reg = 0;
        info.phase = PHASE_START;
    } else {
        res = RESULT_MORE;
    }

    return res;
}

/**
 * Function called when vimb enters the passthrough mode.
 */
void pass_enter(Client *c)
{
    vb_modelabel_update(c, "-- PASS THROUGH --");
}

/**
 * Called when passthrough mode is left.
 */
void pass_leave(Client *c)
{
    ext_proxy_eval_script(c, "document.activeElement.blur();", NULL);
    vb_modelabel_update(c, "");
}

VbResult pass_keypress(Client *c, int key)
{
    if (key == CTRL('[')) { /* esc */
        vb_enter(c, 'n');
    }
    c->state.processed_key = FALSE;
    return RESULT_COMPLETE;
}

static VbResult normal_clear_input(Client *c, const NormalCmdInfo *info)
{
    /* If an element requested the fullscreen - the <esc> shoudl cause to
     * leave this fullscreen mode. */
    if (c->state.is_fullscreen) {
        /* Unset the processed_key to indicate that the <esc> was not handled
         * by us and letting the event bubble up. So that webkit handles the
         * key for us to leave the fullscreen mode. */
        c->state.processed_key = FALSE;
        return RESULT_COMPLETE;
    }

    gtk_widget_grab_focus(GTK_WIDGET(c->webview));

    /* Clear the inputbox and change the style to normal to reset also the
     * possible colored error background. */
    vb_echo(c, MSG_NORMAL, FALSE, "");

    /* Unset search highlightning. */
    command_search(c, &((Arg){0, NULL}), FALSE);

    return RESULT_COMPLETE;
}

static VbResult normal_descent(Client *c, const NormalCmdInfo *info)
{
    int count = info->count ? info->count : 1;
    const char *uri, *p = NULL, *domain = NULL;

    uri = c->state.uri;

    /* get domain part */
    if (!uri || !*uri
        || !(domain = strstr(uri, "://"))
        || !(domain = strchr(domain + 3, '/'))
    ) {
        return RESULT_ERROR;
    }

    switch (info->key2) {
        case 'U':
            p = domain;
            break;

        case 'u':
            /* start at the end */
            p = uri + strlen(uri);
            /* if last char is / increment count to step over this first */
            if (*(p - 1) == '/') {
                count++;
            }
            for (int i = 0; i < count; i++) {
                while (*(p--) != '/') {
                    if (p == uri) {
                        /* reach the beginning */
                        return RESULT_ERROR;
                    }
                }
            }
            /* keep the last / in uri */
            p++;
            break;
    }

    /* if the url is shorter than the domain use the domain instead */
    if (p < domain) {
        p = domain;
    }

    Arg a = {TARGET_CURRENT};
    a.s   = g_strndup(uri, p - uri + 1);

    vb_load_uri(c, &a);
    g_free(a.s);

    return RESULT_COMPLETE;
}

static VbResult normal_ex(Client *c, const NormalCmdInfo *info)
{
    if (info->key == 'F') {
        vb_enter_prompt(c, 'c', ";t", TRUE);
    } else if (info->key == 'f') {
        vb_enter_prompt(c, 'c', ";o", TRUE);
    } else {
        char prompt[2] = {info->key, '\0'};
        vb_enter_prompt(c, 'c', prompt, TRUE);
    }

    return RESULT_COMPLETE;
}

static VbResult normal_fire(Client *c, const NormalCmdInfo *info)
{
    /* If searching is currently active - click link containing current search
     * highlight. We use the search_matches as indicator that the searching is
     * active. */
    if (c->state.search.active) {
        ext_proxy_eval_script(c, "getSelection().anchorNode.parentNode.click();", NULL);

        return RESULT_COMPLETE;
    }

    return RESULT_ERROR;
}

static VbResult normal_g_cmd(Client *c, const NormalCmdInfo *info)
{
    Arg a;
    switch (info->key2) {
        case ';': {
            const char prompt[4] = {'g', ';', info->key3, 0};

            return normal_do_hint(c, prompt);
        }

        case 'F':
            return normal_view_inspector(c, info);

        case 'f':
            return normal_view_source(c, info);

        case 'g':
            return normal_scroll(c, info);

        case 'H':
        case 'h':
            a.i = info->key2 == 'H' ? TARGET_NEW : TARGET_CURRENT;
            a.s = NULL;
            vb_load_uri(c, &a);
            return RESULT_COMPLETE;

        case 'i':
            ext_proxy_focus_input(c);
            return RESULT_COMPLETE;

        case 'U':
        case 'u':
            return normal_descent(c, info);
    }

    return RESULT_ERROR;
}

static VbResult normal_hint(Client *c, const NormalCmdInfo *info)
{
    const char prompt[3] = {info->key, info->key2, 0};

    /* Save the current register char to make it available in case of ;y
     * hinting. This is only a hack, because we don't need this state variable
     * somewhere else - it's only use is for hinting. It might be better to
     * allow to set various data to the mode itself to avoid toggling
     * variables in global skope. */
    c->state.current_register = info->reg;
    return normal_do_hint(c, prompt);
}

static VbResult normal_do_hint(Client *c, const char *prompt)
{
    /* TODO check if the prompt is of a valid hint mode */

    vb_enter_prompt(c, 'c', prompt, TRUE);
    return RESULT_COMPLETE;
}

static VbResult normal_increment_decrement(Client *c, const NormalCmdInfo *info)
{
    char *js;
    int count = info->count ? info->count : 1;

    js = g_strdup_printf("VimbToplevel.incrementUriNumber(%d);", info->key == CTRL('A') ? count : -count);
    ext_proxy_eval_script(c, js, NULL);
    g_free(js);

    return RESULT_COMPLETE;
}

static VbResult normal_input_open(Client *c, const NormalCmdInfo *info)
{
    if (strchr("ot", info->key)) {
        vb_echo(c, MSG_NORMAL, FALSE,
                ":%s ", info->key == 't' ? "tabopen" : "open");
    } else {
        vb_echo(c, MSG_NORMAL, FALSE,
                ":%s %s", info->key == 'T' ? "tabopen" : "open", c->state.uri);
    }
    /* switch mode after setting the input text to not trigger the
     * commands modes input change handler */
    vb_enter_prompt(c, 'c', ":", FALSE);

    return RESULT_COMPLETE;
}

static VbResult normal_mark(Client *c, const NormalCmdInfo *info)
{
    glong current;
    char *js, *mark;
    int idx;

    /* check if the second char is a valid mark char */
    if (!(mark = strchr(MARK_CHARS, info->key2))) {
        return RESULT_ERROR;
    }

    /* get the index of the mark char */
    idx = mark - MARK_CHARS;

    if ('m' == info->key) {
        c->state.marks[idx] = c->state.scroll_top;
    } else {
        /* check if the mark was set */
        if ((int)(c->state.marks[idx] - .5) < 0) {
            return RESULT_ERROR;
        }

        current = c->state.scroll_top;

        /* jump to the location */
        js = g_strdup_printf("window.scroll(window.screenLeft,%ld);", c->state.marks[idx]);
        ext_proxy_eval_script(c, js, NULL);
        g_free(js);

        /* save previous adjust as last position */
        c->state.marks[MARK_TICK] = current;
    }

    return RESULT_COMPLETE;
}

static VbResult normal_navigate(Client *c, const NormalCmdInfo *info)
{
    int count;
    WebKitBackForwardList *list;
    WebKitBackForwardListItem *item;

    WebKitWebView *view = c->webview;
    switch (info->key) {
        case CTRL('I'): /* fall through */
        case CTRL('O'):
            count = info->count ? info->count : 1;
            if (info->key == CTRL('O')) {
                if (webkit_web_view_can_go_back(view)) {
                    list = webkit_web_view_get_back_forward_list(view);
                    item = webkit_back_forward_list_get_nth_item(list, -1 * count);
                    webkit_web_view_go_to_back_forward_list_item(view, item);
                }
            } else {
                if (webkit_web_view_can_go_forward(view)) {
                    list = webkit_web_view_get_back_forward_list(view);
                    item = webkit_back_forward_list_get_nth_item(list, count);
                    webkit_web_view_go_to_back_forward_list_item(view, item);
                }
            }
            break;

        case 'r':
            webkit_web_view_reload(view);
            break;

        case 'R':
            webkit_web_view_reload_bypass_cache(view);
            break;

        case CTRL('C'):
            webkit_web_view_stop_loading(view);
            break;
    }

    return RESULT_COMPLETE;
}

static VbResult normal_open_clipboard(Client *c, const NormalCmdInfo *info)
{
    Arg a = {info->key == 'P' ? TARGET_NEW : TARGET_CURRENT};

    /* if register is not the default - read out of the internal register */
    if (info->reg) {
        a.s = g_strdup(vb_register_get(c, info->reg));
    } else {
        /* if no register is given use the system clipboard */
        a.s = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY));
        if (!a.s) {
            a.s = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_NONE));
        }
    }

    if (a.s) {
        vb_load_uri(c, &a);
        g_free(a.s);

        return RESULT_COMPLETE;
    }

    return RESULT_ERROR;
}

/**
 * Open the last closed page.
 */
static VbResult normal_open(Client *c, const NormalCmdInfo *info)
{
    Arg a;
    if (!vb.files[FILES_CLOSED]) {
        return RESULT_ERROR;
    }

    a.i = info->key == 'U' ? TARGET_NEW : TARGET_CURRENT;
    a.s = util_file_pop_line(vb.files[FILES_CLOSED], NULL);
    if (!a.s) {
        return RESULT_ERROR;
    }

    vb_load_uri(c, &a);
    g_free(a.s);

    return RESULT_COMPLETE;
}

static VbResult normal_pass(Client *c, const NormalCmdInfo *info)
{
    vb_enter(c, 'p');
    return RESULT_COMPLETE;
}

static VbResult normal_prevnext(Client *c, const NormalCmdInfo *info)
{
#if 0 /* TODO implement outside of hints.js */
    int count = info->count ? info->count : 1;
    if (info->key2 == ']') {
        hints_follow_link(FALSE, count);
    } else if (info->key2 == '[') {
        hints_follow_link(TRUE, count);
    } else {
        return RESULT_ERROR;
    }
#endif
    return RESULT_COMPLETE;
}

static VbResult normal_queue(Client *c, const NormalCmdInfo *info)
{
#ifdef FEATURE_QUEUE
    command_queue(c, &((Arg){COMMAND_QUEUE_POP}));
#endif
    return RESULT_COMPLETE;
}

static VbResult normal_quit(Client *c, const NormalCmdInfo *info)
{
    vb_quit(c, FALSE);
    return RESULT_COMPLETE;
}

static VbResult normal_scroll(Client *c, const NormalCmdInfo *info)
{
    char *js;

    js = g_strdup_printf("VimbToplevel.scroll('%c',%d,%d);", info->key, c->config.scrollstep, info->count);
    ext_proxy_eval_script(c, js, NULL);
    g_free(js);

    return RESULT_COMPLETE;
}

static VbResult normal_search(Client *c, const NormalCmdInfo *info)
{
    int count = (info->count > 0) ? info->count : 1;

    command_search(c, &((Arg){info->key == 'n' ? count : -count, NULL}), FALSE);

    return RESULT_COMPLETE;
}

static VbResult normal_search_selection(Client *c, const NormalCmdInfo *info)
{
    int count;
    char *query;

    /* there is no function to get the selected text so we copy current
     * selection to clipboard */
    webkit_web_view_execute_editing_command(c->webview, WEBKIT_EDITING_COMMAND_COPY);
    query = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY));
    if (!query) {
        return RESULT_ERROR;
    }
    count = (info->count > 0) ? info->count : 1;

    command_search(c, &((Arg){info->key == '*' ? count : -count, query}), TRUE);
    g_free(query);

    return RESULT_COMPLETE;
}

static VbResult normal_view_inspector(Client *c, const NormalCmdInfo *info)
{
    WebKitSettings *settings;

    settings = webkit_web_view_get_settings(c->webview);

    /* Try to get the inspected uri to identify if the inspector is shown at
     * the time or not. */
    if (webkit_web_inspector_is_attached(c->inspector)) {
        webkit_web_inspector_close(c->inspector);
    } else if (webkit_settings_get_enable_developer_extras(settings)) {
        webkit_web_inspector_show(c->inspector);
    } else {
        /* Inform the user on attempt to enable webinspector when the
         * developer extra are not enabled. */
        vb_echo(c, MSG_ERROR, TRUE, "webinspector is not enabled");

        return RESULT_ERROR;
    }
    return RESULT_COMPLETE;
}

static VbResult normal_view_source(Client *c, const NormalCmdInfo *info)
{
    WebKitWebResource *resource;

    if ((resource = webkit_web_view_get_main_resource(c->webview)) == NULL) {
        return RESULT_ERROR;
    }

    webkit_web_resource_get_data(resource, NULL,
            (GAsyncReadyCallback)normal_view_source_loaded, c);

    return RESULT_COMPLETE;
}

static void normal_view_source_loaded(WebKitWebResource *resource,
    GAsyncResult *res, Client *c)
{
    gsize length;
    guchar *data = NULL;
    char *text = NULL;

    data = webkit_web_resource_get_data_finish(resource, res, &length, NULL);
    if (data) {
        text = g_strndup((gchar*)data, length);
        command_spawn_editor(c, &((Arg){0, (char *)text}), NULL, NULL);
        g_free(data);
        g_free(text);
    }
}

static VbResult normal_yank(Client *c, const NormalCmdInfo *info)
{
    Arg a = {info->key == 'Y' ? COMMAND_YANK_SELECTION : COMMAND_YANK_URI};

    return command_yank(c, &a, info->reg) ? RESULT_COMPLETE : RESULT_ERROR;
}

static VbResult normal_zoom(Client *c, const NormalCmdInfo *info)
{
    float step = 0.1, level, count;
    WebKitWebView *view = c->webview;

    /* check if the second key is allowed */
    if (!strchr("iIoOz", info->key2)) {
        return RESULT_ERROR;
    }

    count = info->count ? (float)info->count : 1.0;

    /* zz reset zoom to it's default zoom level */
    if (info->key2 == 'z') {
        webkit_settings_set_zoom_text_only(webkit_web_view_get_settings(view), FALSE);
        webkit_web_view_set_zoom_level(view, c->config.default_zoom / 100.0);

        return RESULT_COMPLETE;
    }

    level= webkit_web_view_get_zoom_level(view);

    /* calculate the new zoom level */
    if (info->key2 == 'i' || info->key2 == 'I') {
        level += ((float)count * step);
    } else {
        level -= ((float)count * step);
    }

    /* apply the new zoom level */
    webkit_settings_set_zoom_text_only(webkit_web_view_get_settings(view), VB_IS_LOWER(info->key2));
    webkit_web_view_set_zoom_level(view, level);

    return RESULT_COMPLETE;
}
