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

#include "util.h"
#include <stdio.h>

char* util_get_config_dir(void)
{
    char *path = g_build_filename(g_get_user_config_dir(), "vimp", NULL);
    util_create_dir_if_not_exists(path);

    return path;
}

char* util_get_cache_dir(void)
{
    char *path = g_build_filename(g_get_user_cache_dir(), "vimp", NULL);
    util_create_dir_if_not_exists(path);

    return path;
}

const char* util_get_home_dir(void)
{
    const char* dir = g_getenv("HOME");

    if (!dir) {
        dir = g_get_home_dir();
    }

    return dir;
}

void util_create_dir_if_not_exists(const char* dirpath)
{
    if (!g_file_test(dirpath, G_FILE_TEST_IS_DIR)) {
        g_mkdir_with_parents(dirpath, 0755);
    }
}

void util_create_file_if_not_exists(const char* filename)
{
    if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
        FILE* f = fopen(filename, "a");
        fclose(f);
    }
}

/**
 * Retrieves the length bytes from given file.
 *
 * The memory of returned string have to be freed!
 */
char* util_get_file_contents(const char* filename, gsize* length)
{
    GError* error  = NULL;
    char* content = NULL;
    if (!(g_file_test(filename, G_FILE_TEST_IS_REGULAR)
        && g_file_get_contents(filename, &content, length, &error))
    ) {
        fprintf(stderr, "Cannot open %s: %s\n", filename, error ? error->message : "file not found");
        g_clear_error(&error);
    }
    return content;
}

/**
 * Retrieves the file content as lines.
 *
 * The result have to be freed by g_strfreev().
 */
char** util_get_lines(const char* filename)
{
    char* content = util_get_file_contents(filename, NULL);
    char** lines  = NULL;
    if (content) {
        /* split the file content into lines */
        lines = g_strsplit(content, "\n", -1);
        g_free(content);
    }
    return lines;
}
