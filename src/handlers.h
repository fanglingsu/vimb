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

#ifndef _HANDLERS_H
#define _HANDLERS_H

void handlers_init(void);
void handlers_cleanup(void);
gboolean handler_add(const char *key, const char *cmd);
gboolean handler_remove(const char *key);
gboolean handle_uri(const char *uri);
gboolean handler_fill_completion(GtkListStore *store, const char *input);

#endif /* end of include guard: _HANDLERS_H */
