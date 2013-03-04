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

extern const int COMMAND_HISTORY_SIZE;

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

const char* history_get(const int step)
{
    const char* command;

    /* get the search prefix only on start of history search */
    if (!vp.state.history_active) {
        OVERWRITE_STRING(vp.state.history_prefix, GET_TEXT());

        /* generate new history list with the matching items */
        for (GList* l = core.behave.history; l; l = l->next) {
            char* entry = g_strdup((char*)l->data);
            if (g_str_has_prefix(entry, vp.state.history_prefix)) {
                vp.state.history_active = g_list_prepend(vp.state.history_active, entry);
            }
        }

        vp.state.history_active = g_list_reverse(vp.state.history_active);
    }

    const int len = g_list_length(vp.state.history_active);
    if (!len) {
        return NULL;
    }

    /* if reached end/beginnen start at the opposit site of list again */
    vp.state.history_pointer = (len + vp.state.history_pointer + step) % len;

    command = (char*)g_list_nth_data(vp.state.history_active, vp.state.history_pointer);

    return command;
}

void history_rewind(void)
{
    if (vp.state.history_active) {
        OVERWRITE_STRING(vp.state.history_prefix, NULL);
        vp.state.history_pointer = 0;
        /* free temporary used history list */
        g_list_free_full(vp.state.history_active, (GDestroyNotify)g_free);
        vp.state.history_active = NULL;
    }
}
