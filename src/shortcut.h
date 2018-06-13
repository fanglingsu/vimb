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

#ifndef _SHORTCUT_H
#define _SHORTCUT_H

typedef struct shortcut Shortcut;

Shortcut *shortcut_new(void);
void shortcut_free(Shortcut *sc);
gboolean shortcut_add(Shortcut *sc, const char *key, const char *uri);
gboolean shortcut_remove(Shortcut *sc, const char *key);
gboolean shortcut_set_default(Shortcut *sc, const char *key);
char *shortcut_get_uri(Shortcut *sc, const char *key);
gboolean shortcut_fill_completion(Shortcut *c, GtkListStore *store, const char *input);

#endif /* end of include guard: _SHORTCUT_H */

