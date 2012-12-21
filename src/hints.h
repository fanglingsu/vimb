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

#define HINTS_GET_TYPE(type)       ((type) & (HINTS_TYPE_LAST))
#define HINTS_GET_PROCESSING(type) ((type) & ~(HINTS_TYPE_LAST | HINTS_PROCESS | HINTS_TARGET_BLANK))

/*
bits 1 and 2 form the hint type
3:  0 = click hint       1 = process source
4:  0 = open current     1 = open in new window
all further bits are used for processing types
*/
typedef enum {
    HINTS_TYPE_LINK,
    HINTS_TYPE_IMAGE,
    HINTS_TYPE_DEFAULT,
    HINTS_TYPE_FORM,
    HINTS_TYPE_LAST = HINTS_TYPE_FORM,
} HintsType;

enum {
    HINTS_CLICK,
    HINTS_PROCESS = (1 << 2)
};

enum {
    HINTS_TARGET_CURRENT,
    HINTS_TARGET_BLANK = (1 << 3)
};

typedef enum {
    HINTS_PROCESS_INPUT = (1 << 4),
    HINTS_PROCESS_YANK
} HintsProcess;

void hints_create(const gchar* input, guint mode);
void hints_update(const gulong num);
void hints_clear(void);
void hints_focus_next(const gboolean back);

#endif /* end of include guard: HINTS_H */
