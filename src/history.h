/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2018 Daniel Carl
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

#ifndef _HISTORY_H
#define _HISTORY_H

#include <glib.h>

#include "main.h"

typedef enum {
    HISTORY_FIRST   = 0,
    HISTORY_COMMAND = 0,
    HISTORY_SEARCH,
    HISTORY_URL,
    HISTORY_LAST
} HistoryType;

void history_add(Client *c, HistoryType type, const char *value, const char *additional);
void history_cleanup(void);
gboolean history_fill_completion(GtkListStore *store, HistoryType type, const char *input);
GList *history_get_list(VbInputType type, const char *query);

#endif /* end of include guard: _HISTORY_H */
