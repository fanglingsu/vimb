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

#include <fcntl.h>
#include <glib.h>
#include <sys/file.h>

#include "ascii.h"
#include "completion.h"
#include "config.h"
#include "history.h"
#include "main.h"
#include "util.h"
#include "file-storage.h"

#define HIST_STORAGE(t) (vb.storage[storage_map[t]])
typedef struct {
    char *first;
    char *second;
} History;

static gboolean history_item_contains_all_tags(History *item, char **query, guint qlen);
static void free_history(History *item);
static History *line_to_history(const char *uri, const char *title);
static GList *load(FileStorage *s);
static void write_to_file(GList *list, const char *file);

/* map history types to files */
static const int storage_map[HISTORY_LAST] = {
    STORAGE_COMMAND,
    STORAGE_SEARCH,
    STORAGE_HISTORY
};
extern struct Vimb vb;

/**
 * Write a new history entry to the end of history file.
 */
void history_add(Client *c, HistoryType type, const char *value, const char *additional)
{
    FileStorage *s;

    /* Don't write a history entry if the history max size is set to 0. */
    if (!vb.config.history_max) {
        return;
    }

    s = HIST_STORAGE(type);
    if (additional) {
        file_storage_append(s, "%s\t%s\n", value, additional);
    } else {
        file_storage_append(s, "%s\n", value);
    }
}

/**
 * Makes all history items unique and force them to fit the maximum history
 * size and writes all entries of the different history types to file.
 */
void history_cleanup(void)
{
    FileStorage *s;
    GList *list;

    /* don't cleanup the history file if history max size is 0 */
    if (!vb.config.history_max) {
        return;
    }

    for (HistoryType i = HISTORY_FIRST; i < HISTORY_LAST; i++) {
        s = HIST_STORAGE(i);
        if (!file_storage_is_readonly(s)) {
            list = load(s);
            write_to_file(list, file_storage_get_path(s));
            g_list_free_full(list, (GDestroyNotify)free_history);
        }
    }
}

gboolean history_fill_completion(GtkListStore *store, HistoryType type, const char *input)
{
    char **parts;
    unsigned int len;
    gboolean found = FALSE;
    GList *src = NULL;
    GtkTreeIter iter;
    History *item;

    src = load(HIST_STORAGE(type));
    src = g_list_reverse(src);
    if (!input || !*input) {
        /* without any tags return all items */
        for (GList *l = src; l; l = l->next) {
            item = l->data;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(
                store, &iter,
                COMPLETION_STORE_FIRST, item->first,
#ifdef FEATURE_TITLE_IN_COMPLETION
                COMPLETION_STORE_SECOND, item->second,
#endif
                -1
            );
            found = TRUE;
        }
    } else if (HISTORY_URL == type) {
        parts = g_strsplit(input, " ", 0);
        len   = g_strv_length(parts);

        for (GList *l = src; l; l = l->next) {
            item = l->data;
            if (history_item_contains_all_tags(item, parts, len)) {
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(
                    store, &iter,
                    COMPLETION_STORE_FIRST, item->first,
#ifdef FEATURE_TITLE_IN_COMPLETION
                    COMPLETION_STORE_SECOND, item->second,
#endif
                    -1
                );
                found = TRUE;
            }
        }
        g_strfreev(parts);
    } else {
        for (GList *l = src; l; l = l->next) {
            item = l->data;
            if (g_str_has_prefix(item->first, input)) {
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(
                    store, &iter,
                    COMPLETION_STORE_FIRST, item->first,
#ifdef FEATURE_TITLE_IN_COMPLETION
                    COMPLETION_STORE_SECOND, item->second,
#endif
                    -1
                );
                found = TRUE;
            }
        }
    }
    g_list_free_full(src, (GDestroyNotify)free_history);

    return found;
}

/**
 * Retrieves the list of matching history items.
 * The list must be freed.
 */
GList *history_get_list(VbInputType type, const char *query)
{
    GList *result = NULL, *src = NULL;

    switch (type) {
        case INPUT_COMMAND:
            src = load(HIST_STORAGE(HISTORY_COMMAND));
            break;

        case INPUT_SEARCH_FORWARD:
        case INPUT_SEARCH_BACKWARD:
            src = load(HIST_STORAGE(HISTORY_SEARCH));
            break;

        default:
            return NULL;
    }

    /* generate new history list with the matching items */
    for (GList *l = src; l; l = l->next) {
        History *item = l->data;
        if (g_str_has_prefix(item->first, query)) {
            result = g_list_prepend(result, g_strdup(item->first));
        }
    }
    g_list_free_full(src, (GDestroyNotify)free_history);

    /* Prepend the original query as own item like done in vim to have the
     * original input string in input box if we step before the first real
     * item. */
    result = g_list_prepend(result, g_strdup(query));

    return result;
}

/**
 * Checks if the given array of tags are all found in history item.
 */
static gboolean history_item_contains_all_tags(History *item, char **query, guint qlen)
{
    unsigned int i;
    if (!qlen) {
        return TRUE;
    }

    /* iterate over all query parts */
    for (i = 0; i < qlen; i++) {
        if (!(util_strcasestr(item->first, query[i])
            || (item->second && util_strcasestr(item->second, query[i])))
        ) {
            return FALSE;
        }
    }

    return TRUE;
}

static void free_history(History *item)
{
    g_free(item->first);
    g_free(item->second);
    g_slice_free(History, item);
}

static History *line_to_history(const char *uri, const char *title)
{
    History *item = g_slice_new0(History);

    item->first  = g_strdup(uri);
    item->second = g_strdup(title);

    return item;
}

/**
 * Loads history items form file but eliminate duplicates in FIFO order.
 *
 * Returned list must be freed with (GDestroyNotify)free_history.
 */
static GList *load(FileStorage *s)
{
    return util_strv_to_unique_list(
        file_storage_get_lines(s),
        (Util_Content_Func)line_to_history,
        vb.config.history_max
    );
}

/**
 * Loads the entries from file, make them unique and write them back to file.
 */
static void write_to_file(GList *list, const char *file)
{
    FILE *f;
    if ((f = fopen(file, "w"))) {
        flock(fileno(f), LOCK_EX);

        /* overwrite the history file with new unique history items */
        for (GList *link = list; link; link = link->next) {
            History *item = link->data;
            if (item->second) {
                fprintf(f, "%s\t%s\n", item->first, item->second);
            } else {
                fprintf(f, "%s\n", item->first);
            }
        }

        flock(fileno(f), LOCK_UN);
        fclose(f);
    }
}
