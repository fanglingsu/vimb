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

void history_rewind(void)
{
    core.behave.history_pointer = 0;
}
