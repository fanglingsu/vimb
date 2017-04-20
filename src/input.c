/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2016 Daniel Carl
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

#include <glib.h>
#include <glib/gstdio.h>

#include "ascii.h"
#include "config.h"
#include "input.h"
#include "main.h"
#include "normal.h"
#include "util.h"
#include "ext-proxy.h"


/**
 * Function called when vimb enters the input mode.
 */
void input_enter(Client *c)
{
    /* switch focus first to make sure we can write to the inputbox without
     * disturbing the user */
    gtk_widget_grab_focus(GTK_WIDGET(c->webview));
    vb_modelabel_update(c, "-- INPUT --");
    ext_proxy_eval_script(c, "var vimb_input_mode_element = document.activeElement;", NULL);
}

/**
 * Called when the input mode is left.
 */
void input_leave(Client *c)
{
    ext_proxy_eval_script(c, "vimb_input_mode_element.blur();", NULL);
    vb_modelabel_update(c, "");
}

/**
 * Handles the keypress events from webview and inputbox.
 */
VbResult input_keypress(Client *c, int key)
{
    static gboolean ctrlo = FALSE;

    if (ctrlo) {
        /* if we are in ctrl-O mode perform the next keys as normal mode
         * commands until the command is complete or error */
        VbResult res = normal_keypress(c, key);
        if (res != RESULT_MORE) {
            ctrlo = FALSE;
            /* Don't overwrite the mode label in case we landed in another
             * mode. This might occurre by CTRL-0 CTRL-Z or after running ex
             * command, where we mainly end up in normal mode. */
            if (c->mode->id == 'i') {
                /* reenter the input mode */
                input_enter(c);
            }
        }
        return res;
    }

    switch (key) {
        case CTRL('['): /* esc */
            vb_enter(c, 'n');
            return RESULT_COMPLETE;

        case CTRL('O'):
            /* enter CTRL-0 mode to execute next command in normal mode */
            ctrlo           = TRUE;
            c->mode->flags |= FLAG_NOMAP;
            vb_modelabel_update(c, "-- (input) --");
            return RESULT_MORE;

        case CTRL('T'):
            return input_open_editor(c);

        case CTRL('Z'):
            vb_enter(c, 'p');
            return RESULT_COMPLETE;
    }

    c->state.processed_key = FALSE;
    return RESULT_ERROR;
}

VbResult input_open_editor(Client *c)
{
    /* TODO should the editor be opened by the webextension or by the
     * application? */
    return RESULT_COMPLETE;
}
