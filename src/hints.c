/**
 * vimp - a webkit based vim like browser.
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
#include <gdk/gdkkeysyms-compat.h>
#include "hints.h"
#include "dom.h"
#include "command.h"
#include "hints.js.h"

#define HINT_VAR "VpHint"
#define HINT_FILE NULL

static void hints_run_script(char* js);
static void hints_fire(void);
static void hints_observe_input(gboolean observe);
static gboolean hints_changed_callback(GtkEditable *entry, gpointer data);
static gboolean hints_keypress_callback(WebKitWebView* webview, GdkEventKey* event);

void hints_init(void)
{
    char* value = NULL;
    char* error = NULL;
    vp_eval_script(
        webkit_web_view_get_main_frame(vp.gui.webview), HINTS_JS, HINT_FILE, &value, &error
    );
    g_free(value);
    g_free(error);
}

void hints_clear(void)
{
    hints_observe_input(FALSE);
    if (GET_CLEAN_MODE() == VP_MODE_HINTING) {
        char* js = g_strdup_printf("%s.clear();", HINT_VAR);
        char* value = NULL;
        char* error = NULL;

        vp_eval_script(
            webkit_web_view_get_main_frame(vp.gui.webview), js, HINT_FILE, &value, &error
        );
        g_free(value);
        g_free(error);
        g_free(js);
    }
}

void hints_create(const char* input, guint mode, const guint prefixLength)
{
    char* js = NULL;
    char  type;
    if (GET_CLEAN_MODE() != VP_MODE_HINTING) {
        Style* style = &core.style;
        vp.hints.prefixLength = prefixLength;
        vp.hints.mode         = mode;
        vp.hints.num          = 0;

        js = g_strdup_printf(
            "%s = new VimpHints('%s', '%s', '%s', '%s');",
            HINT_VAR,
            style->hint_bg,
            style->hint_bg_focus,
            style->hint_fg,
            style->hint_style
        );
        hints_run_script(js);
        g_free(js);

        hints_observe_input(TRUE);
    }

    /* convert the mode into the type chare used in the hint script */
    if (mode & HINTS_PROCESS) {
        type = 'd';
    } else if (mode & HINTS_TYPE_IMAGE) {
        type = (HINTS_TARGET_BLANK & mode) ? 'I' : 'i';
    } else {
        type = (HINTS_TARGET_BLANK & mode) ? 'F' : 'f';
    }

    js = g_strdup_printf("%s.create('%s', '%c');", HINT_VAR, input ? input : "", type);
    hints_run_script(js);
    g_free(js);
}

void hints_update(const gulong num)
{
    char* js = g_strdup_printf("%s.update(%lu);", HINT_VAR, num);
    hints_run_script(js);
    g_free(js);
}

void hints_focus_next(const gboolean back)
{
    char* js = g_strdup_printf(back ? "%s.focusPrev()" : "%s.focusNext();", HINT_VAR);
    hints_run_script(js);
    g_free(js);
}

static void hints_run_script(char* js)
{
    char* value = NULL;
    char* error = NULL;
    int mode = vp.hints.mode;

    vp_eval_script(
        webkit_web_view_get_main_frame(vp.gui.webview), js, HINT_FILE, &value, &error
    );
    if (error) {
        fprintf(stderr, "%s\n", error);
        g_free(error);

        vp_set_mode(VP_MODE_NORMAL, FALSE);

        return;
    }
    if (!value) {
        return;
    }
    if (!strncmp(value, "DONE:", 5)) {
        hints_observe_input(FALSE);
        vp_set_mode(VP_MODE_NORMAL, TRUE);
    } else if (!strncmp(value, "INSERT:", 7)) {
        hints_observe_input(FALSE);
        vp_set_mode(VP_MODE_INSERT, TRUE);
    } else if (!strncmp(value, "DATA:", 5)) {
        hints_observe_input(FALSE);
        HintsProcess type = HINTS_GET_PROCESSING(mode);
        Arg a = {0};
        switch (type) {
            case HINTS_PROCESS_INPUT:
                a.s = g_strconcat((mode & HINTS_TARGET_BLANK) ? ":tabopen " : ":open ", (value + 5), NULL);
                command_input(&a);
                g_free(a.s);
                break;

            case HINTS_PROCESS_YANK:
                a.i = COMMAND_YANK_PRIMARY | COMMAND_YANK_SECONDARY;
                a.s = g_strdup((value + 5));
                command_yank(&a);
                g_free(a.s);
                break;
        }
    }
    g_free(value);
}

static void hints_fire(void)
{
    hints_observe_input(FALSE);
    char* js = g_strdup_printf("%s.fire();", HINT_VAR);
    hints_run_script(js);
    g_free(js);
}

static void hints_observe_input(gboolean observe)
{
    static gulong changeHandler   = 0;
    static gulong keypressHandler = 0;

    if (observe) {
        changeHandler = g_signal_connect(
            G_OBJECT(vp.gui.inputbox), "changed", G_CALLBACK(hints_changed_callback), NULL
        );
        keypressHandler = g_signal_connect(
            G_OBJECT(vp.gui.inputbox), "key-press-event", G_CALLBACK(hints_keypress_callback), NULL
        );
    } else if (changeHandler && keypressHandler) {
        g_signal_handler_disconnect(G_OBJECT(vp.gui.inputbox), changeHandler);
        g_signal_handler_disconnect(G_OBJECT(vp.gui.inputbox), keypressHandler);
        changeHandler = 0;
        keypressHandler = 0;
    }
}

static gboolean hints_changed_callback(GtkEditable *entry, gpointer data)
{
    const char* text = GET_TEXT();

    /* skip hinting prefixes like '.', ',', ';y' ... */
    hints_create(text + vp.hints.prefixLength, vp.hints.mode, vp.hints.prefixLength);

    return TRUE;
}

static gboolean hints_keypress_callback(WebKitWebView* webview, GdkEventKey* event)
{
    Hints* hints = &vp.hints;
    int numval;
    guint keyval = event->keyval;
    guint state  = CLEAN_STATE_WITH_SHIFT(event);

    if (keyval == GDK_Return) {
        hints_fire();
        return TRUE;
    }
    if (keyval == GDK_BackSpace && (state & GDK_SHIFT_MASK) && (state & GDK_CONTROL_MASK)) {
        hints->num /= 10;
        hints_update(hints->num);

        return TRUE;
    }
    numval = g_unichar_digit_value((gunichar)gdk_keyval_to_unicode(keyval));
    if ((numval >= 1 && numval <= 9) || (numval == 0 && hints->num)) {
        /* allow a zero as non-first number */
        hints->num = (hints->num ? hints->num * 10 : 0) + numval;
        hints_update(hints->num);

        return TRUE;
    }

    return FALSE;
}
