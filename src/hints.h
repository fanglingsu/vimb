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

#ifndef _HINTS_H
#define _HINTS_H

#include "main.h"

VbResult hints_keypress(Client *c, int key);
void hints_create(Client *c, const char *input);
void hints_fire(Client *c);
void hints_follow_link(Client *c, gboolean back, int count);
void hints_increment_uri(Client *c, int count);
gboolean hints_parse_prompt(const char *prompt, char *mode, gboolean *is_gmode);
void hints_clear(Client *c);
void hints_focus_next(Client *c, const gboolean back);

#endif /* end of include guard: _HINTS_H */
