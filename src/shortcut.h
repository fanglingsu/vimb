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

#ifndef _SHORTCUT_H
#define _SHORTCUT_H

void shortcut_init(void);
void shortcut_cleanup(void);
gboolean shortcut_add(const char *key, const char *uri);
gboolean shortcut_remove(const char *key);
gboolean shortcut_set_default(const char *key);
char *shortcut_get_uri(const char *key);
gboolean shortcut_fill_completion(GtkListStore *store, const char *input);

#endif /* end of include guard: _SHORTCUT_H */
