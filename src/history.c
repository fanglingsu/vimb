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
#include "history.h"
#include "util.h"
#include "completion.h"

extern VbCore vb;

/* map history types to files */
static const VbFile file_map[HISTORY_LAST] = {
    FILES_COMMAND,
    FILES_SEARCH,
    FILES_HISTORY
};

typedef struct {
    char *first;
    char *second;
} History;

static struct {
    char  *prefix;
    char  *query;
    GList *active;
} history;

static GList *get_list(const char *input);
static const char *get_file_by_type(HistoryType type);
static GList *load(const char *file);
static void write_to_file(GList *list, const char *file);
static History *line_to_history(const char *line);
static int history_comp(History *a, History *b);
static void free_history(History *item);


/**
 * Makes all history items unique and force them to fit the maximum history
 * size and writes all entries of the different history types to file.
 */
void history_cleanup(void)
{
    const char *file;
    for (HistoryType i = HISTORY_FIRST; i < HISTORY_LAST; i++) {
        file = get_file_by_type(i);
        write_to_file(load(file), file);
    }
}

/**
 * Write a new history entry to the end of history file.
 */
void history_add(HistoryType type, const char *value, const char *additional)
{
    FILE *f;
    const char *file = get_file_by_type(type);

    if ((f = fopen(file, "a+"))) {
        file_lock_set(fileno(f), F_WRLCK);
        if (additional) {
            fprintf(f, "%s\t%s\n", value, additional);
        } else {
            fprintf(f, "%s\n", value);
        }

        file_lock_set(fileno(f), F_UNLCK);
        fclose(f);
    }
}

/**
 * Retrieves the item from history to be shown in input box.
 * The result must be freed by the caller.
 */
char *history_get(const char *input, gboolean prev)
{
    GList *new = NULL;

    if (!history.active) {
        history.active = get_list(input);
        /* start with latest added items */
        history.active = g_list_first(history.active);
        history.active = g_list_prepend(history.active, g_strdup(""));
    }

    if (prev) {
        if ((new = g_list_next(history.active))) {
            history.active = new;
        }
    } else if ((new = g_list_previous(history.active))) {
        history.active = new;
    }

    return g_strconcat(history.prefix, (char*)history.active->data, NULL);
}

void history_rewind(void)
{
    if (history.active) {
        /* free temporary used history list */
        g_list_free_full(history.active, (GDestroyNotify)g_free);

        OVERWRITE_STRING(history.prefix, NULL);
        history.active = NULL;
    }
}

gboolean history_fill_completion(GtkListStore *store, HistoryType type, const char *input)
{
    char **parts;
    unsigned int len;
    gboolean found = false;
    GList *src = NULL;
    GtkTreeIter iter;
    History *item;

    src = load(get_file_by_type(type));
    if (!input || *input == '\0') {
        /* without any tags return all items */
        for (GList *l = src; l; l = l->next) {
            item = l->data;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, COMPLETION_STORE_FIRST, item->first, -1);
            found = true;
        }
    } else if (HISTORY_URL == type) {
        parts = g_strsplit(input, " ", 0);
        len   = g_strv_length(parts);

        for (GList *l = src; l; l = l->next) {
            item = l->data;
            if (util_string_contains_all_tags(item->first, parts, len)) {
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter, COMPLETION_STORE_FIRST, item->first, -1);
                found = true;
            }
        }
        g_strfreev(parts);
    } else {
        for (GList *l = src; l; l = l->next) {
            item = l->data;
            if (g_str_has_prefix(item->first, input)) {
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter, COMPLETION_STORE_FIRST, item->first, -1);
                found = true;
            }
        }
    }
    g_list_free_full(src, (GDestroyNotify)g_free);

    return found;
}

/**
 * Retrieves the list of matching history items.
 * The list must be freed.
 */
static GList *get_list(const char *input)
{
    VbInputType input_type;
    HistoryType type;
    GList *result = NULL;
    const char *prefix, *suffix;

    input_type = vb_get_input_parts(input, &prefix, &suffix);

    /* get the right history type and command prefix */
    if (input_type == VB_INPUT_OPEN
        || input_type == VB_INPUT_TABOPEN
    ) {
        type = HISTORY_URL;
        OVERWRITE_STRING(history.query, suffix);
        OVERWRITE_STRING(history.prefix, prefix);
    } else if (input_type == VB_INPUT_COMMAND) {
        type = HISTORY_COMMAND;
        OVERWRITE_STRING(history.query, suffix);
        OVERWRITE_STRING(history.prefix, prefix);
    } else if (input_type == VB_INPUT_SEARCH_FORWARD
        || input_type == VB_INPUT_SEARCH_BACKWARD
    ) {
        type = HISTORY_SEARCH;
        OVERWRITE_STRING(history.query, suffix);
        OVERWRITE_STRING(history.prefix, prefix);
    } else {
        return NULL;
    }

    GList *src = load(get_file_by_type(type));

    /* generate new history list with the matching items */
    for (GList *l = src; l; l = l->next) {
        History *item = l->data;
        if (g_str_has_prefix(item->first, history.query)) {
            result = g_list_prepend(result, g_strdup(item->first));
        }
    }
    g_list_free_full(src, (GDestroyNotify)free_history);

    return result;
}

static const char *get_file_by_type(HistoryType type)
{
    return vb.files[file_map[type]];
}

/**
 * Loads history items form file but eleminate duplicates in FIFO order.
 */
static GList *load(const char *file)
{
    /* read the history items from file */
    GList *list = NULL;

    list = util_file_to_unique_list(
        file, (Util_Content_Func)line_to_history, (GCompareFunc)history_comp,
        (GDestroyNotify)free_history
    );

    /* if list is too long - remove items from end (oldest entries) */
    if (vb.config.history_max < g_list_length(list)) {
        while (vb.config.history_max < g_list_length(list)) {
            GList *last = g_list_first(list);
            g_free(last->data);
            list = g_list_delete_link(list, last);
        }
    }

    return list;
}

/**
 * Loads the entries from file, make them unique and write them back to file.
 */
static void write_to_file(GList *list, const char *file)
{
    FILE *f;
    if ((f = fopen(file, "w"))) {
        file_lock_set(fileno(f), F_WRLCK);

        /* overwrite the history file with new unique history items */
        for (GList *link = list; link; link = link->next) {
            History *item = link->data;
            if (item->second) {
                fprintf(f, "%s\t%s\n", item->first, item->second);
            } else {
                fprintf(f, "%s\n", item->first);
            }
        }

        file_lock_set(fileno(f), F_UNLCK);
        fclose(f);
    }

    g_list_free_full(list, (GDestroyNotify)free_history);
}

static History *line_to_history(const char *line)
{
    char **parts;
    int len;

    while (g_ascii_isspace(*line)) {
        line++;
    }
    if (*line == '\0') {
        return NULL;
    }

    History *item = g_new0(History, 1);

    parts = g_strsplit(line, "\t", 2);
    len   = g_strv_length(parts);
    if (len == 2) {
        item->first  = g_strdup(parts[0]);
        item->second = g_strdup(parts[1]);
    } else {
        item->first  = g_strdup(parts[0]);
    }
    g_strfreev(parts);

    return item;
}

static int history_comp(History *a, History *b)
{
    /* compare only the first part */
    return g_strcmp0(a->first, b->first);
}

static void free_history(History *item)
{
    g_free(item->first);
    g_free(item->second);
    g_free(item);
}
