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
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkkeysyms-compat.h>
#include "hints.h"
#include "main.h"
#include "ascii.h"
#include "dom.h"
#include "command.h"
#include "hints.js.h"
#include "mode.h"
#include "input.h"
#include "map.h"
#include "js.h"

#define HINT_FILE "hints.js"

static struct {
    JSObjectRef    obj;       /* the js object */
    char           mode;      /* mode identifying char - that last char of the hint prompt */
    int            promptlen; /* length of the hint prompt chars 2 or 3 */
    gboolean       gmode;     /* indicate if the hints 'g' mode is used */
    JSContextRef   ctx;
#if WEBKIT_CHECK_VERSION(2, 0, 0)
    /* holds the setting if JavaScript can open windows automatically that we
     * have to change to open windows via hinting */
    gboolean       allow_open_win;
#endif
    guint          timeout_id;
} hints;

extern VbCore vb;

static gboolean call_hints_function(const char *func, int count, JSValueRef params[]);
static void fire_timeout(gboolean on);
static gboolean fire_cb(gpointer data);


void hints_init(WebKitWebFrame *frame)
{
    if (hints.obj) {
        JSValueUnprotect(hints.ctx, hints.obj);
        hints.obj = NULL;
    }
    if (!hints.obj) {
        hints.ctx = webkit_web_frame_get_global_context(frame);
        hints.obj = js_create_object(hints.ctx, HINTS_JS);
    }
}

VbResult hints_keypress(int key)
{
    JSValueRef arguments[1];

    /* if we are not already in hint mode we expect to get a ; to start
     * hinting */
    if (!(vb.mode->flags & FLAG_HINTING) && key != ';') {
        return RESULT_ERROR;
    }

    if (key == KEY_CR) {
        hints_fire();

        return RESULT_COMPLETE;
    } else if (key == CTRL('H')) {
        fire_timeout(false);
        arguments[0] = JSValueMakeNull(hints.ctx);
        if (call_hints_function("update", 1, arguments)) {
            return RESULT_COMPLETE;
        }
    } else if (VB_IS_DIGIT(key)) {
        fire_timeout(true);

        arguments[0] = JSValueMakeNumber(hints.ctx, key - '0');
        if (call_hints_function("update", 1, arguments)) {
            return RESULT_COMPLETE;
        }
    } else if (key == KEY_TAB) {
        fire_timeout(false);
        hints_focus_next(false);

        return RESULT_COMPLETE;
    } else if (key == KEY_SHIFT_TAB) {
        fire_timeout(false);
        hints_focus_next(true);

        return RESULT_COMPLETE;
    }

    fire_timeout(false);
    return RESULT_ERROR;
}

void hints_clear(void)
{
    if (vb.mode->flags & FLAG_HINTING) {
        vb.mode->flags &= ~FLAG_HINTING;
        vb_set_input_text("");

        call_hints_function("clear", 0, NULL);

        g_signal_emit_by_name(vb.gui.webview, "hovering-over-link", NULL, NULL);

#if WEBKIT_CHECK_VERSION(2, 0, 0)
        /* if open window was not allowed for JavaScript, restore this */
        if (!hints.allow_open_win) {
            WebKitWebSettings *setting = webkit_web_view_get_settings(vb.gui.webview);
            g_object_set(G_OBJECT(setting), "javascript-can-open-windows-automatically", hints.allow_open_win, NULL);
        }
#endif
    }
}

void hints_create(const char *input)
{
    /* don't start hinting if the hinting object isn't created - for example
     * if hinting is started before the first data of page are received */
    if (!hints.obj) {
        return;
    }

    /* check if the input contains a valid hinting prompt */
    if (!hints_parse_prompt(input, &hints.mode, &hints.gmode)) {
        /* if input is not valid, clear possible previous hint mode */
        if (vb.mode->flags & FLAG_HINTING) {
            mode_enter('n');
        }
        return;
    }

    if (!(vb.mode->flags & FLAG_HINTING)) {
        vb.mode->flags |= FLAG_HINTING;

#if WEBKIT_CHECK_VERSION(2, 0, 0)
        WebKitWebSettings *setting = webkit_web_view_get_settings(vb.gui.webview);

        /* before we enable JavaScript to open new windows, we save the actual
         * value to be able restore it after hints where fired */
        g_object_get(G_OBJECT(setting), "javascript-can-open-windows-automatically", &(hints.allow_open_win), NULL);

        /* if window open is already allowed there's no need to allow it again */
        if (!hints.allow_open_win) {
            g_object_set(G_OBJECT(setting), "javascript-can-open-windows-automatically", true, NULL);
        }
#endif

        hints.promptlen = hints.gmode ? 3 : 2;

        JSValueRef arguments[] = {
            js_string_to_ref(hints.ctx, (char[]){hints.mode, '\0'}),
            JSValueMakeBoolean(hints.ctx, hints.gmode),
            JSValueMakeNumber(hints.ctx, MAXIMUM_HINTS)
        };
        call_hints_function("init", 3, arguments);

        /* if hinting is started there won't be any additional filter given and
         * we can go out of this function */
        return;
    }

    JSValueRef arguments[] = {js_string_to_ref(hints.ctx, *(input + hints.promptlen) ? input + hints.promptlen : "")};
    call_hints_function("filter", 1, arguments);
}

void hints_focus_next(const gboolean back)
{
    JSValueRef arguments[] = {
        JSValueMakeNumber(hints.ctx, back)
    };
    call_hints_function("focus", 1, arguments);
}

void hints_fire(void)
{
    call_hints_function("fire", 0, NULL);
}

void hints_follow_link(const gboolean back, int count)
{
    char *json = g_strdup_printf(
        "[%s]",
        back ? vb.config.prevpattern : vb.config.nextpattern
    );

    JSValueRef arguments[] = {
        js_string_to_ref(hints.ctx, back ? "prev" : "next"),
        js_object_to_ref(hints.ctx, json),
        JSValueMakeNumber(hints.ctx, count)
    };
    g_free(json);

    call_hints_function("followLink", 3, arguments);
}

void hints_increment_uri(int count)
{
    JSValueRef arguments[] = {
        JSValueMakeNumber(hints.ctx, count)
    };

    call_hints_function("incrementUri", 1, arguments);
}

/**
 * Checks if the given hint prompt belong to a known and valid hints mode and
 * parses the mode and is_gmode into given pointers.
 *
 * The given prompt sting may also contain additional chars after the prompt.
 *
 * @prompt:   String to be parsed as prompt. The Prompt can be followed by
 *            additional characters.
 * @mode:     Pointer to char that will be filled with mode char if prompt was
 *            valid and given pointer is not NULL.
 * @is_gmode: Pointer to gboolean to be filled with the flag to indicate gmode
 *            hinting.
 */
gboolean hints_parse_prompt(const char *prompt, char *mode, gboolean *is_gmode)
{
    gboolean res;
    char pmode = '\0';
#ifdef FEATURE_QUEUE
    static char *modes   = "eiIoOpPstTxyY";
    static char *g_modes = "IpPstyY";
#else
    static char *modes   = "eiIoOstTxyY";
    static char *g_modes = "IstyY";
#endif

    if (!prompt) {
        return false;
    }

    /* get the mode identifying char from prompt */
    if (*prompt == ';') {
        pmode = prompt[1];
    } else if (*prompt == 'g' && strlen(prompt) >= 3) {
        /* get mode for g;X hint modes */
        pmode = prompt[2];
    }

    /* no mode found in prompt */
    if (!pmode) {
        return false;
    }

    res = *prompt == 'g'
        ? strchr(g_modes, pmode) != NULL
        : strchr(modes, pmode) != NULL;

    /* fill pointer only if the prompt was valid */
    if (res) {
        if (mode != NULL) {
            *mode = pmode;
        }
        if (is_gmode != NULL) {
            *is_gmode = *prompt == 'g';
        }
    }

    return res;
}

static gboolean call_hints_function(const char *func, int count, JSValueRef params[])
{
    char *value = js_object_call_function(hints.ctx, hints.obj, func, count, params);

    g_return_val_if_fail(value != NULL, false);

    if (!strncmp(value, "ERROR:", 6)) {
        g_free(value);
        return false;
    }

    if (!strncmp(value, "OVER:", 5)) {
        g_signal_emit_by_name(
            vb.gui.webview, "hovering-over-link", NULL, *(value + 5) == '\0' ? NULL : (value + 5)
        );
        g_free(value);

        return true;
    }

    /* following return values mark fired hints */
    if (!strncmp(value, "DONE:", 5)) {
        fire_timeout(false);
        if (!hints.gmode) {
            mode_enter('n');
        }
    } else if (!strncmp(value, "INSERT:", 7)) {
        fire_timeout(false);
        mode_enter('i');
        if (hints.mode == 'e') {
            input_open_editor();
        }
    } else if (!strncmp(value, "DATA:", 5)) {
        fire_timeout(false);
        /* switch first to normal mode - else we would clear the inputbox
         * on switching mode also if we want to show yanked data */
        if (!hints.gmode) {
            mode_enter('n');
        }

        char *v = (value + 5);
        Arg a   = {0};
        /* put the hinted value into register "; */
        vb_register_add(';', v);
        switch (hints.mode) {
            /* used if images should be opened */
            case 'i':
            case 'I':
                a.s = v;
                a.i = (hints.mode == 'I') ? VB_TARGET_NEW : VB_TARGET_CURRENT;
                vb_load_uri(&a);
                break;

            case 'O':
            case 'T':
                vb_echo(VB_MSG_NORMAL, false, "%s %s", (hints.mode == 'T') ? ":tabopen" : ":open", v);
                if (!hints.gmode) {
                    mode_enter('c');
                }
                break;

            case 's':
                a.s = v;
                a.i = COMMAND_SAVE_URI;
                command_save(&a);
                break;

            case 'x':
                map_handle_string(GET_CHAR("x-hint-command"), true);
                break;

            case 'y':
            case 'Y':
                a.i = COMMAND_YANK_ARG;
                a.s = v;
                command_yank(&a, '\0');
                break;

#ifdef FEATURE_QUEUE
            case 'p':
            case 'P':
                a.s = v;
                a.i = (hints.mode == 'P') ? COMMAND_QUEUE_UNSHIFT : COMMAND_QUEUE_PUSH;
                command_queue(&a);
                break;
#endif
        }
    }
    g_free(value);
    return true;
}

static void fire_timeout(gboolean on)
{
    int millis;
    /* remove possible timeout function */
    if (hints.timeout_id) {
        g_source_remove(hints.timeout_id);
        hints.timeout_id = 0;
    }

    if (on) {
        millis = GET_INT("hint-timeout");
        if (millis) {
            hints.timeout_id = g_timeout_add(millis, (GSourceFunc)fire_cb, NULL);
        }
    }
}

static gboolean fire_cb(gpointer data)
{
    hints_fire();

    /* remove timeout id for the timeout that is removed by return value of
     * false automatic */
    hints.timeout_id = 0;
    return false;
}
