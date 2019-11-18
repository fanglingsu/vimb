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

#include "config.h"
#include <string.h>
#include <webkit2/webkit2.h>
#include <JavaScriptCore/JavaScript.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkkeysyms-compat.h>
#include "hints.h"
#include "main.h"
#include "ascii.h"
#include "command.h"
#include "input.h"
#include "map.h"
#include "normal.h"
#include "ext-proxy.h"

static struct {
    char           mode;      /* mode identifying char - that last char of the hint prompt */
    int            promptlen; /* length of the hint prompt chars 2 or 3 */
    gboolean       gmode;     /* indicate if the hints 'g' mode is used */
    /* holds the setting if JavaScript can open windows automatically that we
     * have to change to open windows via hinting */
    gboolean       allow_open_win;
    gboolean       allow_javascript;
    guint          timeout_id;
} hints;

extern struct Vimb vb;

static gboolean call_hints_function(Client *c, const char *func, const char* args,
        gboolean sync);
static void on_hint_function_finished(GDBusProxy *proxy, GAsyncResult *result,
        Client *c);
static gboolean hint_function_check_result(Client *c, GVariant *return_value);
static void fire_timeout(Client *c, gboolean on);
static gboolean fire_cb(gpointer data);


VbResult hints_keypress(Client *c, int key)
{
    if (key == KEY_CR) {
        hints_fire(c);

        return RESULT_COMPLETE;
    } else if (key == CTRL('H')) { /* backspace */
        fire_timeout(c, FALSE);
        if (call_hints_function(c, "update", "null", TRUE)) {
            return RESULT_COMPLETE;
        }
    } else if (key == KEY_TAB) {
        fire_timeout(c, FALSE);
        hints_focus_next(c, FALSE);

        return RESULT_COMPLETE;
    } else if (key == KEY_SHIFT_TAB) {
        fire_timeout(c, FALSE);
        hints_focus_next(c, TRUE);

        return RESULT_COMPLETE;
    } else if (key == CTRL('D') || key == CTRL('F') || key == CTRL('B') || key == CTRL('U')) {
        return normal_keypress(c, key);
    } else if (key == CTRL('J') || key == CTRL('K')) {
        return normal_keypress(c, UNCTRL(key));
    } else {
        fire_timeout(c, TRUE);
        /* try to handle the key by the javascript */
        if (call_hints_function(c, "update", (char[]){'"', key, '"', '\0'}, TRUE)) {
            return RESULT_COMPLETE;
        }
    }

    fire_timeout(c, FALSE);
    return RESULT_ERROR;
}

void hints_clear(Client *c)
{
    if (c->mode->flags & FLAG_HINTING) {
        c->mode->flags &= ~FLAG_HINTING;
        vb_input_set_text(c, "");

        /* Run this sync else we would disable JavaScript before the hint is
         * fired. */
        call_hints_function(c, "clear", "true", TRUE);

        /* if open window was not allowed for JavaScript, restore this */
        WebKitSettings *setting = webkit_web_view_get_settings(c->webview);
        if (!hints.allow_open_win) {
            g_object_set(G_OBJECT(setting), "javascript-can-open-windows-automatically", hints.allow_open_win, NULL);
        }
        if (!hints.allow_javascript) {
            g_object_set(G_OBJECT(setting), "enable-javascript", hints.allow_javascript, NULL);
        }
    }
}

void hints_create(Client *c, const char *input)
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

        WebKitSettings *setting = webkit_web_view_get_settings(c->webview);

        /* before we enable JavaScript to open new windows, we save the actual
         * value to be able restore it after hints where fired */
        g_object_get(G_OBJECT(setting),
                "javascript-can-open-windows-automatically", &(hints.allow_open_win),
                "enable-javascript", &(hints.allow_javascript),
                NULL);

        /* if window open is already allowed there's no need to allow it again */
        if (!hints.allow_open_win) {
            g_object_set(G_OBJECT(setting), "javascript-can-open-windows-automatically", TRUE, NULL);
        }
        /* TODO This might be a security issue to toggle JavaScript
         * temporarily on. */
        /* This is a hack to allow window.setTimeout() and scroll observers in
         * hinting script which does not work when JavaScript is disabled. */
        if (!hints.allow_javascript) {
            g_object_set(G_OBJECT(setting), "enable-javascript", TRUE, NULL);
        }

        hints.promptlen = hints.gmode ? 3 : 2;

        jsargs = g_strdup_printf("'%s', %s, %d, '%s', %s, %s",
            (char[]){hints.mode, '\0'},
            hints.gmode ? "true" : "false",
            MAXIMUM_HINTS,
            GET_CHAR(c, "hint-keys"),
            GET_BOOL(c, "hint-follow-last") ? "true" : "false",
            GET_BOOL(c, "hint-keys-same-length") ? "true" : "false"
        );

        call_hints_function(c, "init", jsargs, FALSE);
        g_free(jsargs);

        /* if hinting is started there won't be any additional filter given and
         * we can go out of this function */
        return;
    }

    if (GET_BOOL(c, "hint-match-element")) {
        jsargs = g_strdup_printf("'%s'", *(input + hints.promptlen) ? input + hints.promptlen : "");
        call_hints_function(c, "filter", jsargs, FALSE);
        g_free(jsargs);
    }
}

void hints_focus_next(Client *c, const gboolean back)
{
    call_hints_function(c, "focus", back ? "true" : "false", FALSE);
}

void hints_fire(Client *c)
{
    call_hints_function(c, "fire", "", FALSE);
}

void hints_follow_link(Client *c, const gboolean back, int count)
{
    /* TODO implement outside of hints.c */
    /* We would previously "piggyback" on hints.js for the "js" part of this feature
     * but this would actually be more elegant in its own JS file. This has nothing
     * to do with hints.
     */
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
    call_hints_function(c, "incrementUri", jsargs, FALSE);
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
    static char *g_modes = "IopPstyY";
#else
    static char *modes   = "eiIoOstTxyY";
    static char *g_modes = "IostyY";
#endif

    if (!prompt) {
        return FALSE;
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
        return FALSE;
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

static gboolean call_hints_function(Client *c, const char *func, const char* args,
        gboolean sync)
{
    char *jscode;
    /* Default value is only return in case of async call. */
    gboolean success = TRUE;

    jscode = g_strdup_printf("hints.%s(%s);", func, args);
    if (sync) {
        GVariant *result;
        result  = ext_proxy_eval_script_sync(c, jscode);
        success = hint_function_check_result(c, result);
    } else {
        ext_proxy_eval_script(c, jscode, (GAsyncReadyCallback)on_hint_function_finished);
    }
    g_free(jscode);

    return success;
}

static void on_hint_function_finished(GDBusProxy *proxy, GAsyncResult *result,
        Client *c)
{
    GVariant *return_value;

    return_value = g_dbus_proxy_call_finish(proxy, result, NULL);
    hint_function_check_result(c, return_value);
}

static gboolean hint_function_check_result(Client *c, GVariant *return_value)
{
    gboolean success = FALSE;
    char *value = NULL;

    if (!return_value) {
        goto error;
    }

    g_variant_get(return_value, "(bs)", &success, &value);
    if (!success || !strncmp(value, "ERROR:", 6)) {
        goto error;
    }
    if (!strncmp(value, "OVER:", 5)) {
        /* If focused elements src is given fire mouse-target-changed signal
         * to show its uri in the statusbar. */
        if (*(value + 7)) {
            /* We get OVER:{I,A}:element-url so we use byte 6 to check for the
             * hinted element type image I or link A. */
            if (*(value + 5) == 'I') {
                vb_statusbar_show_hover_url(c, LINK_TYPE_IMAGE, value + 7);
            } else {
                vb_statusbar_show_hover_url(c, LINK_TYPE_LINK, value + 7);
            }
        } else {
            goto error;
        }
    } else if (!strncmp(value, "DONE:", 5)) {
        fire_timeout(c, FALSE);
        /* Change to normal mode only if we are currently in command mode and
         * we are not in g-mode hinting. This is required to not switch to
         * normal mode when the hinting triggered a click that set focus on
         * editable element that lead vimb to switch to input mode. */
        if (!hints.gmode && c->mode->id == 'c') {
            vb_enter(c, 'n');
        }
        /* If open in new window hinting is use, set a flag on the mode after
         * changing to normal mode. This is used in on_webview_decide_policy
         * to enforce opening into new instance for the next navigation
         * action. */
        if (hints.mode == 't') {
            c->mode->flags |= FLAG_NEW_WIN;
        }
    } else if (!strncmp(value, "INSERT:", 7)) {
        fire_timeout(c, FALSE);
        vb_enter(c, 'i');
        if (hints.mode == 'e') {
            input_open_editor(c);
        }
    } else if (!strncmp(value, "DATA:", 5)) {
        fire_timeout(c, FALSE);
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
                vb_echo(c, MSG_NORMAL, FALSE, "%s %s", (hints.mode == 'T') ? ":tabopen" : ":open", v);
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
                map_handle_string(c, GET_CHAR(c, "x-hint-command"), TRUE);
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

    return TRUE;

error:
    vb_statusbar_show_hover_url(c, LINK_TYPE_NONE, NULL);
    return FALSE;
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
    return FALSE;
}
