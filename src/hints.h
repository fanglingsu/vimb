/**
 * vimp - a webkit based vim like browser.
 *
 * Copyright (C) 2012 Daniel Carl
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

#ifndef HINTS_H
#define HINTS_H

#include "main.h"

typedef enum {
    HINTS_MODE_LINK,
    HINTS_MODE_IMAGE,
} HintMode;

void hints_create(const gchar* input, HintMode mode);
void hints_update(const gulong num);
void hints_clear(void);
void hints_clear_focus(void);
void hints_focus_next(const gboolean back);

#endif /* end of include guard: HINTS_H */
