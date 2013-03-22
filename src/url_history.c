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
static void url_history_free(UrlHist* item);


void url_history_init(void)
{
    /* read the history items from file */
    char buf[512] = {0};
    guint max = vb.config.url_history_max;
    FILE* file = fopen(vb.files[FILES_HISTORY], "r");
    if (!file) {
        return;
    }

    file_lock_set(fileno(file), F_WRLCK);

    for (guint i = 0; i < max && fgets(buf, sizeof(buf), file); i++) {
        char** argv = NULL;
        gint   argc = 0;
        if (g_shell_parse_argv(buf, &argc, &argv, NULL)) {
            url_history_add(argv[0], argc > 1 ? argv[1] : NULL);
        }
        g_strfreev(argv);
    }

    file_lock_set(fileno(file), F_UNLCK);
    fclose(file);

    /* reverse the history because we read it from lates to old from file */
    vb.behave.url_history = g_list_reverse(vb.behave.url_history);
}

void url_history_cleanup(void)
{
    FILE* file = fopen(vb.files[FILES_HISTORY], "w");
    if (file) {
        file_lock_set(fileno(file), F_WRLCK);

        /* write the history to the file */
        GList* link;
        for (link = vb.behave.url_history; link != NULL; link = link->next) {
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

    if (vb.behave.url_history) {
        g_list_free_full(vb.behave.url_history, (GDestroyNotify)url_history_free);
    }
}

void url_history_add(const char* url, const char* title)
{
    /* uf the url is already in history, remove this entry */
    /* TODO use g_list_find_custom for this task */
    for (GList* link = vb.behave.url_history; link; link = link->next) {
        UrlHist* hi = (UrlHist*)link->data;
        if (!g_strcmp0(url, hi->uri)) {
            url_history_free(hi);
            vb.behave.url_history = g_list_delete_link(vb.behave.url_history, link);
            break;
        }
    }

    while (vb.config.url_history_max < g_list_length(vb.behave.url_history)) {
        /* if list is too long - remove items from end */
        GList* last = g_list_last(vb.behave.url_history);
        url_history_free((UrlHist*)last->data);
        vb.behave.url_history = g_list_delete_link(vb.behave.url_history, last);
    }

    UrlHist* item = g_new0(UrlHist, 1);
    item->uri   = g_strdup(url);
    item->title = title ? g_strdup(title) : NULL;

    vb.behave.url_history = g_list_prepend(vb.behave.url_history, item);
}

/**
 * Appends the url history entries to given list.
 */
void url_history_get_all(GList** list)
{
    for (GList* link = vb.behave.url_history; link; link = link->next) {
        UrlHist* hi = (UrlHist*)link->data;
        /* put only the url in the list - do not allocate new memory */
        *list = g_list_prepend(*list, hi->uri);
    }

    *list = g_list_reverse(*list);
}

static void url_history_free(UrlHist* item)
{
    g_free(item->uri);
    if (item->title) {
        g_free(item->title);
    }
    g_free(item);
}
