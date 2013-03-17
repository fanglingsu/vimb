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
#include <gdk/gdkkeysyms-compat.h>
#include "hints.h"
#include "dom.h"
#include "command.h"
#include "hints.js.h"

#define HINT_VAR "VpHint"
#define HINT_FILE "hints.js"

extern const unsigned int MAXIMUM_HINTS;

static void hints_run_script(Client* c, char* js);
static void hints_fire(Client* c);
static void hints_observe_input(Client* c, gboolean observe);
static gboolean hints_changed_callback(GtkEditable *entry, Client* c);
static gboolean hints_keypress_callback(WebKitWebView* webview, GdkEventKey* event, Client* c);

void hints_init(WebKitWebFrame* frame)
{
    char* value = NULL;
    vb_eval_script(frame, HINTS_JS, HINT_FILE, &value);
    g_free(value);
}

void hints_clear(Client* c)
{
    hints_observe_input(c, FALSE);
    if (CLEAN_MODE(c->state.mode) == VB_MODE_HINTING) {
        char* js = g_strdup_printf("%s.clear();", HINT_VAR);
        char* value = NULL;
        vb_eval_script(webkit_web_view_get_main_frame(c->gui.webview), js, HINT_FILE, &value);
        g_free(value);
        g_free(js);

        g_signal_emit_by_name(c->gui.webview, "hovering-over-link", NULL, NULL);
    }
}

void hints_create(Client* c, const char* input, guint mode, const guint prefixLength)
{
    char* js = NULL;
    if (CLEAN_MODE(c->state.mode) != VB_MODE_HINTING) {
        Style* style = &core.style;
        c->hints.prefixLength = prefixLength;
        c->hints.mode         = mode;
        c->hints.num          = 0;

        char type, usage;
        /* convert the mode into the type chare used in the hint script */
        type = mode & HINTS_TYPE_LINK ? 'l' : 'i';

        if (mode & HINTS_PROCESS_OPEN) {
            usage = mode & HINTS_OPEN_NEW ? 'T' : 'O';
        } else {
            usage = 'U';
        }

        js = g_strdup_printf(
            "%s = new VimbHints('%c', '%c', '%s', '%s', '%s', '%s', %d);",
            HINT_VAR, type, usage,
            style->hint_bg,
            style->hint_bg_focus,
            style->hint_fg,
            style->hint_style,
            MAXIMUM_HINTS
        );
        hints_run_script(c, js);
        g_free(js);

        hints_observe_input(c, TRUE);
    }


    js = g_strdup_printf("%s.create('%s');", HINT_VAR, input ? input : "");
    hints_run_script(c, js);
    g_free(js);
}

void hints_update(Client* c, const gulong num)
{
    char* js = g_strdup_printf("%s.update(%lu);", HINT_VAR, num);
    hints_run_script(c, js);
    g_free(js);
}

void hints_focus_next(Client* c, const gboolean back)
{
    char* js = g_strdup_printf(back ? "%s.focusPrev()" : "%s.focusNext();", HINT_VAR);
    hints_run_script(c, js);
    g_free(js);
}

static void hints_run_script(Client* c, char* js)
{
    char* value = NULL;
    int mode = c->hints.mode;

    gboolean success = vb_eval_script(
        webkit_web_view_get_main_frame(c->gui.webview), js, HINT_FILE, &value
    );
    if (!success) {
        fprintf(stderr, "%s\n", value);
        g_free(value);

        vb_set_mode(c, VB_MODE_NORMAL, FALSE);

        return;
    }

    if (!strncmp(value, "OVER:", 5)) {
        g_signal_emit_by_name(
            c->gui.webview, "hovering-over-link", NULL, *(value + 5) == '\0' ? NULL : (value + 5)
        );
    } else if (!strncmp(value, "DONE:", 5)) {
        hints_observe_input(c, FALSE);
        vb_set_mode(c, VB_MODE_NORMAL, TRUE);
    } else if (!strncmp(value, "INSERT:", 7)) {
        hints_observe_input(c, FALSE);
        vb_set_mode(c, VB_MODE_INSERT, FALSE);
    } else if (!strncmp(value, "DATA:", 5)) {
        hints_observe_input(c, FALSE);
        Arg a = {0};
        char* v = (value + 5);
        if (mode & HINTS_PROCESS_INPUT) {
            a.s = g_strconcat((mode & HINTS_OPEN_NEW) ? ":tabopen " : ":open ", v, NULL);
            command_input(c, &a);
            g_free(a.s);
        } else {
            a.i = COMMAND_YANK_PRIMARY | COMMAND_YANK_SECONDARY;
            a.s = v;
            command_yank(c, &a);
        }
    }
    g_free(value);
}

static void hints_fire(Client* c)
{
    hints_observe_input(c, FALSE);
    char* js = g_strdup_printf("%s.fire();", HINT_VAR);
    hints_run_script(c, js);
    g_free(js);
}

static void hints_observe_input(Client* c, gboolean observe)
{
    if (observe) {
        c->hints.change_handler = g_signal_connect(
            G_OBJECT(c->gui.inputbox), "changed", G_CALLBACK(hints_changed_callback), c
        );
        c->hints.keypress_handler = g_signal_connect(
            G_OBJECT(c->gui.inputbox), "key-press-event", G_CALLBACK(hints_keypress_callback), c
        );
    } else if (c->hints.change_handler && c->hints.keypress_handler) {
        g_signal_handler_disconnect(G_OBJECT(c->gui.inputbox), c->hints.change_handler);
        g_signal_handler_disconnect(G_OBJECT(c->gui.inputbox), c->hints.keypress_handler);

        c->hints.change_handler = c->hints.keypress_handler = 0;

        /* clear the input box - TODO move this to a better place */
        gtk_widget_grab_focus(GTK_WIDGET(c->gui.webview));
        gtk_entry_set_text(GTK_ENTRY(c->gui.inputbox), "");
    }
}

static gboolean hints_changed_callback(GtkEditable *entry, Client* c)
{
    const char* text = gtk_entry_get_text(GTK_ENTRY(c->gui.inputbox));

    /* skip hinting prefixes like '.', ',', ';y' ... */
    hints_create(c, text + c->hints.prefixLength, c->hints.mode, c->hints.prefixLength);

    return TRUE;
}

static gboolean hints_keypress_callback(WebKitWebView* webview, GdkEventKey* event, Client* c)
{
    int numval;
    guint keyval = event->keyval;
    guint state  = CLEAN_STATE_WITH_SHIFT(event);

    if (keyval == GDK_Return) {
        hints_fire(c);
        return TRUE;
    }
    if (keyval == GDK_BackSpace && (state & GDK_SHIFT_MASK) && (state & GDK_CONTROL_MASK)) {
        c->hints.num /= 10;
        hints_update(c, c->hints.num);

        return TRUE;
    }
    numval = g_unichar_digit_value((gunichar)gdk_keyval_to_unicode(keyval));
    if ((numval >= 1 && numval <= 9) || (numval == 0 && c->hints.num)) {
        /* allow a zero as non-first number */
        c->hints.num = (c->hints.num ? c->hints.num * 10 : 0) + numval;
        hints_update(c, c->hints.num);

        return TRUE;
    }

    return FALSE;
}
