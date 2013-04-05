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
#include "searchengine.h"

extern VbCore vb;

typedef struct {
    char *handle;
    char *uri;
} Searchengine;

static GSList *find(const char *handle);
static gboolean is_valid_uri(const char *uri);
static void free_searchengine(Searchengine *se);


void searchengine_cleanup(void)
{
    if (vb.behave.searchengines) {
        g_slist_free_full(vb.behave.searchengines, (GDestroyNotify)free_searchengine);
    }
}

gboolean searchengine_add(const char *handle, const char *uri)
{
    /* validate if the uri contains only one %s sequence */
    if (!is_valid_uri(uri)) {
        return false;
    }
    Searchengine *s = g_new0(Searchengine, 1);

    s->handle = g_strdup(handle);
    s->uri    = g_strdup(uri);

    vb.behave.searchengines = g_slist_prepend(vb.behave.searchengines, s);

    return TRUE;
}

gboolean searchengine_remove(const char *handle)
{
    GSList *list = find(handle);

    if (list) {
        free_searchengine((Searchengine*)list->data);
        vb.behave.searchengines = g_slist_delete_link(vb.behave.searchengines, list);

        return TRUE;
    }

    return false;
}

gboolean searchengine_set_default(const char *handle)
{
    /* do not check if the search engin exists to be able to set the default
     * before defining the search engines */
    OVERWRITE_STRING(vb.behave.searchengine_default, handle);

    return TRUE;
}

/**
 * Retrieves the search uri for given query string.
 * Not that the memory of the returned uri must be freed.
 */
char *searchengine_get_uri(const char *string)
{
    GSList *l = NULL;
    char *p = NULL, *tmpl = NULL, *query = NULL, *uri = NULL;

    if ((p = strchr(string, ' '))) {
        *p = '\0';
        /* is the first word the handle? */
        if ((l = find(string))) {
            tmpl  = ((Searchengine*)l->data)->uri;
            query = soup_uri_encode(p + 1, "&");
        } else {
            *p = ' ';
        }
    }

    if (!tmpl && (l = find(vb.behave.searchengine_default))) {
        tmpl  = ((Searchengine*)l->data)->uri;
        query = soup_uri_encode(string, "&");
    }

    if (tmpl) {
        uri = g_strdup_printf(tmpl, query);
        g_free(query);

        return uri;
    }

    return NULL;
}

static GSList *find(const char *handle)
{
    GSList *s;
    for (s = vb.behave.searchengines; s != NULL; s = s->next) {
        if (!strcmp(((Searchengine*)s->data)->handle, handle)) {
            return s;
        }
    }

    return NULL;
}

static gboolean is_valid_uri(const char *uri)
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

static void free_searchengine(Searchengine *se)
{
    g_free(se->uri);
    g_free(se->handle);
    g_free(se);
}
