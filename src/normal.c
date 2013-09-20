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
#include "command.h"
#include "hints.h"
#include "dom.h"
#include "history.h"
#include "util.h"

typedef enum {
    PHASE_START,
    PHASE_AFTERG,
    PHASE_KEY2,
    PHASE_COMPLETE,
} Phase;

struct NormalCmdInfo_s {
    int count;          /* count used for the command */
    unsigned char cmd;  /* command key */
    unsigned char arg;  /* argument key if this is used */
    Phase phase;        /* current parsing phase */
} info = {0, '\0', '\0', PHASE_START};

typedef VbResult (*NormalCommand)(const NormalCmdInfo *info);

static VbResult normal_clear_input(const NormalCmdInfo *info);
static VbResult normal_descent(const NormalCmdInfo *info);
static VbResult normal_ex(const NormalCmdInfo *info);
static VbResult normal_focus_input(const NormalCmdInfo *info);
static VbResult normal_hint(const NormalCmdInfo *info);
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
/* NUL  */ {NULL},
/* ^A   */ {NULL},
/* ^B   */ {normal_scroll},
/* ^C   */ {NULL},
/* ^D   */ {normal_scroll},
/* ^E   */ {NULL},
/* ^F   */ {normal_scroll},
/* ^G   */ {NULL},
/* ^H   */ {NULL},
/* ^I   */ {normal_navigate},
/* ^J   */ {NULL},
/* ^K   */ {NULL},
/* ^L   */ {NULL},
/* ^M   */ {NULL},
/* ^N   */ {NULL},
/* ^O   */ {normal_navigate},
/* ^P   */ {normal_queue},
/* ^Q   */ {normal_quit},
/* ^R   */ {NULL},
/* ^S   */ {NULL},
/* ^T   */ {NULL},
/* ^U   */ {normal_scroll},
/* ^V   */ {NULL},
/* ^W   */ {NULL},
/* ^X   */ {NULL},
/* ^Y   */ {NULL},
/* ^Z   */ {normal_pass},
/* ^[   */ {normal_clear_input},
/* ^\   */ {NULL},
/* ^]   */ {NULL},
/* ^^   */ {NULL},
/* ^_   */ {NULL},
/* SPC  */ {NULL},
/* !    */ {NULL},
/* "    */ {NULL},
/* #    */ {normal_search_selection},
/* $    */ {normal_scroll},
/* %    */ {NULL},
/* &    */ {NULL},
/* '    */ {NULL},
/* (    */ {NULL},
/* )    */ {NULL},
/* *    */ {normal_search_selection},
/* +    */ {NULL},
/* ,    */ {NULL},
/* -    */ {NULL},
/* .    */ {NULL},
/* /    */ {normal_ex},
/* 0    */ {normal_scroll},
/* 1    */ {NULL},
/* 2    */ {NULL},
/* 3    */ {NULL},
/* 4    */ {NULL},
/* 5    */ {NULL},
/* 6    */ {NULL},
/* 7    */ {NULL},
/* 8    */ {NULL},
/* 9    */ {NULL},
/* :    */ {normal_ex},
/* ;    */ {normal_hint},
/* <    */ {NULL},
/* =    */ {NULL},
/* >    */ {NULL},
/* ?    */ {normal_ex},
/* @    */ {NULL},
/* A    */ {NULL},
/* B    */ {NULL},
/* C    */ {NULL},
/* D    */ {NULL},
/* E    */ {NULL},
/* F    */ {normal_ex},
/* G    */ {normal_scroll},
/* H    */ {NULL},
/* I    */ {NULL},
/* J    */ {NULL},
/* K    */ {NULL},
/* L    */ {NULL},
/* M    */ {NULL},
/* N    */ {normal_search},
/* O    */ {NULL},
/* P    */ {normal_open_clipboard},
/* Q    */ {NULL},
/* R    */ {normal_navigate},
/* S    */ {NULL},
/* T    */ {NULL},
/* U    */ {normal_open},
/* V    */ {NULL},
/* W    */ {NULL},
/* X    */ {NULL},
/* Y    */ {normal_yank},
/* Z    */ {NULL},
/* [    */ {NULL},
/* \    */ {NULL},
/* ]    */ {NULL},
/* ^    */ {NULL},
/* _    */ {NULL},
/* `    */ {NULL},
/* a    */ {NULL},
/* b    */ {NULL},
/* c    */ {NULL},
/* d    */ {NULL},
/* e    */ {NULL},
/* f    */ {normal_ex},
/* g    */ {NULL},
/* h    */ {normal_scroll},
/* i    */ {NULL},
/* j    */ {normal_scroll},
/* k    */ {normal_scroll},
/* l    */ {normal_scroll},
/* m    */ {NULL},
/* n    */ {normal_search},
/* o    */ {normal_ex},
/* p    */ {normal_open_clipboard},
/* q    */ {NULL},
/* r    */ {normal_navigate},
/* s    */ {NULL},
/* t    */ {normal_ex},
/* u    */ {normal_open},
/* v    */ {NULL},
/* w    */ {NULL},
/* x    */ {NULL},
/* y    */ {normal_yank},
/* z    */ {normal_zoom}, /* arg chars are handled in normal_keypress */
/* {    */ {NULL},
/* |    */ {NULL},
/* }    */ {NULL},
/* ~    */ {NULL},
/* DEL  */ {NULL},
/***************/
/* gNUL */ {NULL},
/* g^A  */ {NULL},
/* g^B  */ {NULL},
/* g^C  */ {NULL},
/* g^D  */ {NULL},
/* g^E  */ {NULL},
/* g^F  */ {NULL},
/* g^G  */ {NULL},
/* g^H  */ {NULL},
/* g^I  */ {NULL},
/* g^J  */ {NULL},
/* g^K  */ {NULL},
/* g^L  */ {NULL},
/* g^M  */ {NULL},
/* g^N  */ {NULL},
/* g^O  */ {NULL},
/* g^P  */ {NULL},
/* g^Q  */ {NULL},
/* g^R  */ {NULL},
/* g^S  */ {NULL},
/* g^T  */ {NULL},
/* g^U  */ {NULL},
/* g^V  */ {NULL},
/* g^W  */ {NULL},
/* g^X  */ {NULL},
/* g^Y  */ {NULL},
/* g^Z  */ {NULL},
/* gESC */ {NULL}, /* used as Control Sequence Introducer */
/* g^\  */ {NULL},
/* g^]  */ {NULL},
/* g^^  */ {NULL},
/* g^_  */ {NULL},
/* gSPC */ {NULL},
/* g!   */ {NULL},
/* g"   */ {NULL},
/* g#   */ {NULL},
/* g$   */ {NULL},
/* g%   */ {NULL},
/* g&   */ {NULL},
/* g'   */ {NULL},
/* g(   */ {NULL},
/* g)   */ {NULL},
/* g*   */ {NULL},
/* g+   */ {NULL},
/* g,   */ {NULL},
/* g-   */ {NULL},
/* g.   */ {NULL},
/* g/   */ {NULL},
/* g0   */ {NULL},
/* g1   */ {NULL},
/* g2   */ {NULL},
/* g3   */ {NULL},
/* g4   */ {NULL},
/* g5   */ {NULL},
/* g6   */ {NULL},
/* g7   */ {NULL},
/* g8   */ {NULL},
/* g9   */ {NULL},
/* g:   */ {NULL},
/* g;   */ {NULL},
/* g<   */ {NULL},
/* g=   */ {NULL},
/* g>   */ {NULL},
/* g?   */ {NULL},
/* g@   */ {NULL},
/* gA   */ {NULL},
/* gB   */ {NULL},
/* gC   */ {NULL},
/* gD   */ {NULL},
/* gE   */ {NULL},
/* gF   */ {normal_view_inspector},
/* gG   */ {NULL},
/* gH   */ {normal_open},
/* gI   */ {NULL},
/* gJ   */ {NULL},
/* gK   */ {NULL},
/* gL   */ {NULL},
/* gM   */ {NULL},
/* gN   */ {NULL},
/* gO   */ {NULL},
/* gP   */ {NULL},
/* gQ   */ {NULL},
/* gR   */ {NULL},
/* gS   */ {NULL},
/* gT   */ {NULL},
/* gU   */ {normal_descent},
/* gV   */ {NULL},
/* gW   */ {NULL},
/* gX   */ {NULL},
/* gY   */ {NULL},
/* gZ   */ {NULL},
/* g[   */ {NULL},
/* g\   */ {NULL},
/* g]   */ {NULL},
/* g^   */ {NULL},
/* g_   */ {NULL},
/* g`   */ {NULL},
/* ga   */ {NULL},
/* gb   */ {NULL},
/* gc   */ {NULL},
/* gd   */ {NULL},
/* ge   */ {NULL},
/* gf   */ {normal_view_source},
/* gg   */ {normal_scroll},
/* gh   */ {normal_open},
/* gi   */ {normal_focus_input},
/* gj   */ {NULL},
/* gk   */ {NULL},
/* gl   */ {NULL},
/* gm   */ {NULL},
/* gn   */ {NULL},
/* go   */ {NULL},
/* gp   */ {NULL},
/* gq   */ {NULL},
/* gr   */ {NULL},
/* gs   */ {NULL},
/* gt   */ {NULL},
/* gu   */ {normal_descent},
/* gv   */ {NULL},
/* gw   */ {NULL},
/* gx   */ {NULL},
/* gy   */ {NULL},
/* gz   */ {NULL},
/* g{   */ {NULL},
/* g|   */ {NULL},
/* g}   */ {NULL},
/* g~   */ {NULL},
/* gDEL */ {NULL}
};

extern VbCore vb;


/**
 * Function called when vimb enters the normal mode.
 */
void normal_enter(void)
{
    dom_clear_focus(vb.gui.webview);
    gtk_widget_grab_focus(GTK_WIDGET(vb.gui.webview));
    history_rewind();
    hints_clear();
}

/**
 * Called when the normal mode is left.
 */
void normal_leave(void)
{
    /* TODO clean those only if they where active */
    command_search(&((Arg){COMMAND_SEARCH_OFF}));
}

/**
 * Handles the keypress events from webview and inputbox.
 */
VbResult normal_keypress(unsigned int key)
{
    State *s = &vb.state;
    VbResult res;
    if (info.phase == PHASE_START && key == 'g') {
        info.phase = PHASE_AFTERG;
        vb.mode->flags |= FLAG_NOMAP;

        return RESULT_MORE;
    }

    if (info.phase == PHASE_AFTERG) {
        key        = G_CMD(key);
        info.phase = PHASE_START;
    }

    if (info.phase == PHASE_START && info.count == 0 && key == '0') {
        info.cmd   = key;
        info.phase = PHASE_COMPLETE;
    } else if (info.phase == PHASE_KEY2) {
        info.arg = key;
        info.phase = PHASE_COMPLETE;
    } else if (info.phase == PHASE_START && isdigit(key)) {
        info.count = info.count * 10 + key - '0';
    } else if (strchr(";z", (char)key)) {
        /* handle commands that needs additional char */
        info.phase = PHASE_KEY2;
        info.cmd   = key;
        vb.mode->flags |= FLAG_NOMAP;
    } else {
        info.cmd   = key & 0xff;
        info.phase = PHASE_COMPLETE;
    }

    if (info.phase == PHASE_COMPLETE) {
        if (commands[info.cmd].func) {
            res = commands[info.cmd].func(&info);
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
        info.cmd = info.arg = info.count = 0;
        info.phase = PHASE_START;
    }
    return res;
}

/**
 * Handles changes in the inputbox. If there are usergenerated changes, we
 * switch to command mode.
 */
void normal_input_changed(const char *text)
{
    /*mode_enter('i');*/
}

static VbResult normal_clear_input(const NormalCmdInfo *info)
{
    vb_set_input_text("");
    gtk_widget_grab_focus(GTK_WIDGET(vb.gui.webview));
    /* TODO implement the search new - so we can remove the command.c file */
    command_search(&((Arg){COMMAND_SEARCH_OFF}));
    /* check flags on the current state to only clear hints if they where
     * enabled before */
    /*hints_clear();*/

    return RESULT_COMPLETE;
}

static VbResult normal_descent(const NormalCmdInfo *info)
{
    int count = info->count ? info->count : 1;
    const char *uri, *p = NULL, *domain = NULL;

    uri = GET_URI();

    /* get domain part */
    if (!uri || *uri == '\0'
        || !(domain = strstr(uri, "://"))
        || !(domain = strchr(domain + 3, '/'))
    ) {
        return RESULT_ERROR;
    }

    switch (UNG_CMD(info->cmd)) {
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
    char prompt[2] = {0};

    /* Handle some hardwired mapping here instead of using map_insert. This
     * makes the binding imutable and we can simply use f, F, o and t in
     * mapped keys too */
    switch (info->cmd) {
        case 'o':
            vb_set_input_text(":open ");
            /* switch mode after setting the input text to not trigger the
             * commands modes input change handler */
            mode_enter('c');
            break;

        case 't':
            vb_set_input_text(":tabopen ");
            mode_enter('c');
            break;

        case 'f':
            /* switch the mode first so that the input change handler of
             * command mode will recognize the ; to switch to hinting submode */
            mode_enter('c');
            vb_set_input_text(";o");
            break;

        case 'F':
            mode_enter('c');
            vb_set_input_text(";t");
            break;
        
        default:
            prompt[0] = info->cmd;
            vb_set_input_text(prompt);
            mode_enter('c');
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

static VbResult normal_hint(const NormalCmdInfo *info)
{
    char prompt[3] = {info->cmd, info->arg, 0};
#ifdef FEATURE_QUEUE
    const char *allowed = "eiIoOpPstTy";
#else
    const char *allowed = "eiIoOstTy";
#endif

    /* check if this is a valid hint mode */
    if (!info->arg || !strchr(allowed, info->arg)) {
        return RESULT_ERROR;
    }

    mode_enter('c');
    vb_set_input_text(prompt);
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

        case 'C': /* TODO shouldn't we use ^C instead? */
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
    if (strchr("uU", info->cmd)) { /* open last closed */
        a.i = info->cmd == 'U' ? VB_TARGET_NEW : VB_TARGET_CURRENT;
        a.s = util_get_file_contents(vb.files[FILES_CLOSED], NULL);
        vb_load_uri(&a);
        g_free(a.s);
    } else { /* open home page */
        a.i = UNG_CMD(info->cmd) == 'H' ? VB_TARGET_NEW : VB_TARGET_CURRENT;
        a.s = NULL;
        vb_load_uri(&a);
    }

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

    /* TODO is this required? */
    /*vb_set_mode(VB_MODE_NORMAL | (vb.state.mode & VB_MODE_SEARCH), false);*/

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
        case G_CMD('g'):
            adjust = vb.gui.adjust_v;
            max    = gtk_adjustment_get_upper(adjust) - gtk_adjustment_get_page_size(adjust);
            new    = info->count ? (max * info->count / 100) : gtk_adjustment_get_lower(adjust);
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
            return RESULT_ERROR;
    }
    max = gtk_adjustment_get_upper(adjust) - gtk_adjustment_get_page_size(adjust);
    gtk_adjustment_set_value(adjust, new > max ? max : new);

    return RESULT_COMPLETE;
}

static VbResult normal_search(const NormalCmdInfo *info)
{
    command_search(&((Arg){info->cmd == 'n' ? COMMAND_SEARCH_FORWARD : COMMAND_SEARCH_BACKWARD}));
    return RESULT_COMPLETE;
}

static VbResult normal_search_selection(const NormalCmdInfo *info)
{
    /* there is no function to get the selected text so we copy current
     * selection to clipboard */
    webkit_web_view_copy_clipboard(vb.gui.webview);
    char *query = gtk_clipboard_wait_for_text(PRIMARY_CLIPBOARD());
    if (!query) {
        return RESULT_ERROR;
    }

    command_search(&((Arg){info->cmd == '*' ? COMMAND_SEARCH_FORWARD : COMMAND_SEARCH_BACKWARD, query}));
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

    if (info->arg == 'z') { /* zz reset zoom */
        webkit_web_view_set_zoom_level(view, 1.0);

        return RESULT_COMPLETE;
    }

    level   = webkit_web_view_get_zoom_level(view);
    setting = webkit_web_view_get_settings(view);
    g_object_get(G_OBJECT(setting), "zoom-step", &step, NULL);

    webkit_web_view_set_full_content_zoom(view, isupper(info->arg));

    /* calculate the new zoom level */
    if (info->arg == 'i' || info->arg == 'I') {
        level += ((float)count * step);
    } else {
        level -= ((float)count * step);
    }

    /* apply the new zoom level */
    webkit_web_view_set_zoom_level(view, level);

    return RESULT_COMPLETE;
}
