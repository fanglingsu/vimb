/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2017 Daniel Carl
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

#ifndef _HANDLERS_H
#define _HANDLERS_H

void handler_init(Client *c);
void handler_cleanup(Client *c);
gboolean handler_add(Client *c, const char *key, const char *cmd);
gboolean handler_remove(Client *c, const char *key);
gboolean handler_handle_uri(Client *c, const char *uri);
gboolean handler_fill_completion(Client *c, GtkListStore *store, const char *input);

#endif /* end of include guard: _HANDLERS_H */

