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

#include "util.h"
#include <stdio.h>

gchar* util_get_config_dir(void)
{
    gchar *path = g_build_filename(g_get_user_config_dir(), PROJECT, NULL);
    util_create_dir_if_not_exists(path);

    return path;
}

gchar* util_get_cache_dir(void)
{
    gchar *path = g_build_filename(g_get_user_cache_dir(), PROJECT, NULL);
    util_create_dir_if_not_exists(path);

    return path;
}

void util_create_dir_if_not_exists(const gchar* dirpath)
{
    if (!g_file_test(dirpath, G_FILE_TEST_IS_DIR)) {
        g_mkdir_with_parents(dirpath, 0755);
    }
}

void util_create_file_if_not_exists(const char* filename) {
    if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
        FILE* f = fopen(filename, "a");
        fclose(f);
    }
}

gboolean util_atob(const gchar* str)
{
    if (str == NULL) {
        return FALSE;
    }

    return g_ascii_strncasecmp(str, "true", 4) == 0
        || g_ascii_strncasecmp(str, "on", 2) == 0;
}
