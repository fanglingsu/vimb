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

#ifndef _SEARCHENGINE_H
#define _SEARCHENGINE_H

void searchengine_cleanup(void);
gboolean searchengine_add(const char *handle, const char *uri);
gboolean searchengine_remove(const char *handle);
gboolean searchengine_set_default(const char *handle);
char *searchengine_get_uri(const char *handle);

#endif /* end of include guard: _SEARCHENGINE_H */
