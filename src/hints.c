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

#include <ctype.h>
#include "config.h"
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkkeysyms-compat.h>
#include "hints.h"
#include "ascii.h"
#include "dom.h"
#include "command.h"
#include "hints.js.h"
#include "mode.h"
#include "input.h"
#include "map.h"

#define HINT_VAR "VbHint"
#define HINT_FILE "hints.js"

static struct {
    guint    num;       /* olds the numeric filter for hints typed by the user */
    char     mode;      /* mode identifying char - that last char of the hint prompt */
    int      promptlen; /* lenfth of the hint prompt chars 2 or 3 */
    gboolean gmode;     /* indicate if the hints g mode is used */
} hints;

extern VbCore vb;

static void run_script(char *js);


void hints_init(WebKitWebFrame *frame)
{
    char *value = NULL;
    vb_eval_script(frame, HINTS_JS, HINT_FILE, &value);
    g_free(value);
}

VbResult hints_keypress(int key)
{
    /* if we are not already in hint mode we expect to get a ; to start
     * hinting */
    if (!(vb.mode->flags & FLAG_HINTING) && key != ';') {
        return RESULT_ERROR;
    }

    if (key == KEY_CR) {
        hints_fire();

        return RESULT_COMPLETE;
    }

    /* if there is an active filter by hint num backspace will remove the
     * number filter first */
    if (hints.num && key == CTRL('H')) {
        hints.num /= 10;
        hints_update(hints.num);

        return RESULT_COMPLETE;
    }
    if (isdigit(key)) {
        int num = key - '0';
        if ((num >= 1 && num <= 9) || (num == 0 && hints.num)) {
            /* allow a zero as non-first number */
            hints.num = (hints.num ? hints.num * 10 : 0) + num;
            hints_update(hints.num);

            return RESULT_COMPLETE;
        }
    }
    if (key == KEY_TAB) {
        hints_focus_next(false);

        return RESULT_COMPLETE;
    }
    if (key == KEY_SHIFT_TAB) {
        hints_focus_next(true);

        return RESULT_COMPLETE;
    }

    return RESULT_ERROR;
}

void hints_clear(void)
{
    char *js, *value = NULL;

    if (vb.mode->flags & FLAG_HINTING) {
        vb.mode->flags &= ~FLAG_HINTING;
        vb_set_input_text("");
        js = g_strconcat(HINT_VAR, ".clear();", NULL);
        vb_eval_script(webkit_web_view_get_main_frame(vb.gui.webview), js, HINT_FILE, &value);
        g_free(value);
        g_free(js);

        g_signal_emit_by_name(vb.gui.webview, "hovering-over-link", NULL, NULL);
    }
}

void hints_create(const char *input)
{
    char *js = NULL;

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

        /* unset number filter - this is required to remove the last char from
         * inputbox on backspace also if there was used a number filter prior */
        hints.num       = 0;
        hints.promptlen = hints.gmode ? 3 : 2;

        js = g_strdup_printf("%s.init('%c', %d);", HINT_VAR, hints.mode, MAXIMUM_HINTS);

        run_script(js);
        g_free(js);

        /* if hinting is started there won't be any aditional filter given and
         * we can go out of this function */
        return;
    }

    js = g_strdup_printf("%s.filter('%s');", HINT_VAR, *(input + hints.promptlen) ? input + hints.promptlen : "");
    run_script(js);
    g_free(js);
}

void hints_update(int num)
{
    char *js = g_strdup_printf("%s.update(%d);", HINT_VAR, hints.num);
    run_script(js);
    g_free(js);
}

void hints_focus_next(const gboolean back)
{
    char *js = g_strdup_printf("%s.focus(%d);", HINT_VAR, back);
    run_script(js);
    g_free(js);
}

void hints_fire(void)
{
    char *js = g_strconcat(HINT_VAR, ".fire();", NULL);
    run_script(js);
    g_free(js);
}

void hints_follow_link(const gboolean back, int count)
{
    char *pattern = back ? vb.config.prevpattern : vb.config.nextpattern;
    char *js      = g_strdup_printf(
        "%s.followLink('%s', [%s], %d);", HINT_VAR,
        back ? "prev" : "next",
        pattern,
        count
    );
    run_script(js);
    g_free(js);
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
    static char *modes   = "eiIoOpPstTy";
    static char *g_modes = "IpPsty";
#else
    static char *modes   = "eiIoOstTy";
    static char *g_modes = "Isty";
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

    /* fill pointer only if the promt was valid */
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

static void run_script(char *js)
{
    char *value = NULL;

    gboolean success = vb_eval_script(
        webkit_web_view_get_main_frame(vb.gui.webview), js, HINT_FILE, &value
    );
    if (!success) {
        fprintf(stderr, "%s\n", value);
        g_free(value);

        mode_enter('n');

        return;
    }

    if (!strncmp(value, "OVER:", 5)) {
        g_signal_emit_by_name(
            vb.gui.webview, "hovering-over-link", NULL, *(value + 5) == '\0' ? NULL : (value + 5)
        );
    } else if (!strncmp(value, "DONE:", 5)) {
        if (hints.gmode) {
            /* if g mode is used reset number filter and keep in hint mode */
            hints.num = 0;
            hints_update(hints.num);
        } else {
            mode_enter('n');
        }
    } else if (!strncmp(value, "INSERT:", 7)) {
        mode_enter('i');
        if (hints.mode == 'e') {
            input_open_editor();
        }
    } else if (!strncmp(value, "DATA:", 5)) {
        if (hints.gmode) {
            /* if g mode is used reset number filter and keep in hint mode */
            hints.num = 0;
            hints_update(hints.num);
        } else {
            /* switch first to normal mode - else we would clear the inputbox
             * on switching mode also if we want to show yanked data */
            mode_enter('n');
        }

        char *v   = (value + 5);
        Arg a     = {0};
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

            case 'y':
                a.i = COMMAND_YANK_ARG;
                a.s = v;
                command_yank(&a);
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
}
