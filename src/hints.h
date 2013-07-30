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

#ifndef _HINTS_H
#define _HINTS_H

#include "main.h"

#define HINTS_GET_TYPE(n) ((n) & (HINTS_TYPE_LINK | HINTS_TYPE_IMAGE))

enum {
    HINTS_TYPE_LINK     = 1,
    HINTS_TYPE_IMAGE    = 2,
    HINTS_TYPE_EDITABLE = 3,
    HINTS_PROCESS_INPUT = (1 << 2),
    HINTS_PROCESS_YANK  = (1 << 3),
    HINTS_PROCESS_OPEN  = (1 << 4),
    HINTS_PROCESS_SAVE  = (1 << 5),
    /* additional flag for HINTS_PROCESS_OPEN */
    HINTS_OPEN_NEW      = (1 << 6),
    HINTS_PROCESS_PUSH  = (1 << 7),
};

void hints_init(WebKitWebFrame *frame);
void hints_create(const char *input, guint mode, const guint prefixLength);
void hints_update(const gulong num);
void hints_clear();
void hints_focus_next(const gboolean back);

#endif /* end of include guard: _HINTS_H */
