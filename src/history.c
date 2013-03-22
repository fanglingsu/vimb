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

#include "main.h"
#include "history.h"

extern VbCore vb;
extern const unsigned int COMMAND_HISTORY_SIZE;

void history_cleanup(void)
{
    g_list_free_full(vb.behave.history, (GDestroyNotify)g_free);
}

void history_append(const char* line)
{
    if (COMMAND_HISTORY_SIZE <= g_list_length(vb.behave.history)) {
        /* if list is too long - remove items from beginning */
        GList* first = g_list_first(vb.behave.history);
        g_free((char*)first->data);
        vb.behave.history = g_list_delete_link(vb.behave.history, first);
    }
    vb.behave.history = g_list_append(vb.behave.history, g_strdup(line));
}

const char* history_get(const int step)
{
    const char* command;

    /* get the search prefix only on start of history search */
    if (!vb.state.history_active) {
        OVERWRITE_STRING(vb.state.history_prefix, gtk_entry_get_text(GTK_ENTRY(vb.gui.inputbox)));

        /* generate new history list with the matching items */
        for (GList* l = vb.behave.history; l; l = l->next) {
            char* entry = g_strdup((char*)l->data);
            if (g_str_has_prefix(entry, vb.state.history_prefix)) {
                vb.state.history_active = g_list_prepend(vb.state.history_active, entry);
            }
        }

        vb.state.history_active = g_list_reverse(vb.state.history_active);
    }

    const int len = g_list_length(vb.state.history_active);
    if (!len) {
        return NULL;
    }

    /* if reached end/beginnen start at the opposit site of list again */
    vb.state.history_pointer = (len + vb.state.history_pointer + step) % len;

    command = (char*)g_list_nth_data(vb.state.history_active, vb.state.history_pointer);

    return command;
}

void history_rewind()
{
    if (vb.state.history_active) {
        OVERWRITE_STRING(vb.state.history_prefix, NULL);
        vb.state.history_pointer = 0;
        /* free temporary used history list */
        g_list_free_full(vb.state.history_active, (GDestroyNotify)g_free);
        vb.state.history_active = NULL;
    }
}
