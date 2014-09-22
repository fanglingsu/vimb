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

#ifndef _UTIL_H
#define _UTIL_H

#include "main.h"

enum {
    UTIL_EXP_TILDE   = 0x01, /* ~/ and ~user expansion */
    UTIL_EXP_DOLLAR  = 0x02, /* $ENV and ${ENV} expansion */
    UTIL_EXP_SPECIAL = 0x04, /* expand % to current URI */
};

typedef gboolean (*Util_Comp_Func)(const char*, const char*);
typedef void *(*Util_Content_Func)(const char*);

char* util_get_config_dir(void);
char* util_get_cache_dir(void);
const char* util_get_home_dir(void);
void util_create_dir_if_not_exists(const char* dirpath);
void util_create_file_if_not_exists(const char* filename);
char* util_get_file_contents(const char* filename, gsize* length);
char** util_get_lines(const char* filename);
GList *util_file_to_unique_list(const char *filename, Util_Content_Func func,
    GCompareFunc unique_func, GDestroyNotify free_func, unsigned int max_items);
gboolean util_file_append(const char *file, const char *format, ...);
gboolean util_file_prepend(const char *file, const char *format, ...);
char* util_strcasestr(const char* haystack, const char* needle);
char *util_str_replace(const char* search, const char* replace, const char* string);
gboolean util_create_tmp_file(const char *content, char **file);
char *util_build_path(const char *path, const char *dir);
char *util_expand(const char *src, int expflags);
gboolean util_parse_expansion(const char **input, GString *str, int flags,
    const char *quoteable);
gboolean util_wildmatch(const char *pattern, const char *subject);
gboolean util_fill_completion(GtkListStore *store, const char *input, GList *src);

#endif /* end of include guard: _UTIL_H */
