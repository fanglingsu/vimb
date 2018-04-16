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

#ifndef _HANDLERS_H
#define _HANDLERS_H

typedef struct handler Handler;

Handler *handler_new();
void handler_free(Handler *h);
gboolean handler_add(Handler *h, const char *key, const char *cmd);
gboolean handler_remove(Handler *h, const char *key);
gboolean handler_handle_uri(Handler *h, const char *uri);
gboolean handler_fill_completion(Handler *h, GtkListStore *store, const char *input);

#endif /* end of include guard: _HANDLERS_H */

