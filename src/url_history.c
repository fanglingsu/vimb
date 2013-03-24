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
#include "url_history.h"

extern VbCore vb;
static void url_history_add_unique(GList** list, const char* url, const char* title);
static GList* url_history_load(void);
static void url_history_write_to_file(GList* list);
static void url_history_clear(GList** list);
static void url_history_free(UrlHist* item);


void url_history_cleanup(void)
{
    url_history_write_to_file(url_history_load());
}

/**
 * Write a new history entry to the end of history file.
 */
void url_history_add(const char* uri, const char* title)
{
    FILE* file = fopen(vb.files[FILES_HISTORY], "a+");
    if (file) {
        file_lock_set(fileno(file), F_WRLCK);

        char* name = g_shell_quote(title ? title : "");
        char* new  = g_strdup_printf("%s %s\n", uri, name);

        fwrite(new, strlen(new), 1, file);

        g_free(name);
        g_free(new);

        file_lock_set(fileno(file), F_UNLCK);
        fclose(file);
    }
}

/**
 * Appends all url history entries form history file to given list.
 */
void url_history_get_all(GList** list)
{
    GList* src = url_history_load();

    for (GList* link = src; link; link = link->next) {
        UrlHist* hi = (UrlHist*)link->data;
        /* put only the url in the list - do not allocate new memory */
        *list = g_list_prepend(*list, hi->uri);
    }

    *list = g_list_reverse(*list);
}

/**
 * Loads history items form file but elemiate duplicates.
 */
static GList* url_history_load(void)
{
    /* read the history items from file */
    GList* list   = NULL;
    char buf[512] = {0};
    FILE* file;
    
    if (!(file = fopen(vb.files[FILES_HISTORY], "r"))) {
        return list;
    }

    file_lock_set(fileno(file), F_WRLCK);
    while (fgets(buf, sizeof(buf), file)) {
        char** argv = NULL;
        int    argc = 0;
        if (g_shell_parse_argv(buf, &argc, &argv, NULL)) {
            url_history_add_unique(&list, argv[0], argc > 1 ? argv[1] : NULL);
        }
        g_strfreev(argv);
    }
    file_lock_set(fileno(file), F_UNLCK);
    fclose(file);

    /* if list is too long - remove items from end (oldest entries) */
    while (vb.config.url_history_max < g_list_length(list)) {
        GList* last = g_list_last(list);
        url_history_free((UrlHist*)last->data);
        list = g_list_delete_link(list, last);
    }

    return list;
}

static void url_history_add_unique(GList** list, const char* url, const char* title)
{
    /* if the url is already in history, remove this entry */
    for (GList* link = *list; link; link = link->next) {
        UrlHist* hi = (UrlHist*)link->data;
        if (!g_strcmp0(url, hi->uri)) {
            url_history_free(hi);
            *list     = g_list_delete_link(*list, link);
            break;
        }
    }

    UrlHist* item = g_new0(UrlHist, 1);
    item->uri     = g_strdup(url);
    item->title   = title ? g_strdup(title) : NULL;

    *list = g_list_prepend(*list, item);
}

/**
 * Loads the entries from file, make them unique and write them back to file.
 */
static void url_history_write_to_file(GList* list)
{
    FILE* file = fopen(vb.files[FILES_HISTORY], "w");
    if (file) {
        file_lock_set(fileno(file), F_WRLCK);

        /* overwrite the history file with new unique history items */
        for (GList* link = g_list_reverse(list); link; link = link->next) {
            UrlHist* item = (UrlHist*)link->data;

            char* title = g_shell_quote(item->title ? item->title : "");
            char* new   = g_strdup_printf("%s %s\n", item->uri, title);

            fwrite(new, strlen(new), 1, file);

            g_free(title);
            g_free(new);
        }

        file_lock_set(fileno(file), F_UNLCK);
        fclose(file);
    }
    url_history_clear(&list);
}

static void url_history_clear(GList** list)
{
    if (*list) {
        g_list_free_full(*list, (GDestroyNotify)url_history_free);
        *list = NULL;
    }
}

static void url_history_free(UrlHist* item)
{
    g_free(item->uri);
    if (item->title) {
        g_free(item->title);
    }
    g_free(item);
}
