/**
 * vimp - a webkit based vim like browser.
 *
 * Copyright (C) 2012 Daniel Carl
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

static GSList* searchengine_find(const gchar* handle);
static gboolean searchengine_is_valid_uri(const gchar* uri);


void searchengine_cleanup(void)
{
    if (vp.behave.searchengines) {
        g_slist_free(vp.behave.searchengines);
    }
}

gboolean searchengine_add(const gchar* handle, const gchar* uri)
{
    /* validate if the uri contains only one %s sequence */
    if (!searchengine_is_valid_uri(uri)) {
        return FALSE;
    }
    Searchengine* s = g_new0(Searchengine, 1);

    s->handle = g_strdup(handle);
    s->uri    = g_strdup(uri);

    vp.behave.searchengines = g_slist_prepend(vp.behave.searchengines, s);

    return TRUE;
}

gboolean searchengine_remove(const gchar* handle)
{
    GSList* list = searchengine_find(handle);

    if (list) {
        Searchengine* s = (Searchengine*)list->data;
        g_free(s->handle);
        g_free(s->uri);

        vp.behave.searchengines = g_slist_delete_link(vp.behave.searchengines, list);

        return TRUE;
    }

    return FALSE;
}

gchar* searchengine_get_uri(const gchar* handle)
{
    GSList* list = searchengine_find(handle);

    if (list) {
        return ((Searchengine*)list->data)->uri;
    }

    return NULL;
}

static GSList* searchengine_find(const gchar* handle)
{
    GSList* s;
    for (s = vp.behave.searchengines; s != NULL; s = s->next) {
        if (!strcmp(((Searchengine*)s->data)->handle, handle)) {
            return s;
        }
    }

    return NULL;
}

static gboolean searchengine_is_valid_uri(const gchar* uri)
{
    gint count = 0;

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
