/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2018 Daniel Carl
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

#include <glib.h>
#include "main.h"

enum {
    UTIL_EXP_TILDE   = 0x01, /* ~/ and ~user expansion */
    UTIL_EXP_DOLLAR  = 0x02, /* $ENV and ${ENV} expansion */
    UTIL_EXP_SPECIAL = 0x04, /* expand % to current URI */
};
typedef void *(*Util_Content_Func)(const char*, const char*);

char *util_build_path(State state, const char *path, const char *dir);
void util_cleanup(void);
gboolean util_create_dir_if_not_exists(const char *dirpath);
gboolean util_create_tmp_file(const char *content, char **file);
char *util_expand(State state, const char *src, int expflags);
gboolean util_file_append(const char *file, const char *format, ...);
gboolean util_file_prepend(const char *file, const char *format, ...);
void util_file_prepend_line(const char *file, const char *line,
        unsigned int max_lines);
char *util_file_pop_line(const char *file, int *item_count);
char *util_get_config_dir(void);
char *util_get_file_contents(const char *filename, gsize *length);
gboolean util_file_set_content(const char *file, const char *contents);
char **util_get_lines(const char *filename);
GList *util_strv_to_unique_list(char **lines, Util_Content_Func func,
        guint max_items);
gboolean util_fill_completion(GtkListStore *store, const char *input, GList *src);
gboolean util_filename_fill_completion(State state, GtkListStore *store, const char *input);
char *util_js_result_as_string(WebKitJavascriptResult *result);
double util_js_result_as_number(WebKitJavascriptResult *result);
gboolean util_parse_expansion(State state, const char **input, GString *str,
        int flags, const char *quoteable);
char *util_sanitize_filename(char *filename);
char *util_sanitize_uri(const char *uri_str);
char *util_strcasestr(const char *haystack, const char *needle);
char *util_str_replace(const char* search, const char* replace, const char* string);
char *util_strescape(const char *source, const char *exceptions);
gboolean util_wildmatch(const char *pattern, const char *subject);
GTimeSpan util_string_to_timespan(const char *input);

#endif /* end of include guard: _UTIL_H */
