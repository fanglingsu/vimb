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

#include "main.h"
#include "history.h"

extern const unsigned int COMMAND_HISTORY_SIZE;

void history_cleanup(void)
{
    g_list_free_full(core.behave.history, (GDestroyNotify)g_free);
}

void history_append(const char* line)
{
    if (COMMAND_HISTORY_SIZE <= g_list_length(core.behave.history)) {
        /* if list is too long - remove items from beginning */
        GList* first = g_list_first(core.behave.history);
        g_free((char*)first->data);
        core.behave.history = g_list_delete_link(core.behave.history, first);
    }
    core.behave.history = g_list_append(core.behave.history, g_strdup(line));
}

const char* history_get(Client* c, const int step)
{
    const char* command;

    /* get the search prefix only on start of history search */
    if (!c->state.history_active) {
        OVERWRITE_STRING(c->state.history_prefix, gtk_entry_get_text(GTK_ENTRY(c->gui.inputbox)));

        /* generate new history list with the matching items */
        for (GList* l = core.behave.history; l; l = l->next) {
            char* entry = g_strdup((char*)l->data);
            if (g_str_has_prefix(entry, c->state.history_prefix)) {
                c->state.history_active = g_list_prepend(c->state.history_active, entry);
            }
        }

        c->state.history_active = g_list_reverse(c->state.history_active);
    }

    const int len = g_list_length(c->state.history_active);
    if (!len) {
        return NULL;
    }

    /* if reached end/beginnen start at the opposit site of list again */
    c->state.history_pointer = (len + c->state.history_pointer + step) % len;

    command = (char*)g_list_nth_data(c->state.history_active, c->state.history_pointer);

    return command;
}

void history_rewind(Client* c)
{
    if (c->state.history_active) {
        OVERWRITE_STRING(c->state.history_prefix, NULL);
        c->state.history_pointer = 0;
        /* free temporary used history list */
        g_list_free_full(c->state.history_active, (GDestroyNotify)g_free);
        c->state.history_active = NULL;
    }
}
