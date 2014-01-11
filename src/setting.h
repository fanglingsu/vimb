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

#ifndef _SETTING_H
#define _SETTING_H

#include "main.h"

typedef enum {
    SETTING_SET,
    SETTING_GET,
    SETTING_TOGGLE
} SettingType;

typedef enum {
    SETTING_OK,
    SETTING_ERROR         = (0 << 1),
    SETTING_USER_NOTIFIED = (0 << 2)
} SettingStatus;

typedef struct _Setting Setting;
typedef SettingStatus (*SettingFunc)(const Setting*, const SettingType);

struct _Setting {
    char*       alias;
    char*       name;
    Type        type;
    SettingFunc func;
    Arg         arg;
};

void setting_init(void);
void setting_cleanup(void);
gboolean setting_run(char* name, const char* param);
gboolean setting_fill_completion(GtkListStore *store, const char *input);

#endif /* end of include guard: _SETTING_H */
