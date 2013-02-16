/**
 * vimp - a webkit based vim like browser.
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

#ifndef _UTIL_H
#define _UTIL_H

#include "main.h"

gchar* util_get_config_dir(void);
gchar* util_get_cache_dir(void);
const gchar* util_get_home_dir(void);
void util_create_dir_if_not_exists(const gchar* dirpath);
void util_create_file_if_not_exists(const gchar* filename);
gchar* util_get_file_contents(const gchar* filename, gsize* length);
gchar** util_get_lines(const gchar* filename);

#endif /* end of include guard: _UTIL_H */
