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

#ifndef _KEYBIND_H
#define _KEYBIND_H

#include <gdk/gdkkeysyms.h>
#include <gdk/gdkkeysyms-compat.h>

typedef struct {
    int    mode;        /* mode maks for allowed browser modes */
    guint  modkey;
    guint  modmask;     /* modemask for the kayval */
    guint  keyval;
    char* command;     /* command to run */
    char* param;
} Keybind;

void keybind_init(void);
void keybind_init_client(Client* c);
void keybind_cleanup(void);
gboolean keybind_add_from_string(char* keys, const char* command, const Mode mode);
gboolean keybind_remove_from_string(char* str, const Mode mode);

#endif /* end of include guard: _KEYBIND_H */
