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
#include "pass.h"
#include "mode.h"
#include "dom.h"
#include "ascii.h"

extern VbCore vb;


/**
 * Function called when vimb enters the passthrough mode.
 */
void pass_enter(void)
{
    /* switch focus first to make sure we can write to the inputbox without
     * disturbing the user */
    gtk_widget_grab_focus(GTK_WIDGET(vb.gui.webview));
    vb_echo(VB_MSG_NORMAL, false, "-- PASS THROUGH --");
}

/**
 * Called when passthrough mode is left.
 */
void pass_leave(void)
{
    vb_set_input_text("");
}

VbResult pass_keypress(int key)
{
    if (key == CTRL('[')) { /* esc */
        mode_enter('n');
    }
    vb.state.processed_key = false;
    return RESULT_COMPLETE;
}
