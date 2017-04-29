/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2017 Daniel Carl
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
#include <JavaScriptCore/JavaScript.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkkeysyms-compat.h>
#include "hints.h"
#include "main.h"
#include "ascii.h"
#include "command.h"
#include "hints.js.h"
#include "input.h"
#include "map.h"
#include "ext-proxy.h"

static struct {
    JSObjectRef    obj;       /* the js object */
    char           mode;      /* mode identifying char - that last char of the hint prompt */
    int            promptlen; /* length of the hint prompt chars 2 or 3 */
    gboolean       gmode;     /* indicate if the hints 'g' mode is used */
    JSContextRef   ctx;
    /* holds the setting if JavaScript can open windows automatically that we
     * have to change to open windows via hinting */
    gboolean       allow_open_win;
    guint          timeout_id;
} hints;

extern struct Vimb vb;

static void call_hints_function(Client *c, const char *func, char* args);
static void fire_timeout(Client *c, gboolean on);
static gboolean fire_cb(gpointer data);


VbResult hints_keypress(Client *c, int key)
{
    if (key == KEY_CR) {
        hints_fire(c);

        return RESULT_COMPLETE;
    } else if (key == CTRL('H')) {
        fire_timeout(c, false);
        call_hints_function(c, "update", "null");
        return RESULT_MORE; // continue handling the backspace
    } else if (key == KEY_TAB) {
        fire_timeout(c, false);
        hints_focus_next(c, false);

        return RESULT_COMPLETE;
    } else if (key == KEY_SHIFT_TAB) {
        fire_timeout(c, false);
        hints_focus_next(c, true);

        return RESULT_COMPLETE;
    } else {
        fire_timeout(c, true);
        /* try to handle the key by the javascript */
        call_hints_function(c, "update", (char[]){key, '\0'});
        return RESULT_COMPLETE;
    }

    fire_timeout(c, false);
    return RESULT_ERROR;
}

void hints_clear(Client *c)
{
    if (c->mode->flags & FLAG_HINTING) {
        c->mode->flags &= ~FLAG_HINTING;
        vb_input_set_text(c, "");

        call_hints_function(c, "clear", "");

        /* g_signal_emit_by_name(c->webview, "hovering-over-link", NULL, NULL); */

        /* if open window was not allowed for JavaScript, restore this */
        /* if (!hints.allow_open_win) {                                                                                  */
        /*     WebKitWebSettings *setting = webkit_web_view_get_settings(c->webview);                                    */
        /*     g_object_set(G_OBJECT(setting), "javascript-can-open-windows-automatically", hints.allow_open_win, NULL); */
        /* }                                                                                                             */
    }
}

void hints_create(Client *c, char *input)
{
    char *jsargs;

    /* check if the input contains a valid hinting prompt */
    if (!hints_parse_prompt(input, &hints.mode, &hints.gmode)) {
        /* if input is not valid, clear possible previous hint mode */
        if (c->mode->flags & FLAG_HINTING) {
            vb_enter(c, 'n');
        }
        return;
    }

    if (!(c->mode->flags & FLAG_HINTING)) {
        c->mode->flags |= FLAG_HINTING;

        /* WebKitWebSettings *setting = webkit_web_view_get_settings(c->webview); */

        /* before we enable JavaScript to open new windows, we save the actual
         * value to be able restore it after hints where fired */
        /* g_object_get(G_OBJECT(setting), "javascript-can-open-windows-automatically", &(hints.allow_open_win), NULL); */

        /* if window open is already allowed there's no need to allow it again */
        /* if (!hints.allow_open_win) {                                                                  */
        /*     g_object_set(G_OBJECT(setting), "javascript-can-open-windows-automatically", true, NULL); */
        /* }                                                                                             */

        hints.promptlen = hints.gmode ? 3 : 2;

        jsargs = g_strdup_printf("'%s', %s, %d, '%s', %s, %s",
            (char[]){hints.mode, '\0'},
            hints.gmode ? "true" : "false",
            MAXIMUM_HINTS,
            GET_CHAR(c, "hintkeys"),
            GET_BOOL(c, "hint-follow-last") ? "true" : "false",      
            GET_BOOL(c, "hint-number-same-length") ? "true" : "false"
        );

        call_hints_function(c, "init", jsargs);
        g_free(jsargs);

        /* if hinting is started there won't be any additional filter given and
         * we can go out of this function */
        return;
    }

    jsargs = *(input + hints.promptlen) ? input + hints.promptlen : "";
    call_hints_function(c, "filter", jsargs);
}

void hints_focus_next(Client *c, const gboolean back)
{
    call_hints_function(c, "focus", back ? "true" : "false");
}

void hints_fire(Client *c)
{
    call_hints_function(c, "fire", "");
}

void hints_follow_link(Client *c, const gboolean back, int count)
{
    /* char *json = g_strdup_printf(                            */
    /*     "[%s]",                                              */
    /*     back ? vb.config.prevpattern : vb.config.nextpattern */
    /* );                                                       */

    /* JSValueRef arguments[] = {                               */
    /*     js_string_to_ref(hints.ctx, back ? "prev" : "next"), */
    /*     js_object_to_ref(hints.ctx, json),                   */
    /*     JSValueMakeNumber(hints.ctx, count)                  */
    /* };                                                       */
    /* g_free(json);                                            */

    /* call_hints_function(c, "followLink", 3, arguments);         */
}

void hints_increment_uri(Client *c, int count)
{
    char *jsargs;

    jsargs = g_strdup_printf("%d", count);
    call_hints_function(c, "incrementUri", jsargs);
    g_free(jsargs);
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

static void hints_function_callback(GDBusProxy *proxy, GAsyncResult *result, Client *c)
{
    gboolean success = FALSE;
    char *value = NULL;

    g_print("callback!\n");
    GVariant *return_value = g_dbus_proxy_call_finish(proxy, result, NULL);   
    if (!return_value) {
        return;
    }

    g_variant_get(return_value, "(bs)", &success, &value);
    if (!success) {
        return;
    }
    g_print("foo! %s\n", value);

    if (!strncmp(value, "ERROR:", 6)) {
        return;
    }

    if (!strncmp(value, "OVER:", 5)) {
        /* g_signal_emit_by_name(                                                                */
        /*     c->webview, "hovering-over-link", NULL, *(value + 5) == '\0' ? NULL : (value + 5) */
        /* );                                                                                    */
        return;
    }

    /* following return values mark fired hints */
    if (!strncmp(value, "DONE:", 5)) {
        fire_timeout(c, false);
        /* Change to normal mode only if we are currently in command mode and
         * we are not in g-mode hinting. This is required to not switch to
         * normal mode when the hinting triggered a click that set focus on
         * editable element that lead vimb to switch to input mode. */
        if (!hints.gmode && c->mode->id == 'c') {
            vb_enter(c, 'n');
        }
    } else if (!strncmp(value, "INSERT:", 7)) {
        fire_timeout(c, false);
        vb_enter(c, 'i');
        if (hints.mode == 'e') {
            input_open_editor(c);
        }
    } else if (!strncmp(value, "DATA:", 5)) {
        fire_timeout(c, false);
        /* switch first to normal mode - else we would clear the inputbox
         * on switching mode also if we want to show yanked data */
        if (!hints.gmode) {
            vb_enter(c, 'n');
        }

        char *v = (value + 5);
        Arg a   = {0};
        /* put the hinted value into register "; */
        vb_register_add(c, ';', v);
        switch (hints.mode) {
            /* used if images should be opened */
            case 'i':
            case 'I':
                a.s = v;
                a.i = (hints.mode == 'I') ? TARGET_NEW : TARGET_CURRENT;
                vb_load_uri(c, &a);
                break;

            case 'O':
            case 'T':
                vb_echo(c, MSG_NORMAL, false, "%s %s", (hints.mode == 'T') ? ":tabopen" : ":open", v);
                if (!hints.gmode) {
                    vb_enter(c, 'c');
                }
                break;

            case 's':
                a.s = v;
                a.i = COMMAND_SAVE_URI;
                command_save(c, &a);
                break;

            case 'x':
                map_handle_string(c, GET_CHAR(c, "x-hint-command"), true);
                break;

            case 'y':
            case 'Y':
                a.i = COMMAND_YANK_ARG;
                a.s = v;
                command_yank(c, &a, c->state.current_register);
                break;

#ifdef FEATURE_QUEUE
            case 'p':
            case 'P':
                a.s = v;
                a.i = (hints.mode == 'P') ? COMMAND_QUEUE_UNSHIFT : COMMAND_QUEUE_PUSH;
                command_queue(c, &a);
                break;
#endif
        }
    }
}

static void call_hints_function(Client *c, const char *func, char* args)
{
    char *jscode;

    jscode = g_strdup_printf("hints.%s(%s);", func, args);
    ext_proxy_eval_script(c, jscode, (GAsyncReadyCallback)hints_function_callback);
    g_free(jscode);

}

static void fire_timeout(Client *c, gboolean on)
{
    int millis;
    /* remove possible timeout function */
    if (hints.timeout_id) {
        g_source_remove(hints.timeout_id);
        hints.timeout_id = 0;
    }

    if (on) {
        millis = GET_INT(c, "hint-timeout");
        g_print("millis %d", millis);
        if (millis) {
            hints.timeout_id = g_timeout_add(millis, (GSourceFunc)fire_cb, c);
        }
    }
}

static gboolean fire_cb(gpointer data)
{
    hints_fire(data);

    /* remove timeout id for the timeout that is removed by return value of
     * false automatic */
    hints.timeout_id = 0;
    return false;
}
