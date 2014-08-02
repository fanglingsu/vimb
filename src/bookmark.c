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

#include "config.h"
#include "main.h"
#include "bookmark.h"
#include "util.h"
#include "completion.h"

extern VbCore vb;

typedef struct {
    char *uri;
    char *title;
    char *tags;
} Bookmark;

static GList *load(const char *file);
static gboolean bookmark_contains_all_tags(Bookmark *bm, char **query,
    unsigned int qlen);
static Bookmark *line_to_bookmark(const char *line);
static int bookmark_comp(Bookmark *a, Bookmark *b);
static void free_bookmark(Bookmark *bm);

/**
 * Write a new bookmark entry to the end of bookmark file.
 */
gboolean bookmark_add(const char *uri, const char *title, const char *tags)
{
    const char *file = vb.files[FILES_BOOKMARK];
    if (tags) {
        return util_file_append(file, "%s\t%s\t%s\n", uri, title ? title : "", tags);
    }
    if (title) {
        return util_file_append(file, "%s\t%s\n", uri, title);
    }
    return util_file_append(file, "%s\n", uri);
}

gboolean bookmark_remove(const char *uri)
{
    char **lines, *line, *p;
    int len, i;
    GString *new;
    gboolean removed = false;

    if (!uri) {
        return false;
    }

    lines = util_get_lines(vb.files[FILES_BOOKMARK]);
    if (lines) {
        new = g_string_new(NULL);
        len = g_strv_length(lines) - 1;
        for (i = 0; i < len; i++) {
            line = lines[i];
            g_strstrip(line);
            /* ignore the title or bookmark tags and test only the uri */
            if ((p = strchr(line, '\t'))) {
                *p = '\0';
                if (!strcmp(uri, line)) {
                    removed = true;
                    continue;
                } else {
                    /* reappend the tags */
                    *p = '\t';
                }
            }
            if (!strcmp(uri, line)) {
                removed = true;
                continue;
            }
            g_string_append_printf(new, "%s\n", line);
        }
        g_strfreev(lines);
        g_file_set_contents(vb.files[FILES_BOOKMARK], new->str, -1, NULL);
        g_string_free(new, true);
    }

    return removed;
}

gboolean bookmark_fill_completion(GtkListStore *store, const char *input)
{
    gboolean found = false;
    char **parts;
    unsigned int len;
    GtkTreeIter iter;
    GList *src = NULL;
    Bookmark *bm;

    src = load(vb.files[FILES_BOOKMARK]);
    src = g_list_reverse(src);
    if (!input || !*input) {
        /* without any tags return all bookmarked items */
        for (GList *l = src; l; l = l->next) {
            bm = (Bookmark*)l->data;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(
                store, &iter,
                COMPLETION_STORE_FIRST, bm->uri,
#ifdef FEATURE_TITLE_IN_COMPLETION
                COMPLETION_STORE_SECOND, bm->title,
#endif
                -1
            );
            found = true;
        }
    } else {
        parts = g_strsplit(input, " ", 0);
        len   = g_strv_length(parts);

        for (GList *l = src; l; l = l->next) {
            bm = (Bookmark*)l->data;
            if (bookmark_contains_all_tags(bm, parts, len)) {
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(
                    store, &iter,
                    COMPLETION_STORE_FIRST, bm->uri,
#ifdef FEATURE_TITLE_IN_COMPLETION
                    COMPLETION_STORE_SECOND, bm->title,
#endif
                    -1
                );
                found = true;
            }
        }
        g_strfreev(parts);
    }

    g_list_free_full(src, (GDestroyNotify)free_bookmark);

    return found;
}

gboolean bookmark_fill_tag_completion(GtkListStore *store, const char *input)
{
    gboolean found;
    unsigned int len, i;
    char **tags, *tag;
    GList *src = NULL, *taglist = NULL;
    Bookmark *bm;

    /* get all distinct tags from bookmark file */
    src = load(vb.files[FILES_BOOKMARK]);
    for (GList *l = src; l; l = l->next) {
        bm = (Bookmark*)l->data;
        /* if bookmark contains no tags we can go to the next bookmark */
        if (!bm->tags) {
            continue;
        }

        tags = g_strsplit(bm->tags, " ", -1);
        len  = g_strv_length(tags);
        for (i = 0; i < len; i++) {
            tag = tags[i];
            /* add tag only if it isn't already in the list */
            if (!g_list_find_custom(taglist, tag, (GCompareFunc)strcmp)) {
                taglist = g_list_prepend(taglist, g_strdup(tag));
            }
        }
        g_strfreev(tags);
    }

    /* generate the completion with the found tags */
    found = util_fill_completion(store, input, taglist);

    g_list_free_full(src, (GDestroyNotify)free_bookmark);
    g_list_free_full(taglist, (GDestroyNotify)g_free);

    return found;
}

#ifdef FEATURE_QUEUE
/**
 * Push a uri to the end of the queue.
 *
 * @uri: URI to put into the queue
 */
gboolean bookmark_queue_push(const char *uri)
{
    return util_file_append(vb.files[FILES_QUEUE], "%s\n", uri);
}

/**
 * Push a uri to the beginning of the queue.
 *
 * @uri: URI to put into the queue
 */
gboolean bookmark_queue_unshift(const char *uri)
{
    return util_file_prepend(vb.files[FILES_QUEUE], "%s\n", uri);
}

/**
 * Retrieves the oldest entry from queue.
 *
 * @item_count: will be filled with the number of remaining items in queue.
 * Returned uri must be freed with g_free.
 */
char *bookmark_queue_pop(int *item_count)
{
    int len, i;
    char **lines, *uri = NULL;

    lines = util_get_lines(vb.files[FILES_QUEUE]);
    if (lines && (len = g_strv_length(lines))) {
        GString *new = g_string_new(NULL);

        uri = g_strdup(lines[0]);
        /* don't process last empty line */
        len -= 1;
        /* skip the first list that should be removed */
        for (i = 1; i < len; i++) {
            g_string_append_printf(new, "%s\n", lines[i]);
        }
        g_strfreev(lines);
        g_file_set_contents(vb.files[FILES_QUEUE], new->str, -1, NULL);
        g_string_free(new, true);

        *item_count = len - 1;
    } else {
        *item_count = 0;
    }
    return uri;
}

/**
 * Removes all contents from the queue file.
 */
gboolean bookmark_queue_clear(void)
{
    FILE *f;
    if ((f = fopen(vb.files[FILES_QUEUE], "w"))) {
        fclose(f);
        return true;
    }
    return false;
}
#endif /* FEATURE_QUEUE */

static GList *load(const char *file)
{
    return util_file_to_unique_list(
        file, (Util_Content_Func)line_to_bookmark, (GCompareFunc)bookmark_comp,
        (GDestroyNotify)free_bookmark, vb.config.history_max
    );
}

/**
 * Checks if the given bookmark have all given query strings as prefix.
 *
 * @bm:    bookmark to test if it matches
 * @query: char array with tags to search for
 * @qlen:  length of given query char array
 *
 * Return: true if the bookmark contained all tags
 */
static gboolean bookmark_contains_all_tags(Bookmark *bm, char **query,
    unsigned int qlen)
{
    unsigned int i;
    gboolean found;

    if (!qlen) {
        return true;
    }
    /* don't use bookmarks without tags if tags are used to filter */
    if (!bm->tags) {
        return false;
    }

    /* iterate over all query parts */
    for (i = 0; i < qlen; i++) {
        /* put the cursor to the tags string of the bookmarks */
        char *cursor = bm->tags;
        found        = false;

        /* we want to do a prefix match on all bookmark tags - so we check for
         * a match on string begin - if this fails we move the cursor to the
         * next space and do the test again */
        while (cursor && *cursor) {
            /* match was not found at current cursor position */
            if (g_str_has_prefix(cursor, query[i])) {
                found = true;
                break;
            }
            /* if match was not found at the cursor position - move cursor
             * behind the next space */
            if ((cursor = strchr(cursor, ' '))) {
                cursor++;
            }
        }

        if (!found) {
            return false;
        }
    }

    return true;
}

static Bookmark *line_to_bookmark(const char *line)
{
    char **parts;
    int len;
    Bookmark *bm;
    while (g_ascii_isspace(*line)) {
        line++;
    }
    if (!*line) {
        return NULL;
    }

    parts = g_strsplit(line, "\t", 3);
    len   = g_strv_length(parts);

    bm        = g_slice_new(Bookmark);
    bm->uri   = g_strdup(parts[0]);
    bm->tags  = NULL;
    bm->title = NULL;
    if (len == 3) {
        bm->title = g_strdup(parts[1]);
        bm->tags  = g_strdup(parts[2]);
    } else if (len == 2) {
        bm->title = g_strdup(parts[1]);
    }
    g_strfreev(parts);

    return bm;
}

static int bookmark_comp(Bookmark *a, Bookmark *b)
{
    return g_strcmp0(a->uri, b->uri);
}

static void free_bookmark(Bookmark *bm)
{
    g_free(bm->uri);
    g_free(bm->title);
    g_free(bm->tags);
    g_slice_free(Bookmark, bm);
}
