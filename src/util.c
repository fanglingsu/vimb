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
    gchar *path = g_build_filename(g_get_user_config_dir(), "vimp", NULL);
    util_create_dir_if_not_exists(path);

    return path;
}

gchar* util_get_cache_dir(void)
{
    gchar *path = g_build_filename(g_get_user_cache_dir(), "vimp", NULL);
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

Arg* util_char_to_arg(const gchar* str, Type type)
{
    Arg* arg = util_new_arg();

    if (!str) {
        return NULL;
    }
    switch (type) {
        case TYPE_BOOLEAN:
            arg->i = g_ascii_strncasecmp(str, "true", 4) == 0
                || g_ascii_strncasecmp(str, "on", 2) == 0 ? 1 : 0;
            break;

        case TYPE_INTEGER:
            arg->i = g_ascii_strtoull(str, (gchar**)NULL, 10);
            break;

        case TYPE_DOUBLE:
            arg->i = (1000 * g_ascii_strtod(str, (gchar**)NULL));
            break;

        case TYPE_CHAR:
        case TYPE_COLOR:
            arg->s = g_strdup(str);
            break;
    }

    return arg;
}

Arg* util_new_arg(void)
{
    Arg* arg = g_new0(Arg, 1);
    arg->i = 0;
    arg->s = NULL;

    return arg;
}
