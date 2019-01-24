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

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <JavaScriptCore/JavaScript.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>

#include "ascii.h"
#include "completion.h"
#include "util.h"

static struct {
    char    *config_dir;
} util;

extern struct Vimb vb;

static void create_dir_if_not_exists(const char *dirpath);
static gboolean match(const char *pattern, int patlen, const char *subject);
static gboolean match_list(const char *pattern, int patlen, const char *subject);

/**
 * Build the absolute file path of given path and possible given directory.
 *
 * Returned path must be freed.
 */
char *util_build_path(State state, const char *path, const char *dir)
{
    char *fullPath = NULL, *fexp, *dexp, *p;
    int expflags   = UTIL_EXP_TILDE|UTIL_EXP_DOLLAR;

    /* if the path could be expanded */
    if ((fexp = util_expand(state, path, expflags))) {
        if (*fexp == '/') {
            /* path is already absolute, no need to use given dir - there is
             * no need to free fexp, because this should be done by the caller
             * on fullPath later */
            fullPath = fexp;
        } else if (dir && *dir) {
            /* try to expand also the dir given - this may be ~/path */
            if ((dexp = util_expand(state, dir, expflags))) {
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
        g_critical("Could not create directory '%s': %s", dirpath, g_strerror(errno));

        return FALSE;
    }

    return TRUE;
}

/**
 * Creates a temporary file with given content.
 *
 * Upon success, and if file is non-NULL, the actual file path used is
 * returned in file. This string should be freed with g_free() when not
 * needed any longer.
 */
gboolean util_create_tmp_file(const char *content, char **file)
{
    int fp;
    ssize_t bytes, len;

    fp = g_file_open_tmp(PROJECT "-XXXXXX", file, NULL);
    if (fp == -1) {
        g_critical("Could not create temp file %s", *file);
        g_free(*file);
        return FALSE;
    }

    if (content == NULL) {
        close(fp);
        return TRUE;
    }

    len = strlen(content);

    /* write content into temporary file */
    bytes = write(fp, content, len);
    if (bytes < len) {
        close(fp);
        unlink(*file);
        g_critical("Could not write temp file %s", *file);
        g_free(*file);

        return FALSE;
    }
    close(fp);

    return TRUE;
}

/**
 * Expand ~user, ~/, $ENV and ${ENV} for given string into new allocated
 * string.
 *
 * Returned path must be g_freed.
 */
char *util_expand(State state, const char *src, int expflags)
{
    const char **input = &src;
    char *result;
    GString *dst = g_string_new("");
    int flags    = expflags;

    while (**input) {
        util_parse_expansion(state, input, dst, flags, "\\");
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
    gboolean res = FALSE;
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

        res = TRUE;
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
    util_file_set_content(file, new_content->str);
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
            util_file_set_content(file, new);
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
 * Atomicly writes contents to given file.
 * Returns TRUE on success, FALSE otherwise.
 */
gboolean util_file_set_content(const char *file, const char *contents)
{
    gboolean retval = FALSE;
    char *tmp_name;
    int fd, mode;
    gsize length;
    struct stat st;

    mode = 0600;
    if (stat(file, &st) == 0) {
        mode = st.st_mode;
    }

    /* Create a temporary file. */
    tmp_name = g_strconcat(file, ".XXXXXX", NULL);
    errno    = 0;
    fd       = g_mkstemp_full(tmp_name, O_RDWR, mode);
    length   = strlen(contents);

    if (fd == -1) {
        g_error("Failed to create file %s: %s", tmp_name, g_strerror(errno));

        goto out;
    }

    /* Write the contents to the temporary file. */
    while (length > 0) {
        gssize s;
        s = write(fd, contents, length);
        if (s < 0) {
            if (errno == EINTR) {
                continue;
            }
            g_error("Failed to write to file %s: write() failed: %s",
                    tmp_name, g_strerror(errno));
            close(fd);
            g_unlink(tmp_name);

            goto out;
        }

        g_assert (s <= length);

        contents += s;
        length   -= s;
    }

    if (!g_close(fd, NULL)) {
        g_unlink(tmp_name);
        goto out;
    }

    /* Atomic rename the temporary file into the destination file. */
    if (g_rename(tmp_name, file) == -1) {
        g_error("Failed to rename file %s to %s: g_rename() failed: %s",
                tmp_name, file, g_strerror(errno));
        g_unlink(tmp_name);
        goto out;
    }

    retval = TRUE;

out:
    g_free(tmp_name);

    return retval;
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
 * @mode:       Mode (file permission as chmod(2)) used for the file when creating it.
 */
char *util_get_filepath(const char *dir, const char *filename, gboolean create, int mode)
{
    char *fullpath;

    /* Built the full path out of config dir and given file name. */
    fullpath = g_build_filename(dir, filename, NULL);

    if (g_file_test(fullpath, G_FILE_TEST_IS_REGULAR)) {
        return fullpath;
    } else if (create) {
        /* If create option was given - create the file. */
        fclose(fopen(fullpath, "a"));
        g_chmod(fullpath, mode);
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
    gboolean found = FALSE;
    GtkTreeIter iter;

    if (!input || !*input) {
        for (GList *l = src; l; l = l->next) {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, COMPLETION_STORE_FIRST, l->data, -1);
            found = TRUE;
        }
    } else {
        for (GList *l = src; l; l = l->next) {
            char *value = (char*)l->data;
            if (g_str_has_prefix(value, input)) {
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter, COMPLETION_STORE_FIRST, l->data, -1);
                found = TRUE;
            }
        }
    }

    return found;
}

/**
 * Fills file path completion entries into given list store for also given
 * input.
 */
gboolean util_filename_fill_completion(State state, GtkListStore *store, const char *input)
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
        state,
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

/**
 * Returns the script result as string.
 * Returned string must be freed by g_free.
 */
char *util_js_result_as_string(WebKitJavascriptResult *result)
{
#if WEBKIT_CHECK_VERSION (2, 22, 0)
	return jsc_value_to_string(webkit_javascript_result_get_js_value(result));
#else
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
#endif
}

/**
 * Reads given input and try to parse ~/, ~user, $VAR or ${VAR} expansion
 * from the start of the input and moves the input pointer to the first
 * not expanded char. If no expansion pattern was found, the first char is
 * appended to given GString.
 *
 * Please note that for a single ~, g_get_home_dir() is used and if a valid
 * HOME environment variable is set it is preferred than passwd file.
 * However, for ~user expansion the passwd file is always used.
 *
 * @input:     String pointer with the content to be parsed.
 * @str:       GString that will be filled with expanded content.
 * @flags      Flags that determine which expansion are processed.
 * @quoteable: String of chars that are additionally escapable by \.
 * Returns TRUE if input started with expandable pattern.
 */
gboolean util_parse_expansion(State state, const char **input, GString *str,
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
        if (state.uri) {
            /* TODO check for modifiers like :h:t:r:e */
            g_string_append(str, state.uri);
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

/**
 * Strips password from a uri.
 *
 * Return newly allocated string.
 */
char *util_sanitize_uri(const char *uri_str)
{
    SoupURI *uri;
    char *sanitized_uri;

    /* Sanitize the uri only in case there is a @ which might be the indicator
     * for credentials used in uri. */
    if (!strchr(uri_str, '@')) {
        return g_strdup(uri_str);
    }

    uri = soup_uri_new(uri_str);
    sanitized_uri = soup_uri_to_string(uri, FALSE);
    soup_uri_free(uri);

    return sanitized_uri;
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
char *util_str_replace(const char *search, const char *replace, const char *string)
{
    if (!string) {
        return NULL;
    }

    char **buf = g_strsplit(string, search, -1);
    char *ret  = g_strjoinv(replace, buf);
    g_strfreev(buf);

    return ret;
}

/**
 * Escapes some special characters in the source string by inserting a '\'
 * before them. Acts like g_strescape() but does not demage utf8 chars.
 * Returns a newly allocated string.
 */
char *util_strescape(const char *source, const char *exceptions)
{
    GString *result = g_string_new(NULL);
    while (TRUE) {
        char c = *source++;
        if ('\0' == c) {
            goto done;
        }
        if (exceptions && !strchr(exceptions, c)) {
            continue;
        }
        switch (c) {
            case '\n':
                g_string_append(result, "\\n");
                break;
            case '\"':
                g_string_append(result, "\\\"");
                break;
            case '\\':
                g_string_append(result, "\\\\");
                break;
            case '\b':
                g_string_append(result, "\\b");
                break;
            case '\f':
                g_string_append(result, "\\f");
                break;
            case '\r':
                g_string_append(result, "\\r");
                break;
            case '\t':
                g_string_append(result, "\\t");
                break;
            default:
                g_string_append_c(result, c);
        }
    }
done:
    return g_string_free(result, FALSE);
}

static void create_dir_if_not_exists(const char *dirpath)
{
    if (!g_file_test(dirpath, G_FILE_TEST_IS_DIR)) {
        g_mkdir_with_parents(dirpath, 0755);
    }
}

/**
 * Compares given string against also given list of patterns.
 *
 * *         Matches any sequence of characters.
 * ?         Matches any single character except of '/'.
 * {foo,bar} Matches foo or bar - '{', ',' and '}' within this pattern must be
 *           escaped by '\'. '*' and '?' have no special meaning within the
 *           curly braces.
 * *?{}      these chars must always be escaped by '\' to match them literally
 */
gboolean util_wildmatch(const char *pattern, const char *subject)
{
    const char *end;
    int braces, patlen, count;

    /* loop through all pattens */
    for (count = 0; *pattern; pattern = (*end == ',' ? end + 1 : end), count++) {
        /* find end of the pattern - but be careful with comma in curly braces */
        braces = 0;
        for (end = pattern; *end && (*end != ',' || braces || *(end - 1) == '\\'); ++end) {
            if (*end == '{') {
                braces++;
            } else if (*end == '}') {
                braces--;
            }
        }
        /* ignore single comma */
        if (*pattern == *end) {
            continue;
        }
        /* calculate the length of the pattern */
        patlen = end - pattern;

        /* if this pattern matches - return */
        if (match(pattern, patlen, subject)) {
            return true;
        }
    }

    if (!count) {
        /* empty pattern matches only on empty subject */
        return !*subject;
    }
    /* there where one or more patterns but none of them matched */
    return false;
}


/**
 * Compares given subject string against the given pattern.
 * The pattern needs not to bee NUL terminated.
 */
static gboolean match(const char *pattern, int patlen, const char *subject)
{
    int i;
    char sl, pl;

    while (patlen > 0) {
        switch (*pattern) {
            case '?':
                /* '?' matches a single char except of / and subject end */
                if (*subject == '/' || !*subject) {
                    return false;
                }
                break;

            case '*':
                /* easiest case - the '*' ist the last char in pattern - this
                 * will always match */
                if (patlen == 1) {
                    return true;
                }
                /* Try to match as much as possible. Try to match the complete
                 * uri, if that fails move forward in uri and check for a
                 * match. */
                i = strlen(subject);
                while (i >= 0 && !match(pattern + 1, patlen - 1, subject + i)) {
                    i--;
                }
                return i >= 0;

            case '}':
                /* spurious '}' in pattern */
                return false;

            case '{':
                /* possible {foo,bar} pattern */
                return match_list(pattern, patlen, subject);

            case '\\':
                /* '\' escapes next special char */
                if (strchr("*?{}", pattern[1])) {
                    pattern++;
                    patlen--;
                    if (*pattern != *subject) {
                        return false;
                    }
                }
                break;

            default:
                /* compare case insensitive */
                sl = *subject;
                if (VB_IS_UPPER(sl)) {
                    sl += 'a' - 'A';
                }
                pl = *pattern;
                if (VB_IS_UPPER(pl)) {
                    pl += 'a' - 'A';
                }
                if (sl != pl) {
                    return false;
                }
                break;
        }
        /* do another loop run with next pattern and subject char */
        pattern++;
        patlen--;
        subject++;
    }

    /* on end of pattern only a also ended subject is a match */
    return !*subject;
}

/**
 * Matches pattern starting with '{'.
 * This function can process also on none null terminated pattern.
 */
static gboolean match_list(const char *pattern, int patlen, const char *subject)
{
    int endlen;
    const char *end, *s;

    /* finde the next none escaped '}' */
    for (end = pattern, endlen = patlen; endlen > 0 && *end != '}'; end++, endlen--) {
        /* if escape char - move pointer one additional step */
        if (*end == '\\') {
            end++;
            endlen--;
        }
    }

    if (!*end) {
        /* unterminated '{' in pattern */
        return false;
    }

    s = subject;
    end++;      /* skip over } */
    endlen--;
    pattern++;  /* skip over { */
    patlen--;
    while (true) {
        switch (*pattern) {
            case ',':
                if (match(end, endlen, s)) {
                    return true;
                }
                s = subject;
                pattern++;
                patlen--;
                break;

            case '}':
                return match(end, endlen, s);

            case '\\':
                if (pattern[1] == ',' || pattern[1] == '}' || pattern[1] == '{') {
                    pattern++;
                    patlen--;
                }
                /* fall through */

            default:
                if (*pattern == *s) {
                    pattern++;
                    patlen--;
                    s++;
                } else {
                    /* this item of the list does not match - move forward to
                     * the next none escaped ',' or '}' */
                    s = subject;
                    for (s = subject; *pattern != ',' && *pattern != '}'; pattern++, patlen--) {
                        /* if escape char is found - skip next char */
                        if (*pattern == '\\') {
                            pattern++;
                            patlen--;
                        }
                    }
                    /* found ',' skip over it to check the next list item */
                    if (*pattern == ',') {
                        pattern++;
                        patlen--;
                    }
                }
        }
    }
}

