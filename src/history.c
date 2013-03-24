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

extern VbCore vb;

/* map history types to files */
static const VbFile file_map[HISTORY_LAST] = {
    FILES_COMMAND,
    FILES_HISTORY
};

static const char* history_get_file_by_type(HistoryType type);
static GList* history_load(const char* file);
static void history_write_to_file(GList* list, const char* file);


/**
 * Makes all history items unique and force them to fit the maximum history
 * size and writes all entries of the different history types to file.
 */
void history_cleanup(void)
{
    for (HistoryType i = HISTORY_FIRST; i < HISTORY_LAST; i++) {
        const char* file = history_get_file_by_type(i);
        history_write_to_file(history_load(file), file);
    }
}

/**
 * Write a new history entry to the end of history file.
 */
void history_add(HistoryType type, const char* value)
{
    const char* file = history_get_file_by_type(type);
    FILE* f;
    if ((f = fopen(file, "a+"))) {
        file_lock_set(fileno(f), F_WRLCK);

        fprintf(f, "%s\n", value);

        file_lock_set(fileno(f), F_UNLCK);
        fclose(f);
    }
}

/**
 * Retrieves all history entries for given history type.
 */
GList* history_get_all(HistoryType type)
{
    return history_load(history_get_file_by_type(type));
}

const char* history_get(HistoryType type, int step)
{
    const char* command;

    /* get the search prefix only on start of history search */
    if (!vb.state.history_active) {
        OVERWRITE_STRING(vb.state.history_prefix, GET_TEXT());

        GList* src = history_load(history_get_file_by_type(type));

        /* generate new history list with the matching items */
        for (GList* l = src; l; l = l->next) {
            char* value = (char*)l->data;
            if (g_str_has_prefix(value, vb.state.history_prefix)) {
                vb.state.history_active = g_list_prepend(vb.state.history_active, g_strdup(value));
            }
        }

        vb.state.history_active = g_list_reverse(vb.state.history_active);
    }

    const int len = g_list_length(vb.state.history_active);
    if (!len) {
        return NULL;
    }

    /* if reached end/beginnen start at the opposit site of list again */
    vb.state.history_pointer = (len + vb.state.history_pointer + step) % len;

    command = (char*)g_list_nth_data(vb.state.history_active, vb.state.history_pointer);

    return command;
}

void history_rewind(void)
{
    if (vb.state.history_active) {
        OVERWRITE_STRING(vb.state.history_prefix, NULL);
        vb.state.history_pointer = 0;
        /* free temporary used history list */
        g_list_free_full(vb.state.history_active, (GDestroyNotify)g_free);
        vb.state.history_active = NULL;
    }
}

void history_list_free(GList** list)
{
    if (*list) {
        g_list_free_full(*list, (GDestroyNotify)g_free);
        *list = NULL;
    }
}

static const char* history_get_file_by_type(HistoryType type)
{
    return vb.files[file_map[type]];
}

/**
 * Loads history items form file but elemiate duplicates.
 */
static GList* history_load(const char* file)
{
    /* read the history items from file */
    GList* list   = NULL;
    char buf[512] = {0};
    FILE* f;

    if (!(f = fopen(file, "r"))) {
        return list;
    }

    file_lock_set(fileno(f), F_WRLCK);
    while (fgets(buf, sizeof(buf), f)) {
        g_strstrip(buf);

        /* skip empty lines */
        if (!*buf) {
            continue;
        }
        /* if the value is already in history, remove this entry */
        for (GList* l = list; l; l = l->next) {
            if (*buf && !g_strcmp0(buf, (char*)l->data)) {
                g_free(l->data);
                list = g_list_delete_link(list, l);
                break;
            }
        }

        list = g_list_prepend(list, g_strdup(buf));
    }
    file_lock_set(fileno(f), F_UNLCK);
    fclose(f);

    /* if list is too long - remove items from end (oldest entries) */
    if (vb.config.history_max < g_list_length(list)) {
        /* reverse to not use the slow g_list_last */
        list = g_list_reverse(list);
        while (vb.config.history_max < g_list_length(list)) {
            GList* last = g_list_first(list);
            g_free(last->data);
            list = g_list_delete_link(list, last);
        }
        list = g_list_reverse(list);
    }

    return list;
}

/**
 * Loads the entries from file, make them unique and write them back to file.
 */
static void history_write_to_file(GList* list, const char* file)
{
    FILE* f;
    if ((f = fopen(file, "w"))) {
        file_lock_set(fileno(f), F_WRLCK);

        /* overwrite the history file with new unique history items */
        for (GList* link = g_list_reverse(list); link; link = link->next) {
            fprintf(f, "%s\n", (char*)link->data);
        }

        file_lock_set(fileno(f), F_UNLCK);
        fclose(f);
    }

    history_list_free(&list);
}
