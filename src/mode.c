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

#include "config.h"
#include "main.h"
#include "mode.h"

static GHashTable *modes = NULL;
extern VbCore vb;

void mode_init(void)
{
    modes = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_free);
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
    Mode *new = g_new(Mode, 1);
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
    if (!new) {
        PRINT_DEBUG("!mode %c not found", id);
        return;
    }

    if (vb.mode) {
        /* don't do anything if the mode isn't a new one */
        if (vb.mode == new) {
            return;
        }

        /* if there is a active mode, leave this first */
        if (vb.mode->leave) {
            PRINT_DEBUG("leave %c", vb.mode->id);
            vb.mode->leave();
        }
    }

    /* reset the flags of the new entered mode */
    new->flags = 0;

    /* set the new mode so that it is available also in enter function */
    vb.mode = new;
    /* call enter only if the new mode isn't the current mode */
    if (new->enter) {
        PRINT_DEBUG("enter %c", new->id);
        new->enter();
    }

    vb_update_statusbar();
}

VbResult mode_handle_key(unsigned int key)
{
    VbResult res;
    if (vb.mode && vb.mode->keypress) {
        int flags = vb.mode->flags;
        int id    = vb.mode->id;
        res = vb.mode->keypress(key);
        if (vb.mode) {
            PRINT_DEBUG(
                "%c: key[0x%x %c] flags[%d] >> %c: flags[%d]",
                id, key, (key >= 0x20 && key <= 0x7e) ? key : ' ',
                flags, vb.mode->id, vb.mode->flags
            );
        }
        return res;
    }
    return RESULT_ERROR;
}

/**
 * Process input changed event on current active mode.
 */
void mode_input_changed(GtkTextBuffer* buffer, gpointer data)
{
    char *text;
    GtkTextIter start, end;
    /* don't process changes not typed by the user */
    if (gtk_widget_is_focus(vb.gui.input) && vb.mode && vb.mode->input_changed) {
        gtk_text_buffer_get_bounds(buffer, &start, &end);
        text = gtk_text_buffer_get_text(buffer, &start, &end, false);

        vb.mode->input_changed(text);

        g_free(text);
    }
}
