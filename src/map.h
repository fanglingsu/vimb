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

#ifndef _MAP_H
#define _MAP_H

typedef enum {
    MAP_DONE,
    MAP_AMBIGUOUS,
    MAP_NOMATCH
} MapState;

void map_init(Client *c);
void map_cleanup(Client *c);
MapState map_handle_keys(Client *c, const guchar *keys, int keylen, gboolean use_map);
void map_handle_string(Client *c, const char *str, gboolean use_map);
void map_insert(Client *c, const char *in, const char *mapped, char mode, gboolean remap);
gboolean map_delete(Client *c, const char *in, char mode);
gboolean on_map_keypress(GtkWidget *widget, GdkEventKey* event, Client *c);

#endif /* end of include guard: _MAP_H */
