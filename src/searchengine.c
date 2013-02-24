/**
 * vimp - a webkit based vim like browser.
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
#include "searchengine.h"

static GSList* searchengine_find(const char* handle);
static gboolean searchengine_is_valid_uri(const char* uri);
static void searchengine_free(Searchengine* se);


void searchengine_cleanup(void)
{
    if (core.behave.searchengines) {
        g_slist_free_full(core.behave.searchengines, (GDestroyNotify)searchengine_free);
    }
}

gboolean searchengine_add(const char* handle, const char* uri)
{
    /* validate if the uri contains only one %s sequence */
    if (!searchengine_is_valid_uri(uri)) {
        return FALSE;
    }
    Searchengine* s = g_new0(Searchengine, 1);

    s->handle = g_strdup(handle);
    s->uri    = g_strdup(uri);

    core.behave.searchengines = g_slist_prepend(core.behave.searchengines, s);

    return TRUE;
}

gboolean searchengine_remove(const char* handle)
{
    GSList* list = searchengine_find(handle);

    if (list) {
        Searchengine* s = (Searchengine*)list->data;
        g_free(s->handle);
        g_free(s->uri);

        core.behave.searchengines = g_slist_delete_link(core.behave.searchengines, list);

        return TRUE;
    }

    return FALSE;
}

char* searchengine_get_uri(const char* handle)
{
    GSList* list = searchengine_find(handle);

    if (list) {
        return ((Searchengine*)list->data)->uri;
    }

    return NULL;
}

static GSList* searchengine_find(const char* handle)
{
    GSList* s;
    for (s = core.behave.searchengines; s != NULL; s = s->next) {
        if (!strcmp(((Searchengine*)s->data)->handle, handle)) {
            return s;
        }
    }

    return NULL;
}

static gboolean searchengine_is_valid_uri(const char* uri)
{
    int count = 0;

    for (; *uri; uri++) {
        if (*uri == '%') {
            uri++;
            if (*uri == 's') {
                count++;
            }
        }
    }

    return count == 1;
}

static void searchengine_free(Searchengine* se)
{
    g_free(se->uri);
    g_free(se->handle);
    g_free(se);
}
