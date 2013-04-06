/**
 * vimb - a webkit based vim like browser.
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

#include "main.h"
#include "bookmark.h"

extern VbCore vb;

typedef struct {
    char *uri;
    char **argv;
    int  argc;
} Bookmark;

static GList *load(const char *file);
static gboolean contains_all_args(char **argv_src, unsigned int s, char **argv_query, unsigned int q);
static void free_bookmark(Bookmark *bm);

/**
 * Write a new bookmark entry to the end of bookmark file.
 */
void bookmark_add(const char *uri, const char *tags)
{
    char **parts, *t = NULL;
    unsigned int len, i;
    GString *string = g_string_new(uri);
    FILE *f;

    if ((f = fopen(vb.files[FILES_BOOKMARK], "a+"))) {
        if (tags) {
            /* splits the tags by space char */
            parts = g_strsplit(tags, " ", 0);
            len   = g_strv_length(parts);

            for (i = 0; i < len; i++) {
                t = g_shell_quote(parts[i]);
                g_string_append_printf(string, " %s", t);
                g_free(t);
            }
        }

        file_lock_set(fileno(f), F_WRLCK);

        fprintf(f, "%s\n", string->str);

        file_lock_set(fileno(f), F_UNLCK);
        fclose(f);

        g_string_free(string, true);
    }
}

/**
 * Retrieves all bookmark uri matching the given space separated tags string.
 * Don't forget to free the returned list.
 */
GList *bookmark_get_by_tags(const char *tags)
{
    GList *res = NULL;
    GList *src = load(vb.files[FILES_BOOKMARK]);
    char **parts;
    unsigned int len;

    parts = g_strsplit(tags, " ", 0);
    len   = g_strv_length(parts);

    for (GList *l = src; l; l = l->next) {
        Bookmark *bm = (Bookmark*)l->data;
        if (contains_all_args(bm->argv, bm->argc, parts, len)) {
            res = g_list_prepend(res, g_strdup(bm->uri));
        }
    }

    g_list_free_full(src, (GDestroyNotify)free_bookmark);
    g_strfreev(parts);

    return res;
}

static GList *load(const char *file)
{
    /* read the items from file */
    GList *list   = NULL;
    char buf[512] = {0}, **argv = NULL;
    int argc      = 0;
    FILE *f;

    if (!(f = fopen(file, "r"))) {
        return list;
    }

    file_lock_set(fileno(f), F_RDLCK);
    while (fgets(buf, sizeof(buf), f)) {
        char *p;
        Bookmark *bm;

        g_strstrip(buf);
        /* skip empty lines */
        if (!*buf) {
            continue;
        }

        /* create bookmark */
        bm = g_new(Bookmark, 1);
        if ((p = strchr(buf, ' '))) {
            *p = '\0';
            /* parse tags */
            if (g_shell_parse_argv(p + 1, &argc, &argv, NULL)) {
                bm->uri  = g_strdup(buf);
                bm->argv = argv;
                bm->argc = argc;
            } else {
                continue;
            }
        } else {
            bm->uri  = g_strdup(buf);
            bm->argv = NULL;
            bm->argc = 0;
        }

        list = g_list_prepend(list, bm);
    }
    file_lock_set(fileno(f), F_UNLCK);
    fclose(f);

    return list;
}

/**
 * Checks if the given source array of pointer contains all those entries
 * given as array of search strings.
 */
static gboolean contains_all_args(char **argv_src, unsigned int s, char **argv_query, unsigned int q)
{
    unsigned int i, n;

    if (!s || !q) {
        return true;
    }

    /* iterate over all query parts */
    for (i = 0; i < q; i++) {
        gboolean found = false;
        for (n = 0; n < s; n++) {
            if (!strcmp(argv_query[i], argv_src[n])) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }

    return true;
}

static void free_bookmark(Bookmark *bm)
{
    g_free(bm->uri);
    g_strfreev(bm->argv);
    g_free(bm);
}
