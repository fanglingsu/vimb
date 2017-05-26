/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2017 Daniel Carl
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

#include <glib.h>
#include <string.h>

#include "ascii.h"
#include "main.h"
#include "shortcut.h"
#include "util.h"

extern struct Vimb vb;

static int get_max_placeholder(const char *str);
static const char *shortcut_lookup(Client *c, const char *string, const char **query);


void shortcut_init(Client *c)
{
    c->shortcut.table    = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    c->shortcut.fallback = NULL;
}

void shortcut_cleanup(Client *c)
{
    if (c->shortcut.table) {
        g_hash_table_destroy(c->shortcut.table);
    }
}

gboolean shortcut_add(Client *c, const char *key, const char *uri)
{
    g_hash_table_insert(c->shortcut.table, g_strdup(key), g_strdup(uri));

    return TRUE;
}

gboolean shortcut_remove(Client *c, const char *key)
{
    return g_hash_table_remove(c->shortcut.table, key);
}

gboolean shortcut_set_default(Client *c, const char *key)
{
    /* do not check if the shortcut exists to be able to set the default
     * before defining the shortcut */
    OVERWRITE_STRING(c->shortcut.fallback, key);

    return TRUE;
}

/**
 * Retrieves the uri for given query string. Not that the memory of the
 * returned uri must be freed.
 */
char *shortcut_get_uri(Client *c, const char *string)
{
    const char *tmpl, *query = NULL;
    char *uri, *quoted_param;
    int max_num, current_num;
    GString *token;

    tmpl = shortcut_lookup(c, string, &query);
    if (!tmpl) {
        return NULL;
    }

    max_num = get_max_placeholder(tmpl);
    /* if there are only $0 placeholders we don't need to split the parameters */
    if (max_num == 0) {
        quoted_param = soup_uri_encode(query, "&+");
        uri          = util_str_replace("$0", quoted_param, tmpl);
        g_free(quoted_param);

        return uri;
    }

    uri = g_strdup(tmpl);

    /* skip if no placeholders found */
    if (max_num < 0) {
        return uri;
    }

    current_num = 0;
    token       = g_string_new(NULL);
    while (*query) {
        /* parse the query tokens */
        if (*query == '"' || *query == '\'') {
            /* save the last used quote char to find it's matching counterpart */
            char last_quote = *query;

            /* skip the quote */
            query++;
            /* collect the char until the closing quote or end of string */
            while (*query && *query != last_quote) {
                g_string_append_c(token, *query);
                query++;
            }
            /* if we end up at the closing quote - skip this quote too */
            if (*query == last_quote) {
                query++;
            }
        } else if (VB_IS_SPACE(*query)) {
            /* skip whitespace */
            query++;

            continue;
        } else if (current_num >= max_num) {
            /* if we have parsed as many params like placeholders - put the
             * rest of the query as last parameter */
            while (*query) {
                g_string_append_c(token, *query);
                query++;
            }
        } else {
            /* collect the following character up to the next whitespace */
            while (*query && !VB_IS_SPACE(*query)) {
                g_string_append_c(token, *query);
                query++;
            }
        }

        /* replace the placeholders with parsed token */
        if (token->len) {
            char *new;

            quoted_param = soup_uri_encode(token->str, "&+");
            new = util_str_replace((char[]){'$', current_num + '0', '\0'}, quoted_param, uri);
            g_free(quoted_param);
            g_free(uri);
            uri = new;

            /* truncate the last token to fill for next loop */
            g_string_truncate(token, 0);
        }
        current_num++;
    }
    g_string_free(token, TRUE);

    return uri;
}

gboolean shortcut_fill_completion(Client *c, GtkListStore *store, const char *input)
{
    GList *src = g_hash_table_get_keys(c->shortcut.table);
    gboolean found = util_fill_completion(store, input, src);
    g_list_free(src);

    return found;
}

/**
 * Retrieves th highest placeholder number used in given string.
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
 * Retrieves the shortcut uri template for given string. And fills given query
 * pointer with the query part of the given string (everything except of the
 * shortcut identifier).
 */
static const char *shortcut_lookup(Client *c, const char *string, const char **query)
{
    char *p, *uri = NULL;

    if ((p = strchr(string, ' '))) {
        char *key  = g_strndup(string, p - string);
        /* is the first word might be a shortcut */
        if ((uri = g_hash_table_lookup(c->shortcut.table, key))) {
            *query = p + 1;
        }
        g_free(key);
    } else {
        uri = g_hash_table_lookup(c->shortcut.table, string);
    }

    if (!uri && c->shortcut.fallback
            && (uri = g_hash_table_lookup(c->shortcut.table, c->shortcut.fallback))) {
        *query = string;
    }

    return uri;
}
