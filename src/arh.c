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
#include "ascii.h"
#include "arh.h"
#include "util.h"

typedef struct {
    char *pattern; /* pattern the uri is matched against */
    char *name;    /* header name */
    char *value;   /* header value */
} MatchARH;

static void marh_free(MatchARH *);
static char *read_pattern(char **parse);
static char *read_name(char **parse);
static char *read_value(char **parse);


/**
 * parse the data string to a list of MatchARH
 *
 * pattern name=value[,...]
 */
GSList *arh_parse(const char *data, const char **error)
{
    GSList *parsed = NULL;
    char *data_dup = g_strdup(data);
    char *parse = data_dup;

    while (*parse) {
        char *pattern = read_pattern(&parse);
        char *name    = read_name(&parse);
        char *value   = read_value(&parse);

        if (pattern && name) {
            MatchARH *marh = g_slice_new0(MatchARH);

            marh->pattern = g_strdup(pattern);
            marh->name    = g_strdup(name);
            marh->value   = g_strdup(value);

            parsed = g_slist_append(parsed, marh);

            PRINT_DEBUG("append pattern='%s' name='%s' value='%s'",
                   marh->pattern, marh->name, marh->value);

        } else {
            /* one error occurs: free already parsed list */
            arh_free(parsed);

            /* set error if asked */
            if ( error != NULL ) {
                *error = "syntax error";
            }

            g_free(data_dup);
            return NULL;
        }
    }

    g_free(data_dup);
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
            if (marh->value) {
                /* the pattern match, so replace headers
                 * as side-effect, for one header the last matched will be keeped */
                soup_message_headers_replace(msg->response_headers,
                        marh->name, marh->value);

                PRINT_DEBUG(" header added: %s: %s", marh->name, marh->value);
            } else {
                /* remove a previously setted auto-reponse-header */
                soup_message_headers_remove(msg->response_headers, marh->name);

                PRINT_DEBUG(" header removed: %s", marh->name);
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
        g_free(marh->name);
        g_free(marh->value);

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

/**
 * read until '='
 */
static char *read_name(char **line)
{
    char *name;

    if (!*line || !**line) {
        return NULL;
    }

    /* remember where the name starts */
    name = *line;

    /* move pointer to the end of the name (or end-of-line) */
    while (**line && **line != '=') {
        (*line)++;
    }

    if (**line) {
        /* end the name */
        *(*line)++ = '\0';
        return name;

    } else {
        /* end-of-line was encounter */
        return NULL;
    }
}

/**
 * read until ',' or end-of-line
 */
static char *read_value(char **line)
{
    char *value;

    if (!*line || !**line) {
        return NULL;
    }

    /* remember where the value starts */
    value = *line;

    /* move pointer to the end of the value or of the line */
    while (**line && **line != ',') {
        (*line)++;
    }

    /* end the value */
    if (**line) {
        *(*line)++ = '\0';
    }

    return value;
}
