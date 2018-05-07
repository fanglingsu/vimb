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

#include <string.h>

#include "config.h"
#include "main.h"
#include "bookmark.h"
#include "util.h"
#include "completion.h"

typedef struct {
    char *uri;
    char *title;
    char *tags;
} Bookmark;

extern struct Vimb vb;

static GList *load(const char *file);
static Bookmark *line_to_bookmark(const char *uri, const char *data);
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
    gboolean removed = FALSE;

    if (!uri) {
        return FALSE;
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
                    removed = TRUE;
                    continue;
                } else {
                    /* reappend the tags */
                    *p = '\t';
                }
            }
            if (!strcmp(uri, line)) {
                removed = TRUE;
                continue;
            }
            g_string_append_printf(new, "%s\n", line);
        }
        g_strfreev(lines);
        util_file_set_content(vb.files[FILES_BOOKMARK], new->str);
        g_string_free(new, TRUE);
    }

    return removed;
}

gboolean bookmark_fill_url_completion(GtkListStore *store, gpointer data)
{
    Bookmark *bm;
    GList *src = NULL;
    GtkTreeIter iter;
    gboolean found = FALSE;

    src = load(vb.files[FILES_BOOKMARK]);
    src = g_list_reverse(src);
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
            COMPLETION_ADDITIONAL, bm->tags,
            -1
        );
        found = TRUE;
    }

    g_list_free_full(src, (GDestroyNotify)free_bookmark);

    return found;
}

gboolean bookmark_tag_completion_visible_func(GtkTreeModel *model,
        GtkTreeIter *iter, char **input)
{
    const char *separators;
    char *url, *tags, *cursor, **parts;
    unsigned int len, i;
    gboolean found;

    gtk_tree_model_get(model, iter, COMPLETION_STORE_FIRST, &url, COMPLETION_ADDITIONAL, &tags, -1);
    parts = g_strsplit(*input, " ", 0);
    len   = g_strv_length(parts);

    if (tags) {
        /* If there are tags - use them for matching. */
        separators = " ";
        cursor     = tags;
    } else {
        /* No tags available - matching is based on the path parts of the URL. */
        separators = "./";
        cursor     = url;
    }

    /* iterate over all parts */
    for (i = 0; i < len; i++) {
        found = FALSE;

        /* we want to do a prefix match on all bookmark tags - so we check for
         * a match on string begin - if this fails we move the cursor to the
         * next space and do the test again */
        while (cursor && *cursor) {
            if (g_str_has_prefix(cursor, parts[i])) {
                found = TRUE;
                break;
            }
            /* If match was not found at the cursor position - move cursor
             * behind the next separator char. */
            if ((cursor = strpbrk(cursor, separators))) {
                cursor++;
            }
        }

        if (!found) {
            break;
        }
    }

    g_strfreev(parts);
    g_free(tags);
    g_free(url);

    return found;
}

gboolean bookmark_fill_tag_completion(GtkListStore *store, gpointer data)
{
    GtkTreeIter iter;
    gboolean found = FALSE;
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
    for (GList *l = taglist; l; l = l->next) {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, COMPLETION_STORE_FIRST, l->data, -1);
        found = TRUE;
    }

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
    return util_file_pop_line(vb.files[FILES_QUEUE], item_count);
}

/**
 * Removes all contents from the queue file.
 */
gboolean bookmark_queue_clear(void)
{
    FILE *f;
    if ((f = fopen(vb.files[FILES_QUEUE], "w"))) {
        fclose(f);
        return TRUE;
    }
    return FALSE;
}
#endif /* FEATURE_QUEUE */

static GList *load(const char *file)
{
    return util_file_to_unique_list(file, (Util_Content_Func)line_to_bookmark, 0);
}

static Bookmark *line_to_bookmark(const char *uri, const char *data)
{
    char *p;
    Bookmark *bm;

    /* data part may consist of title or title<tab>tags */
    bm      = g_slice_new(Bookmark);
    bm->uri = g_strdup(uri);
    if (data && (p = strchr(data, '\t'))) {
        *p        = '\0';
        bm->title = g_strdup(data);
        bm->tags  = g_strdup(p + 1);
    } else {
        bm->title = g_strdup(data);
        bm->tags  = NULL;
    }

    return bm;
}

static void free_bookmark(Bookmark *bm)
{
    g_free(bm->uri);
    g_free(bm->title);
    g_free(bm->tags);
    g_slice_free(Bookmark, bm);
}

