/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2014 Daniel Carl
 * Copyright (C) 2014 Sébastien Marie
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

#include "config.h"
#ifdef FEATURE_ARH
#include "ascii.h"
#include "arh.h"
#include "util.h"

typedef struct {
    char       *pattern; /* pattern the uri is matched against */
    GHashTable *headers; /* header list */
} MatchARH;

static void marh_free(MatchARH *);
static char *read_pattern(char **);


/**
 * parse the data string to a list of MatchARH
 *
 * pattern name=value[,...]
 */
GSList *arh_parse(const char *data, const char **error)
{
    GSList *parsed = NULL;
    GSList *data_list = NULL;

    /* parse data as comma separated list
     * of "pattern1 HEADERS-GROUP1","pattern2 HEADERS-GROUP2",... */
    data_list = soup_header_parse_list(data);

    /* iter the list for parsing each elements */
    for (GSList *l = data_list; l; l = g_slist_next(l)) {
        char *one_data = (char *)l->data;
        char *pattern;
        GHashTable *headers;

        /* remove QUOTE around one_data if any */
        if (one_data && *one_data == '"') {
            int last = strlen(one_data) - 1;
            if (last >= 0 && one_data[last] == '"') {
                one_data[last] = '\0';
                one_data++;
            }
        }

        /* read pattern and parse headers */
        pattern = read_pattern(&one_data);
        headers = soup_header_parse_param_list(one_data);

        /* check result (need a pattern and at least one header) */
        if (pattern && g_hash_table_size(headers)) {
            MatchARH *marh = g_slice_new0(MatchARH);
            marh->pattern = g_strdup(pattern);
            marh->headers = headers;

            parsed = g_slist_append(parsed, marh);

#ifdef DEBUG
            PRINT_DEBUG("append pattern='%s' headers[%d]='0x%lx'",
                   marh->pattern, g_hash_table_size(marh->headers), (long)marh->headers);
            GHashTableIter iterHeaders;
            const char *name, *value;
            g_hash_table_iter_init(&iterHeaders, headers);
            while (g_hash_table_iter_next(&iterHeaders, (gpointer)&name, (gpointer)&value)) {
                PRINT_DEBUG(" %s=%s", name, value);
            }
#endif
        } else {
            /* an error occurs: cleanup */
            soup_header_free_param_list(headers);
            soup_header_free_list(data_list);
            arh_free(parsed);

            /* set error if asked */
            if (error != NULL) {
                *error = "syntax error";
            }

            return NULL;
        }
    }

    soup_header_free_list(data_list);

    return parsed;
}

/**
 * free the list of MatchARH
 */
void arh_free(GSList *list)
{
    g_slist_free_full(list, (GDestroyNotify)marh_free);
}

/**
 * append to reponse-header of SoupMessage,
 * the header that match the specified uri
 */
void arh_run(GSList *list, const char *uri, SoupMessage *msg)
{
    PRINT_DEBUG("uri: %s", uri);

    /* walk thought the list */
    for (GSList *l=list; l; l = g_slist_next(l)) {
        MatchARH *marh = (MatchARH *)l->data;

        if (util_wildmatch(marh->pattern, uri)) {
            GHashTableIter iter;
            const char *name  = NULL;
            const char *value = NULL;

            g_hash_table_iter_init(&iter, marh->headers);
            while (g_hash_table_iter_next(&iter, (gpointer)&name, (gpointer)&value)) {
                if (value) {
                    /* the pattern match, so replace headers
                     * as side-effect, for one header the last matched will be keeped */
                    soup_message_headers_replace(msg->response_headers, name, value);

                    PRINT_DEBUG(" header added: %s: %s", name, value);
                } else {
                    /* remove a previously setted auto-reponse-header */
                    soup_message_headers_remove(msg->response_headers, name);

                    PRINT_DEBUG(" header removed: %s", name);
                }
            }
        }
    }
}

/**
 * free a MatchARH
 */
static void marh_free(MatchARH *marh)
{
    if (marh) {
        g_free(marh->pattern);
        soup_header_free_param_list(marh->headers);

        g_slice_free(MatchARH, marh);
    }
}

/**
 * read until ' ' (or any other SPACE)
 */
static char *read_pattern(char **line)
{
    char *pattern;

    if (!*line || !**line) {
        return NULL;
    }

    /* remember where the pattern starts */
    pattern = *line;

    /* move pointer to the end of the pattern (or end-of-line) */
    while (**line && !VB_IS_SPACE(**line)) {
        (*line)++;
    }

    if (**line) {
        /* end the pattern */
        *(*line)++ = '\0';

        /* skip trailing whitespace */
        while (VB_IS_SPACE(**line)) {
            (*line)++;
        }

        return pattern;

    } else {
        /* end-of-line was encounter */
        return NULL;
    }
}
#endif
