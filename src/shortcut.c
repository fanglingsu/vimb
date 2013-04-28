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

static GHashTable *shortcuts = NULL;
static char *default_key = NULL;

static int get_max_placeholder(const char *str);
static const char *shortcut_lookup(const char *string, const char **query);


void shortcut_init(void)
{
    shortcuts = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
}

void shortcut_cleanup(void)
{
    if (shortcuts) {
        g_hash_table_destroy(shortcuts);
    }
}

gboolean shortcut_add(const char *key, const char *uri)
{
    g_hash_table_insert(shortcuts, g_strdup(key), g_strdup(uri));

    return true;
}

gboolean shortcut_remove(const char *key)
{
    return g_hash_table_remove(shortcuts, key);
}

gboolean shortcut_set_default(const char *key)
{
    /* do not check if the shotcut exists to be able to set the default
     * before defining the shotcut */
    OVERWRITE_STRING(default_key, key);

    return true;
}

/**
 * Retrieves the uri for given query string. Not that the memory of the
 * returned uri must be freed.
 */
char *shortcut_get_uri(const char *string)
{
    const char *tmpl, *query = NULL;

    tmpl = shortcut_lookup(string, &query);
    if (!tmpl) {
        return NULL;
    }

    char *qs, *uri, **parts, ph[3] = "$0";
    unsigned int len;

    /* replace $0 with all parameters */
    qs = soup_uri_encode(query, "&");
    uri = util_str_replace(ph, qs, tmpl);
    g_free(qs);

    int max = get_max_placeholder(tmpl);
    /* skip if no placeholders found */
    if (max < 0) {
        return uri;
    }

    /* split the parameters */
    parts = g_strsplit(query, " ", max + 1);
    len   = g_strv_length(parts);

    for (unsigned int n = 0; n < len; n++) {
        char *new;
        ph[1] = n + '1';
        qs  = soup_uri_encode(parts[n], "&");
        new = util_str_replace(ph, qs, uri);
        g_free(qs);
        g_free(uri);
        uri = new;
    }
    g_strfreev(parts);

    return uri;
}

/**
 * Retrieves th highest placesholder number used in given string.
 * If no placeholder is found -1 is returned.
 */
static int get_max_placeholder(const char *str)
{
    int n, res;

    for (n = 0, res = -1; *str; str++) {
        if (*str == '$') {
            n = *(++str) - '0';
            if (0 <= n && n <= 9 && n > res) {
                res = n;
            }
        }
    }

    return res;
}

/**
 * Retrieves the shortcut uri template for given string.
 * If the string contains the shortcut key the shortcut for this wee be
 * returned, else the default shortcur uri well be returned.
 * In given query pointer will be filled with the query part of the string,
 * thats the string without a possible shortcut key.
 */
static const char *shortcut_lookup(const char *string, const char **query)
{
    char *p, *uri = NULL;

    if ((p = strchr(string, ' '))) {
        *p = '\0';
        /* is the first word the key? */
        if ((uri = g_hash_table_lookup(shortcuts, string))) {
            *query = p + 1;
        } else {
            *p = ' ';
        }
    }

    if (!uri && (uri = g_hash_table_lookup(shortcuts, default_key))) {
        *query = string;
    }

    return uri;
}
