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
#include "shortcut.h"
#include "util.h"

extern VbCore vb;

typedef struct {
    char *handle;
    char *uri;
} Shortcut;

static GSList *shortcuts;
static char   *default_handle = NULL;

static GSList *find(const char *handle);
static void free_shortcut(Shortcut *se);


void shortcut_cleanup(void)
{
    if (shortcuts) {
        g_slist_free_full(shortcuts, (GDestroyNotify)free_shortcut);
    }
}

gboolean shortcut_add(const char *handle, const char *uri)
{
    /* validate if the uri contains only one %s sequence */
    if (!util_valid_format_string(uri, 's', 1)) {
        return false;
    }
    Shortcut *s = g_new0(Shortcut, 1);

    s->handle = g_strdup(handle);
    s->uri    = g_strdup(uri);

    shortcuts = g_slist_prepend(shortcuts, s);

    return true;
}

gboolean shortcut_remove(const char *handle)
{
    GSList *list = find(handle);

    if (list) {
        free_shortcut((Shortcut*)list->data);
        shortcuts = g_slist_delete_link(shortcuts, list);

        return true;
    }

    return false;
}

gboolean shortcut_set_default(const char *handle)
{
    /* do not check if the shotcut exists to be able to set the default
     * before defining the shotcut */
    OVERWRITE_STRING(default_handle, handle);

    return true;
}

/**
 * Retrieves the uri for given query string. Not that the memory of the
 * returned uri must be freed.
 */
char *shortcut_get_uri(const char *string)
{
    GSList *l = NULL;
    char *p = NULL, *tmpl = NULL, *query = NULL, *uri = NULL;

    if ((p = strchr(string, ' '))) {
        *p = '\0';
        /* is the first word the handle? */
        if ((l = find(string))) {
            tmpl  = ((Shortcut*)l->data)->uri;
            query = soup_uri_encode(p + 1, "&");
        } else {
            *p = ' ';
        }
    }

    if (!tmpl && (l = find(default_handle))) {
        tmpl  = ((Shortcut*)l->data)->uri;
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
    for (s = shortcuts; s != NULL; s = s->next) {
        if (!strcmp(((Shortcut*)s->data)->handle, handle)) {
            return s;
        }
    }

    return NULL;
}

static void free_shortcut(Shortcut *se)
{
    g_free(se->uri);
    g_free(se->handle);
    g_free(se);
}
