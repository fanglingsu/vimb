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

#ifndef _BOOKMARK_H
#define _BOOKMARK_H

#include "main.h"

gboolean bookmark_add(const char *uri, const char *title, const char *tags);
gboolean bookmark_remove(const char *uri);
gboolean bookmark_fill_completion(GtkListStore *store, const char *input);
gboolean bookmark_fill_tag_completion(GtkListStore *store, const char *input);
#ifdef FEATURE_QUEUE
gboolean bookmark_queue_push(const char *uri);
gboolean bookmark_queue_unshift(const char *uri);
char *bookmark_queue_pop(int *item_count);
gboolean bookmark_queue_clear(void);
#endif

#endif /* end of include guard: _BOOKMARK_H */

