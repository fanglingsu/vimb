/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2016 Daniel Carl
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

void shortcut_init(Client *c);
void shortcut_cleanup(Client *c);
gboolean shortcut_add(Client *c, const char *key, const char *uri);
gboolean shortcut_remove(Client *c, const char *key);
gboolean shortcut_set_default(Client *c, const char *key);
char *shortcut_get_uri(Client *c, const char *key);
/*gboolean shortcut_fill_completion(Client *c, GtkListStore *store, const char *input);*/

#endif /* end of include guard: _SHORTCUT_H */

