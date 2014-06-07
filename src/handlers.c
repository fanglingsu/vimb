/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2014 Daniel Carl
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
#include "handlers.h"
#include "util.h"

static GHashTable *handlers = NULL;

static char *handler_lookup(const char *uri);


void handlers_init(void)
{
    handlers = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
}

void handlers_cleanup(void)
{
    if (handlers) {
        g_hash_table_destroy(handlers);
    }
}

gboolean handler_add(const char *key, const char *cmd)
{
    g_hash_table_insert(handlers, g_strdup(key), g_strdup(cmd));

    return true;
}

gboolean handler_remove(const char *key)
{
    return g_hash_table_remove(handlers, key);
}

gboolean handle_uri(const char *uri)
{
    char *handler, *cmd;
    GError *error = NULL;
    gboolean result;

    if (!(handler = handler_lookup(uri))) {
        return false;
    }

    cmd = g_strdup_printf(handler, uri);
    if (!g_spawn_command_line_async(cmd, &error)) {
        g_warning("Can't run '%s': %s", cmd, error->message);
        g_clear_error(&error);
        result = false;
    } else {
        result = true;
    }

    g_free(cmd);
    return result;
}

gboolean handler_fill_completion(GtkListStore *store, const char *input)
{
    GList *src = g_hash_table_get_keys(handlers);
    gboolean found = util_fill_completion(store, input, src);
    g_list_free(src);

    return found;
}

static char *handler_lookup(const char *uri)
{
    char *p, *schema, *handler = NULL;

    if ((p = strchr(uri, ':'))) {
        schema  = g_strndup(uri, p - uri);
        handler = g_hash_table_lookup(handlers, schema);
        g_free(schema);
    }

    return handler;
}
