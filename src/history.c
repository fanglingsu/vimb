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
        core.behave.history = g_list_delete_link(core.behave.history, g_list_first(core.behave.history));
    }
    core.behave.history = g_list_append(core.behave.history, g_strdup(line));
}

const char* history_get(const int step)
{
    const char* command;
    GList* history = NULL;

    /* get the search prefix only on start of history search */
    if (!vp.state.history_prefix) {
        const char* text = GET_TEXT();
        /* TODO at the moment we skip only the first char of input box but
         * maybe we'll have history items that have a longer or no prefix */
        OVERWRITE_STRING(vp.state.history_prefix, (text + 1));
    }

    for (GList* l = core.behave.history; l; l = l->next) {
        char* entry = (char*)l->data;
        if (g_str_has_prefix(entry, vp.state.history_prefix)) {
            history = g_list_prepend(history, entry);
        }
    }

    const int len = g_list_length(history);
    if (!len) {
        return NULL;
    }

    history = g_list_reverse(history);

    /* if reached end/beginnen start at the opposit site of list again */
    vp.state.history_pointer = (len + vp.state.history_pointer + step) % len;

    command = (char*)g_list_nth_data(history, vp.state.history_pointer);

    return command;
}

void history_rewind(void)
{
    vp.state.history_pointer = 0;
    OVERWRITE_STRING(vp.state.history_prefix, NULL);
}
