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

#include <gdk/gdkkeysyms.h>
#include <ctype.h>
#include "config.h"
#include "mode.h"
#include "main.h"
#include "normal.h"
#include "ascii.h"
#include "command.h"
#include "hints.h"
#include "dom.h"
#include "history.h"
#include "util.h"

/* convert the lower 4 bits of byte n to its hex character */
#define NR2HEX(n)   (n & 0xf) <= 9 ? (n & 0xf) + '0' : (c & 0xf) - 10 + 'a'

typedef enum {
    PHASE_START,
    PHASE_KEY2,
    PHASE_COMPLETE,
} Phase;

struct NormalCmdInfo_s {
    int count;   /* count used for the command */
    char cmd;    /* command key */
    char ncmd;   /* second command key (optional) */
    Phase phase; /* current parsing phase */
} info = {0, '\0', '\0', PHASE_START};

typedef VbResult (*NormalCommand)(const NormalCmdInfo *info);

static char *transchar(int c);

static VbResult normal_clear_input(const NormalCmdInfo *info);
static VbResult normal_descent(const NormalCmdInfo *info);
static VbResult normal_ex(const NormalCmdInfo *info);
static VbResult normal_focus_input(const NormalCmdInfo *info);
static VbResult normal_g_cmd(const NormalCmdInfo *info);
static VbResult normal_hint(const NormalCmdInfo *info);
static VbResult normal_input_open(const NormalCmdInfo *info);
static VbResult normal_navigate(const NormalCmdInfo *info);
static VbResult normal_open_clipboard(const NormalCmdInfo *info);
static VbResult normal_open(const NormalCmdInfo *info);
static VbResult normal_pass(const NormalCmdInfo *info);
static VbResult normal_queue(const NormalCmdInfo *info);
static VbResult normal_quit(const NormalCmdInfo *info);
static VbResult normal_scroll(const NormalCmdInfo *info);
static VbResult normal_search(const NormalCmdInfo *info);
static VbResult normal_search_selection(const NormalCmdInfo *info);
static VbResult normal_view_inspector(const NormalCmdInfo *info);
static VbResult normal_view_source(const NormalCmdInfo *info);
static VbResult normal_yank(const NormalCmdInfo *info);
static VbResult normal_zoom(const NormalCmdInfo *info);

static struct {
    NormalCommand func;
} commands[] = {
/* NUL 0x00 */ {NULL},
/* ^A  0x01 */ {NULL},
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
/* ^M  0x0d */ {NULL},
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
/* ^X  0x18 */ {NULL},
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
/* '   0x27 */ {NULL},
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
/* [   0x5b */ {NULL},
/* \   0x5c */ {NULL},
/* ]   0x5d */ {NULL},
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
/* m   0x6d */ {NULL},
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

extern VbCore vb;

static char showcmd_buf[SHOWCMD_LEN + 1];   /* buffer to show ambiguous key sequence */

/**
 * Function called when vimb enters the normal mode.
 */
void normal_enter(void)
{
    dom_clear_focus(vb.gui.webview);
    gtk_widget_grab_focus(GTK_WIDGET(vb.gui.webview));
    hints_clear();
}

/**
 * Called when the normal mode is left.
 */
void normal_leave(void)
{
    /* TODO clean those only if they where active */
    command_search(&((Arg){0}));
}

/**
 * Handles the keypress events from webview and inputbox.
 */
VbResult normal_keypress(int key)
{
    State *s = &vb.state;
    VbResult res;

    if (info.phase == PHASE_START && info.count == 0 && key == '0') {
        info.cmd   = key;
        info.phase = PHASE_COMPLETE;
    } else if (info.phase == PHASE_KEY2) {
        info.ncmd = key;
        info.phase = PHASE_COMPLETE;
    } else if (info.phase == PHASE_START && isdigit(key)) {
        info.count = info.count * 10 + key - '0';
    } else if (strchr(";zg", (char)key)) {
        /* handle commands that needs additional char */
        info.phase = PHASE_KEY2;
        info.cmd   = key;
        vb.mode->flags |= FLAG_NOMAP;
    } else {
        info.cmd   = key;
        info.phase = PHASE_COMPLETE;
    }

    if (info.phase == PHASE_COMPLETE) {
        /* TODO allow more commands - some that are looked up via command key
         * direct and those that are searched via binary search */
        if ((guchar)info.cmd <= LENGTH(commands) && commands[(guchar)info.cmd].func) {
            res = commands[(guchar)info.cmd].func(&info);
        } else {
            /* let gtk handle the keyevent if we have no command attached to
             * it */
            s->processed_key = false;
            res = RESULT_COMPLETE;
        }
    } else {
        res = RESULT_MORE;
    }

    if (res == RESULT_COMPLETE) {
        /* unset the info */
        info.cmd = info.ncmd = info.count = 0;
        info.phase = PHASE_START;
    } else if (res == RESULT_MORE) {
        normal_showcmd(key);
    }
    return res;
}

/**
 * Put the given char onto the show command buffer.
 */
void normal_showcmd(int c)
{
    char *translated;
    int old, extra, overflow;

    if (c) {
        translated = transchar(c);
        old        = strlen(showcmd_buf);
        extra      = strlen(translated);
        overflow   = old + extra - SHOWCMD_LEN;
        if (overflow > 0) {
            g_memmove(showcmd_buf, showcmd_buf + overflow, old - overflow + 1);
        }
        strcat(showcmd_buf, translated);
    } else {
        showcmd_buf[0] = '\0';
    }
    /* show the typed keys */
    gtk_label_set_text(GTK_LABEL(vb.gui.statusbar.cmd), showcmd_buf);
}

/**
 * Transalte a singe char into a readable representation to be show to the
 * user in status bar.
 */
static char *transchar(int c)
{
    static char trans[5];
    int i = 0;
    if (IS_CTRL(c)) {
        trans[i++] = '^';
        trans[i++] = CTRL(c);
    } else if ((unsigned)c >= 0x80) {
        trans[i++] = '<';
        trans[i++] = NR2HEX((unsigned)c >> 4);
        trans[i++] = NR2HEX((unsigned)c);
        trans[i++] = '>';
    } else {
        trans[i++] = c;
    }
    trans[i++] = '\0';

    return trans;
}

static VbResult normal_clear_input(const NormalCmdInfo *info)
{
    vb_set_input_text("");
    gtk_widget_grab_focus(GTK_WIDGET(vb.gui.webview));
    command_search(&((Arg){0}));

    return RESULT_COMPLETE;
}

static VbResult normal_descent(const NormalCmdInfo *info)
{
    int count = info->count ? info->count : 1;
    const char *uri, *p = NULL, *domain = NULL;

    uri = GET_URI();

    /* get domain part */
    if (!uri || !*uri
        || !(domain = strstr(uri, "://"))
        || !(domain = strchr(domain + 3, '/'))
    ) {
        return RESULT_ERROR;
    }

    switch (info->ncmd) {
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

    Arg a = {VB_TARGET_CURRENT};
    a.s   = g_strndup(uri, p - uri + 1);

    vb_load_uri(&a);
    g_free(a.s);

    return RESULT_COMPLETE;
}

static VbResult normal_ex(const NormalCmdInfo *info)
{
    mode_enter('c');
    if (info->cmd == 'F') {
        vb_set_input_text(";t");
    } else if (info->cmd == 'f') {
        vb_set_input_text(";o");
    } else {
        char prompt[2] = {info->cmd, '\0'};
        vb_set_input_text(prompt);
    }

    return RESULT_COMPLETE;
}

static VbResult normal_focus_input(const NormalCmdInfo *info)
{
    if (dom_focus_input(vb.gui.webview)) {
        mode_enter('i');

        return RESULT_COMPLETE;
    }
    return RESULT_ERROR;
}

static VbResult normal_g_cmd(const NormalCmdInfo *info)
{
    Arg a;
    switch (info->ncmd) {
        case 'F':
            return normal_view_inspector(info);

        case 'f':
            normal_view_source(info);

        case 'g':
            return normal_scroll(info);

        case 'H':
        case 'h':
            a.i = info->ncmd == 'H' ? VB_TARGET_NEW : VB_TARGET_CURRENT;
            a.s = NULL;
            vb_load_uri(&a);
            return RESULT_COMPLETE;

        case 'i':
            return normal_focus_input(info);

        case 'U':
        case 'u':
            return normal_descent(info);
    }

    return RESULT_ERROR;
}

static VbResult normal_hint(const NormalCmdInfo *info)
{
    char prompt[3] = {info->cmd, info->ncmd, 0};
#ifdef FEATURE_QUEUE
    const char *allowed = "eiIoOpPstTy";
#else
    const char *allowed = "eiIoOstTy";
#endif

    /* check if this is a valid hint mode */
    if (!info->ncmd || !strchr(allowed, info->ncmd)) {
        return RESULT_ERROR;
    }

    mode_enter('c');
    vb_set_input_text(prompt);
    return RESULT_COMPLETE;
}

static VbResult normal_input_open(const NormalCmdInfo *info)
{
    if (strchr("ot", info->cmd)) {
        vb_set_input_text(info->cmd == 't' ? ":tabopen " : ":open ");
    } else {
        vb_echo(VB_MSG_NORMAL, false, ":%s %s", info->cmd == 'T' ? "tabopen" : "open", GET_URI());
    }
    /* switch mode after setting the input text to not trigger the
     * commands modes input change handler */
    mode_enter('c');

    return RESULT_COMPLETE;
}

static VbResult normal_navigate(const NormalCmdInfo *info)
{
    int count;

    WebKitWebView *view = vb.gui.webview;
    switch (info->cmd) {
        case CTRL('I'): /* fall through */
        case CTRL('O'):
            count = info->count ? info->count : 1;
            if (info->cmd == CTRL('O')) {
                count *= -1;
            }
            webkit_web_view_go_back_or_forward(view, count);
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

static VbResult normal_open_clipboard(const NormalCmdInfo *info)
{
    Arg a = {info->cmd == 'P' ? VB_TARGET_NEW : VB_TARGET_CURRENT};

    a.s = gtk_clipboard_wait_for_text(PRIMARY_CLIPBOARD());
    if (!a.s) {
        a.s = gtk_clipboard_wait_for_text(SECONDARY_CLIPBOARD());
    }

    if (a.s) {
        vb_load_uri(&a);
        g_free(a.s);

        return RESULT_COMPLETE;
    }

    return RESULT_ERROR;
}

static VbResult normal_open(const NormalCmdInfo *info)
{
    Arg a;
    /* open last closed */
    a.i = info->cmd == 'U' ? VB_TARGET_NEW : VB_TARGET_CURRENT;
    a.s = util_get_file_contents(vb.files[FILES_CLOSED], NULL);
    vb_load_uri(&a);
    g_free(a.s);

    return RESULT_COMPLETE;
}

static VbResult normal_pass(const NormalCmdInfo *info)
{
    mode_enter('p');
    return RESULT_COMPLETE;
}

static VbResult normal_queue(const NormalCmdInfo *info)
{
    command_queue(&((Arg){COMMAND_QUEUE_POP}));
    return RESULT_COMPLETE;
}

static VbResult normal_quit(const NormalCmdInfo *info)
{
    vb_quit();
    return RESULT_COMPLETE;
}

static VbResult normal_scroll(const NormalCmdInfo *info)
{
    GtkAdjustment *adjust;
    gdouble value, max, new;
    int count = info->count ? info->count : 1;

    /* TODO split this into more functions - reduce similar code */
    switch (info->cmd) {
        case 'h':
            adjust = vb.gui.adjust_h;
            value  = vb.config.scrollstep;
            new    = gtk_adjustment_get_value(adjust) - value * count;
            break;
        case 'j':
            adjust = vb.gui.adjust_v;
            value  = vb.config.scrollstep;
            new    = gtk_adjustment_get_value(adjust) + value * count;
            break;
        case 'k':
            adjust = vb.gui.adjust_v;
            value  = vb.config.scrollstep;
            new    = gtk_adjustment_get_value(adjust) - value * count;
            break;
        case 'l':
            adjust = vb.gui.adjust_h;
            value  = vb.config.scrollstep;
            new    = gtk_adjustment_get_value(adjust) + value * count;
            break;
        case CTRL('D'):
            adjust = vb.gui.adjust_v;
            value  = gtk_adjustment_get_page_size(adjust) / 2;
            new    = gtk_adjustment_get_value(adjust) + value * count;
            break;
        case CTRL('U'):
            adjust = vb.gui.adjust_v;
            value  = gtk_adjustment_get_page_size(adjust) / 2;
            new    = gtk_adjustment_get_value(adjust) - value * count;
            break;
        case CTRL('F'):
            adjust = vb.gui.adjust_v;
            value  = gtk_adjustment_get_page_size(adjust);
            new    = gtk_adjustment_get_value(adjust) + value * count;
            break;
        case CTRL('B'):
            adjust = vb.gui.adjust_v;
            value  = gtk_adjustment_get_page_size(adjust);
            new    = gtk_adjustment_get_value(adjust) - value * count;
            break;
        case 'G':
            adjust = vb.gui.adjust_v;
            max    = gtk_adjustment_get_upper(adjust) - gtk_adjustment_get_page_size(adjust);
            new    = info->count ? (max * info->count / 100) : gtk_adjustment_get_upper(adjust);
            break;
        case '0':
            adjust = vb.gui.adjust_h;
            new    = gtk_adjustment_get_lower(adjust);
            break;
        case '$':
            adjust = vb.gui.adjust_h;
            new    = gtk_adjustment_get_upper(adjust);
            break;

        default:
            if (info->ncmd == 'g') {
                adjust = vb.gui.adjust_v;
                max    = gtk_adjustment_get_upper(adjust) - gtk_adjustment_get_page_size(adjust);
                new    = info->count ? (max * info->count / 100) : gtk_adjustment_get_lower(adjust);
                break;
            }
            return RESULT_ERROR;
    }
    max = gtk_adjustment_get_upper(adjust) - gtk_adjustment_get_page_size(adjust);
    gtk_adjustment_set_value(adjust, new > max ? max : new);

    return RESULT_COMPLETE;
}

static VbResult normal_search(const NormalCmdInfo *info)
{
    int count = (info->count > 0) ? info->count : 1;

    command_search(&((Arg){info->cmd == 'n' ? count : -count}));
    return RESULT_COMPLETE;
}

static VbResult normal_search_selection(const NormalCmdInfo *info)
{
    int count;
    char *query;

    /* there is no function to get the selected text so we copy current
     * selection to clipboard */
    webkit_web_view_copy_clipboard(vb.gui.webview);
    query = gtk_clipboard_wait_for_text(PRIMARY_CLIPBOARD());
    if (!query) {
        return RESULT_ERROR;
    }
    count = (info->count > 0) ? info->count : 1;

    command_search(&((Arg){info->cmd == '*' ? count : -count}));
    g_free(query);

    return RESULT_COMPLETE;
}

static VbResult normal_view_inspector(const NormalCmdInfo *info)
{
    gboolean enabled;
    WebKitWebSettings *settings = webkit_web_view_get_settings(vb.gui.webview);

    g_object_get(G_OBJECT(settings), "enable-developer-extras", &enabled, NULL);
    if (enabled) {
        if (vb.state.is_inspecting) {
            webkit_web_inspector_close(vb.gui.inspector);
        } else {
            webkit_web_inspector_show(vb.gui.inspector);
        }
        return RESULT_COMPLETE;
    }

    vb_echo(VB_MSG_ERROR, true, "webinspector is not enabled");
    return RESULT_ERROR;
}

static VbResult normal_view_source(const NormalCmdInfo *info)
{
    gboolean mode = webkit_web_view_get_view_source_mode(vb.gui.webview);
    webkit_web_view_set_view_source_mode(vb.gui.webview, !mode);
    webkit_web_view_reload(vb.gui.webview);
    return RESULT_COMPLETE;
}

static VbResult normal_yank(const NormalCmdInfo *info)
{
    Arg a = {info->cmd == 'Y' ? COMMAND_YANK_SELECTION : COMMAND_YANK_URI};

    return command_yank(&a) ? RESULT_COMPLETE : RESULT_ERROR;
}

static VbResult normal_zoom(const NormalCmdInfo *info)
{
    float step, level, count;
    WebKitWebSettings *setting;
    WebKitWebView *view = vb.gui.webview;

    count = info->count ? (float)info->count : 1.0;

    if (info->ncmd == 'z') { /* zz reset zoom */
        webkit_web_view_set_zoom_level(view, 1.0);

        return RESULT_COMPLETE;
    }

    level   = webkit_web_view_get_zoom_level(view);
    setting = webkit_web_view_get_settings(view);
    g_object_get(G_OBJECT(setting), "zoom-step", &step, NULL);

    webkit_web_view_set_full_content_zoom(view, isupper(info->ncmd));

    /* calculate the new zoom level */
    if (info->ncmd == 'i' || info->ncmd == 'I') {
        level += ((float)count * step);
    } else {
        level -= ((float)count * step);
    }

    /* apply the new zoom level */
    webkit_web_view_set_zoom_level(view, level);

    return RESULT_COMPLETE;
}
