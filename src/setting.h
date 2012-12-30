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

#ifndef SETTING_H
#define SETTING_H

#include "main.h"

typedef struct _Setting Setting;
typedef gboolean (*SettingFunc)(const Setting*, const gboolean get);

struct _Setting {
    gchar*      alias;
    gchar*      name;
    Type        type;
    SettingFunc func;
    Arg         arg;
};

void setting_init(void);
void setting_cleanup(void);
gboolean setting_run(gchar* name, const gchar* param);

#endif /* end of include guard: SETTING_H */
