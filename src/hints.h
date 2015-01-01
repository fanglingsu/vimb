/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2015 Daniel Carl
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

void hints_init(WebKitWebFrame *frame);
VbResult hints_keypress(int key);
void hints_create(const char *input);
void hints_fire(void);
void hints_follow_link(const gboolean back, int count);
void hints_increment_uri(int count);
gboolean hints_parse_prompt(const char *prompt, char *mode, gboolean *is_gmode);
void hints_clear(void);
void hints_focus_next(const gboolean back);

#endif /* end of include guard: _HINTS_H */
