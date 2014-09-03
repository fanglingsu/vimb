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
#include "main.h"
#include "mode.h"
#include "map.h"
#include "ascii.h"
#include <glib.h>

static GHashTable *modes = NULL;
extern VbCore vb;

static void free_mode(Mode *mode);


void mode_init(void)
{
    modes = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)free_mode);
}

void mode_cleanup(void)
{
    if (modes) {
        g_hash_table_destroy(modes);
        vb.mode = NULL;
    }
}

/**
 * Creates a new mode with given callback functions.
 */
void mode_add(char id, ModeTransitionFunc enter, ModeTransitionFunc leave,
    ModeKeyFunc keypress, ModeInputChangedFunc input_changed)
{
    Mode *new = g_slice_new(Mode);
    new->id            = id;
    new->enter         = enter;
    new->leave         = leave;
    new->keypress      = keypress;
    new->input_changed = input_changed;
    new->flags         = 0;

    g_hash_table_insert(modes, GINT_TO_POINTER(id), new);
}

/**
 * Enter into the new given mode and leave possible active current mode.
 */
void mode_enter(char id)
{
    Mode *new = g_hash_table_lookup(modes, GINT_TO_POINTER(id));

    g_return_if_fail(new != NULL);

    if (vb.mode) {
        /* don't do anything if the mode isn't a new one */
        if (vb.mode == new) {
            return;
        }

        /* if there is a active mode, leave this first */
        if (vb.mode->leave) {
            vb.mode->leave();
        }
    }

    /* reset the flags of the new entered mode */
    new->flags = 0;

    /* set the new mode so that it is available also in enter function */
    vb.mode = new;
    /* call enter only if the new mode isn't the current mode */
    if (new->enter) {
        new->enter();
    }

#ifndef TESTLIB
    vb_update_statusbar();
#endif
}

/**
 * Set the prompt chars and switch to new mode.
 *
 * @id:           Mode id.
 * @prompt:       Prompt string to set as current prompt.
 * @print_prompt: Indicates if the new set prompt should be put into inputbox
 *                after switching the mode.
 */
void mode_enter_prompt(char id, const char *prompt, gboolean print_prompt)
{
    /* set the prompt to be accessible in mode_enter */
    strncpy(vb.state.prompt, prompt, PROMPT_SIZE - 1);
    vb.state.prompt[PROMPT_SIZE - 1] = '\0';

    mode_enter(id);

    if (print_prompt) {
        /* set it after the mode was entered so that the modes input change
         * event listener could grep the new prompt */
        vb_echo_force(VB_MSG_NORMAL, false, vb.state.prompt);
    }
}

VbResult mode_handle_key(int key)
{
    VbResult res;
    static gboolean ctrl_v = false;

    if (ctrl_v) {
        vb.state.processed_key = false;
        ctrl_v = false;

        return RESULT_COMPLETE;
    }
    if (vb.mode->id != 'p' && key == CTRL('V')) {
        vb.mode->flags |= FLAG_NOMAP;
        ctrl_v = true;
        map_showcmd(key);

        return RESULT_MORE;
    }

    if (vb.mode && vb.mode->keypress) {
#ifdef DEBUG
        int flags = vb.mode->flags;
        int id    = vb.mode->id;
        res = vb.mode->keypress(key);
        if (vb.mode) {
            PRINT_DEBUG(
                "%c[%d]: %#.2x '%c' -> %c[%d]",
                id - ' ', flags, key, (key >= 0x20 && key <= 0x7e) ? key : ' ',
                vb.mode->id - ' ', vb.mode->flags
            );
        }
#else
        res = vb.mode->keypress(key);
#endif
        return res;
    }
    return RESULT_ERROR;
}

gboolean mode_input_focusin(GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
    /* enter the command mode if the focus is on inputbox */
    mode_enter('c');

    return false;
}

/**
 * Process input changed event on current active mode.
 */
void mode_input_changed(GtkTextBuffer* buffer, gpointer data)
{
    char *text;
    GtkTextIter start, end;
    /* don't observe changes in completion mode */
    if (vb.mode->flags & FLAG_COMPLETION) {
        return;
    }
    /* don't process changes not typed by the user */
    if (gtk_widget_is_focus(vb.gui.input) && vb.mode && vb.mode->input_changed) {
        gtk_text_buffer_get_bounds(buffer, &start, &end);
        text = gtk_text_buffer_get_text(buffer, &start, &end, false);

        vb.mode->input_changed(text);

        g_free(text);
    }
}

static void free_mode(Mode *mode)
{
    g_slice_free(Mode, mode);
}
