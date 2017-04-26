/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2016 Daniel Carl
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

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <JavaScriptCore/JavaScript.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>

#include "ascii.h"
#include "completion.h"
#include "util.h"

static struct {
    char    *config_dir;
} util;

extern struct Vimb vb;

static void create_dir_if_not_exists(const char *dirpath);

/**
 * Build the absolute file path of given path and possible given directory.
 *
 * Returned path must be freed.
 */
char *util_build_path(Client *c, const char *path, const char *dir)
{
    char *fullPath = NULL, *fexp, *dexp, *p;
    int expflags   = UTIL_EXP_TILDE|UTIL_EXP_DOLLAR;

    /* if the path could be expanded */
    if ((fexp = util_expand(c, path, expflags))) {
        if (*fexp == '/') {
            /* path is already absolute, no need to use given dir - there is
             * no need to free fexp, bacuse this should be done by the caller
             * on fullPath later */
            fullPath = fexp;
        } else if (dir && *dir) {
            /* try to expand also the dir given - this may be ~/path */
            if ((dexp = util_expand(c, dir, expflags))) {
                /* use expanded dir and append expanded path */
                fullPath = g_build_filename(dexp, fexp, NULL);
                g_free(dexp);
            }
            g_free(fexp);
        }
    }

    /* if full path not found use current dir */
    if (!fullPath) {
        fullPath = g_build_filename(g_get_current_dir(), path, NULL);
    }

    /* Create the directory part of the path if it does not exists. */
    if ((p = strrchr(fullPath, '/'))) {
        gboolean res;
        *p = '\0';
        res = util_create_dir_if_not_exists(fullPath);
        *p = '/';

        if (!res) {
            g_free(fullPath);

            return NULL;
        }
    }

    return fullPath;
}

/**
 * Free memory for allocated path strings.
 */
void util_cleanup(void)
{
    if (util.config_dir) {
        g_free(util.config_dir);
    }
}

gboolean util_create_dir_if_not_exists(const char *dirpath)
{
    if (g_mkdir_with_parents(dirpath, 0755) == -1) {
        g_critical("Could not create directory '%s': %s", dirpath, strerror(errno));

        return FALSE;
    }

    return TRUE;
}

/**
 * Expand ~user, ~/, $ENV and ${ENV} for given string into new allocated
 * string.
 *
 * Returned path must be g_freed.
 */
char *util_expand(Client *c, const char *src, int expflags)
{
    const char **input = &src;
    char *result;
    GString *dst = g_string_new("");
    int flags    = expflags;

    while (**input) {
        util_parse_expansion(c, input, dst, flags, "\\");
        if (VB_IS_SEPARATOR(**input)) {
            /* after space the tilde expansion is allowed */
            flags = expflags;
        } else {
            /* remove tile expansion for next loop */
            flags &= ~UTIL_EXP_TILDE;
        }
        /* move pointer to the next char */
        (*input)++;
    }

    result = dst->str;
    g_string_free(dst, FALSE);

    return result;
}

/**
 * Append new data to file.
 *
 * @file:   File to append the data
 * @format: Format string used to process va_list
 */
gboolean util_file_append(const char *file, const char *format, ...)
{
    va_list args;
    FILE *f;

    if (file && (f = fopen(file, "a+"))) {
        flock(fileno(f), LOCK_EX);

        va_start(args, format);
        vfprintf(f, format, args);
        va_end(args);

        flock(fileno(f), LOCK_UN);
        fclose(f);

        return TRUE;
    }
    return FALSE;
}

/**
 * Prepend new data to file.
 *
 * @file:   File to prepend the data
 * @format: Format string used to process va_list
 */
gboolean util_file_prepend(const char *file, const char *format, ...)
{
    gboolean res = false;
    va_list args;
    char *content;
    FILE *f;
    if (!file) {
        return FALSE;
    }

    content = util_get_file_contents(file, NULL);
    if ((f = fopen(file, "w"))) {
        flock(fileno(f), LOCK_EX);

        va_start(args, format);
        /* write new content to the file */
        vfprintf(f, format, args);
        va_end(args);

        /* append previous file content */
        fputs(content, f);

        flock(fileno(f), LOCK_UN);
        fclose(f);

        res = true;
    }
    g_free(content);

    return res;
}

/**
 * Prepend a new line to the file and make sure there are not more than
 * max_lines in the file.
 *
 * @file:       File to prepend the data
 * @line:       Line to be written as new first line into the file.
 *              The line ending is inserted automatic.
 * @max_lines   Maximum number of lines in file after the operation.
 */
void util_file_prepend_line(const char *file, const char *line,
        unsigned int max_lines)
{
    char **lines;
    GString *new_content;

    g_assert(file);
    g_assert(line);

    lines = util_get_lines(file);
    /* Write first the new line into the string and append the new line. */
    new_content = g_string_new(line);
    g_string_append(new_content, "\n");
    if (lines) {
        int len, i;

        len = g_strv_length(lines);
        for (i = 0; i < len - 1 && i < max_lines - 1; i++) {
            g_string_append_printf(new_content, "%s\n", lines[i]);
        }
        g_strfreev(lines);
    }
    g_file_set_contents(file, new_content->str, -1, NULL);
    g_string_free(new_content, TRUE);
}

/**
 * Retrieves the first line from file and delete it from file.
 *
 * @file:       file to read from
 * @item_count: will be filled with the number of remaining lines in file if it
 *              is not NULL.
 *
 * Returned string must be freed with g_free.
 */
char *util_file_pop_line(const char *file, int *item_count)
{
    char **lines;
    char *new,
         *line = NULL;
    int len,
        count = 0;

    if (!file) {
        return NULL;
    }
    lines = util_get_lines(file);

    if (lines) {
        len = g_strv_length(lines);
        if (len) {
            line = g_strdup(lines[0]);
            /* minus one for last empty item and one for popped item */
            count = len - 2;
            new   = g_strjoinv("\n", lines + 1);
            g_file_set_contents(file, new, -1, NULL);
            g_free(new);
        }
        g_strfreev(lines);
    }

    if (item_count) {
        *item_count = count;
    }

    return line;
}

/**
 * Retrieves the config directory path according to current used profile.
 * Returned string must be freed.
 */
char *util_get_config_dir(void)
{
    char *path = g_build_filename(g_get_user_config_dir(), PROJECT, vb.profile, NULL);
    create_dir_if_not_exists(path);

    return path;
}

/**
 * Retrieves the length bytes from given file.
 *
 * The memory of returned string have to be freed with g_free().
 */
char *util_get_file_contents(const char *filename, gsize *length)
{
    GError *error = NULL;
    char *content = NULL;

    if (filename && !g_file_get_contents(filename, &content, length, &error)) {
        g_warning("Cannot open %s: %s", filename, error->message);
        g_error_free(error);
    }
    return content;
}

/**
 * Buil the path from given directory and filename and checks if the file
 * exists. If the file does not exists and the create option is not set, this
 * function returns NULL.
 * If the file exists or the create option was given the full generated path
 * is returned as newly allocated string.
 *
 * The return value must be freed with g_free.
 *
 * @dir:        Directory in which the file is searched.
 * @filename:   Filename to built the absolute path with.
 * @create:     If TRUE, the file is created if it does not already exist.
 */
char *util_get_filepath(const char *dir, const char *filename, gboolean create)
{
    char *fullpath;

    /* Built the full path out of config dir and given file name. */
    fullpath = g_build_filename(util_get_config_dir(), filename, NULL);

    if (g_file_test(fullpath, G_FILE_TEST_IS_REGULAR)) {
        return fullpath;
    } else if (create) {
        /* If create option was given - create the file. */
        fclose(fopen(fullpath, "a"));
        return fullpath;
    }

    g_free(fullpath);
    return NULL;
}


/**
 * Retrieves the file content as lines.
 *
 * The result have to be freed by g_strfreev().
 */
char **util_get_lines(const char *filename)
{
    char *content;
    char **lines  = NULL;

    if (!filename) {
        return NULL;
    }

    if ((content = util_get_file_contents(filename, NULL))) {
        /* split the file content into lines */
        lines = g_strsplit(content, "\n", -1);
        g_free(content);
    }
    return lines;
}

/**
 * Retrieves a list with unique items from file. The uniqueness is calculated
 * based on the lines comparing all chars until the next <tab> char or end of
 * line.
 *
 * @filename:    file to read items from
 * @func:        Function to parse a single line to item.
 * @max_items:   maximum number of items that are returned, use 0 for
 *               unlimited items
 */
GList *util_file_to_unique_list(const char *filename, Util_Content_Func func,
        guint max_items)
{
    char *line, **lines;
    int i, len;
    GList *gl = NULL;
    GHashTable *ht;

    lines = util_get_lines(filename);
    if (!lines) {
        return NULL;
    }

    /* Use the hashtable to check for duplicates in a faster way than by
     * iterating over the generated list itself. So it's enough to store the
     * the keys only. */
    ht = g_hash_table_new(g_str_hash, g_str_equal);

    /* Begin with the last line of the file to make unique check easier -
     * every already existing item in the table is the latest, so we don't need
     * to do anything if an item already exists in the hash table. */
    len = g_strv_length(lines);
    for (i = len - 1; i >= 0; i--) {
        char *key, *data;
        void *item;

        line = lines[i];
        g_strstrip(line);
        if (!*line) {
            continue;
        }

        /* if line contains tab char - separate the line at this */
        if ((data = strchr(line, '\t'))) {
            *data = '\0';
            key   = line;
            data++;
        } else {
            key  = line;
            data = NULL;
        }

        /* Check if the entry is already in the result list. */
        if (g_hash_table_lookup_extended(ht, key, NULL, NULL)) {
            continue;
        }

        /* Item is new - prepend it to the list. Because the record are read
         * in reverse order the prepend generates a list in the right order. */
        if ((item = func(key, data))) {
            g_hash_table_insert(ht, key, NULL);
            gl = g_list_prepend(gl, item);

            /* Don't put more entries into the list than requested. */
            if (max_items && g_hash_table_size(ht) >= max_items) {
                break;
            }
        }
    }

    g_strfreev(lines);
    g_hash_table_destroy(ht);

    return gl;
}

/**
 * Fills the given list store by matching data of also given src list.
 */
gboolean util_fill_completion(GtkListStore *store, const char *input, GList *src)
{
    gboolean found = false;
    GtkTreeIter iter;

    if (!input || !*input) {
        for (GList *l = src; l; l = l->next) {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, COMPLETION_STORE_FIRST, l->data, -1);
            found = true;
        }
    } else {
        for (GList *l = src; l; l = l->next) {
            char *value = (char*)l->data;
            if (g_str_has_prefix(value, input)) {
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter, COMPLETION_STORE_FIRST, l->data, -1);
                found = true;
            }
        }
    }

    return found;
}

/**
 * Fills file path completion entries into given list store for also given
 * input.
 */
gboolean util_filename_fill_completion(Client *c, GtkListStore *store, const char *input)
{
    gboolean found = FALSE;
    GError *error  = NULL;
    char *input_dirname, *real_dirname;
    const char *last_slash, *input_basename;
    GDir *dir;

    last_slash     = strrchr(input, '/');
    input_basename = last_slash ? last_slash + 1 : input;
    input_dirname  = g_strndup(input, input_basename - input);
    real_dirname   = util_expand(
        c,
        *input_dirname ? input_dirname : ".",
        UTIL_EXP_TILDE|UTIL_EXP_DOLLAR|UTIL_EXP_SPECIAL
    );

    dir = g_dir_open(real_dirname, 0, &error);
    if (error) {
        /* Can't open directory, likely bad user input */
        g_error_free(error);
    } else {
        GtkTreeIter iter;
        const char *filename;
        char *fullpath, *result;

        while ((filename = g_dir_read_name(dir))) {
            if (g_str_has_prefix(filename, input_basename)) {
                fullpath = g_build_filename(real_dirname, filename, NULL);
                if (g_file_test(fullpath, G_FILE_TEST_IS_DIR)) {
                    result = g_strconcat(input_dirname, filename, "/", NULL);
                } else {
                    result = g_strconcat(input_dirname, filename, NULL);
                }
                g_free(fullpath);
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter, COMPLETION_STORE_FIRST, result, -1);
                g_free(result);
                found = TRUE;
            }
        }
        g_dir_close(dir);
    }

    g_free(input_dirname);
    g_free(real_dirname);

    return found;
}

char *util_js_result_as_string(WebKitJavascriptResult *result)
{
    JSValueRef value;
    JSStringRef string;
    size_t len;
    char *retval = NULL;

    value  = webkit_javascript_result_get_value(result);
    string = JSValueToStringCopy(webkit_javascript_result_get_global_context(result),
                value, NULL);

    len = JSStringGetMaximumUTF8CStringSize(string);
    if (len) {
        retval = g_malloc(len);
        JSStringGetUTF8CString(string, retval, len);
    }
    JSStringRelease(string);

    return retval;
}

double util_js_result_as_number(WebKitJavascriptResult *result)
{
    JSValueRef value = webkit_javascript_result_get_value(result);

    return JSValueToNumber(webkit_javascript_result_get_global_context(result), value,
        NULL);
}

/**
 * Reads given input and try to parse ~/, ~user, $VAR or ${VAR} expansion
 * from the start of the input and moves the input pointer to the first
 * not expanded char. If no expansion pattern was found, the first char is
 * appended to given GString.
 *
 * @input:     String pointer with the content to be parsed.
 * @str:       GString that will be filled with expanded content.
 * @flags      Flags that determine which expansion are processed.
 * @quoteable: String of chars that are additionally escapable by \.
 * Returns TRUE if input started with expandable pattern.
 */
gboolean util_parse_expansion(Client *c, const char **input, GString *str,
        int flags, const char *quoteable)
{
    GString *name;
    const char *env, *prev, quote = '\\';
    struct passwd *pwd;
    gboolean expanded = FALSE;

    prev = *input;
    if (flags & UTIL_EXP_TILDE && **input == '~') {
        /* skip ~ */
        (*input)++;

        if (**input == '/') {
            g_string_append(str, g_get_home_dir());
            expanded = TRUE;
            /* if there is no char or space after ~/ skip the / to get
             * /home/user instead of /home/user/ */
            if (!*(*input + 1) || VB_IS_SPACE(*(*input + 1))) {
                (*input)++;
            }
        } else {
            /* look ahead to / space or end of string to get a possible
             * username for ~user pattern */
            name = g_string_new("");
            /* current char is ~ that is skipped to get the user name */
            while (VB_IS_IDENT(**input)) {
                g_string_append_c(name, **input);
                (*input)++;
            }
            /* append the name to the destination string */
            if ((pwd = getpwnam(name->str))) {
                g_string_append(str, pwd->pw_dir);
                expanded = TRUE;
            }
            g_string_free(name, TRUE);
        }
        /* move pointer back to last expanded char */
        (*input)--;
    } else if (flags & UTIL_EXP_DOLLAR && **input == '$') {
        /* skip the $ */
        (*input)++;

        name = g_string_new("");
        /* look for ${VAR}*/
        if (**input == '{') {
            /* skip { */
            (*input)++;

            /* look ahead to } or end of string */
            while (**input && **input != '}') {
                g_string_append_c(name, **input);
                (*input)++;
            }
            /* if the } was reached - skip this */
            if (**input == '}') {
                (*input)++;
            }
        } else { /* process $VAR */
            /* look ahead to /, space or end of string */
            while (VB_IS_IDENT(**input)) {
                g_string_append_c(name, **input);
                (*input)++;
            }
        }
        /* append the variable to the destination string */
        if ((env = g_getenv(name->str))) {
            g_string_append(str, env);
        }
        /* move pointer back to last expanded char */
        (*input)--;
        /* variable are expanded even if they do not exists */
        expanded = TRUE;
        g_string_free(name, TRUE);
    } else if (flags & UTIL_EXP_SPECIAL && **input == '%') {
        if (*c->state.uri) {
            /* TODO check for modifiers like :h:t:r:e */
            g_string_append(str, c->state.uri);
            expanded = TRUE;
        }
    }

    if (!expanded) {
        /* restore the pointer position if no expansion was found */
        *input = prev;

        /* handle escaping of quoteable chars */
        if (**input == quote) {
            /* move pointer to the next char */
            (*input)++;
            if (!**input) {
                /* if input ends here - use only the quote char */
                g_string_append_c(str, quote);
                (*input)--;
            } else if (strchr(quoteable, **input)
                || (flags & UTIL_EXP_TILDE && **input == '~')
                || (flags & UTIL_EXP_DOLLAR && **input == '$')
                || (flags & UTIL_EXP_SPECIAL && **input == '%')
            ) {
                /* escaped char becomes only char */
                g_string_append_c(str, **input);
            } else {
                /* put escape char and next char into the result string */
                g_string_append_c(str, quote);
                g_string_append_c(str, **input);
            }
        } else {
            /* take the char like it is */
            g_string_append_c(str, **input);
        }
    }

    return expanded;
}

/**
 * Sanituze filename by removeing directory separator by underscore.
 *
 * The string is modified in place.
 */
char *util_sanitize_filename(char *filename)
{
    return g_strdelimit(filename, G_DIR_SEPARATOR_S, '_');
}

char *util_strcasestr(const char *haystack, const char *needle)
{
    guchar c1, c2;
    int i, j;
    int nlen = strlen(needle);
    int hlen = strlen(haystack) - nlen + 1;

    for (i = 0; i < hlen; i++) {
        for (j = 0; j < nlen; j++) {
            c1 = haystack[i + j];
            c2 = needle[j];
            if (toupper(c1) != toupper(c2)) {
                goto next;
            }
        }
        return (char*)haystack + i;
next:
        ;
    }
    return NULL;
}

/**
 * Replaces appearances of search in string by given replace.
 * Returns a new allocated string if search was found.
 */
char *util_str_replace(const char* search, const char* replace, const char* string)
{
    if (!string) {
        return NULL;
    }

    char **buf = g_strsplit(string, search, -1);
    char *ret  = g_strjoinv(replace, buf);
    g_strfreev(buf);

    return ret;
}

static void create_dir_if_not_exists(const char *dirpath)
{
    if (!g_file_test(dirpath, G_FILE_TEST_IS_DIR)) {
        g_mkdir_with_parents(dirpath, 0755);
    }
}
