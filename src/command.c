/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2016 Daniel Carl
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

/**
 * This file contains functions that are used by normal mode and command mode
 * together.
 */
#include "command.h"
#include "config.h"
#include "history.h"
#include "main.h"
#include <stdlib.h>

extern struct Vimb vb;

gboolean command_search(Client *c, const Arg *arg)
{
    const char *query;
    gboolean forward;
    WebKitFindController *fc;

    fc = webkit_web_view_get_find_controller(c->webview);
    if (arg->i == 0) {
        webkit_find_controller_search_finish(fc);
        c->state.search.matches = 0;
        c->state.search.active  = FALSE;
        vb_statusbar_update(c);
        return TRUE;
    }

    /* copy search query for later use */
    if (arg->s) {
        /* set search direction only when the searching is started */
        c->state.search.direction = arg->i > 0 ? 1 : -1;
        query = arg->s;
        /* add new search query to history and search register */
        vb_register_add(c, '/', query);
        history_add(c, HISTORY_SEARCH, query, NULL);
    } else {
        /* no search phrase given - continue a previous search */
        query = vb_register_get(c, '/');
    }

    forward = (arg->i * c->state.search.direction) > 0;

    if (query) {
        guint count = abs(arg->i);

        if (!c->state.search.active) {
            webkit_find_controller_search(fc, query,
                    WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE |
                    WEBKIT_FIND_OPTIONS_WRAP_AROUND |
                    (forward ?  WEBKIT_FIND_OPTIONS_NONE : WEBKIT_FIND_OPTIONS_BACKWARDS),
                    G_MAXUINT);
            c->state.search.active = TRUE;
            /* Skip first search because the first match is already
             * highlighted on search start. */
            count -= 1;
        }

        if (forward) {
            while (count--) {
                webkit_find_controller_search_next(fc);
            };
        } else {
            while (count--) {
                webkit_find_controller_search_previous(fc);
            };
        }
    }

    return TRUE;
}

gboolean command_yank(Client *c, const Arg *arg, char buf)
{
    /* TODO no implemented yet */
    return TRUE;
}

gboolean command_save(Client *c, const Arg *arg)
{
#if 0
    const char *uri, *path = NULL;

    if (arg->i == COMMAND_SAVE_CURRENT) {
        uri = c->state.uri;
        /* given string is the path to save the download to */
        if (arg->s && *(arg->s) != '\0') {
            path = arg->s;
        }
    } else {
        uri = arg->s;
    }

    if (!uri || !*uri) {
        return FALSE;
    }
#endif

    /* TODO start the download to given path here */
    return TRUE;
}

#ifdef FEATURE_QUEUE
gboolean command_queue(Client *c, const Arg *arg)
{
    /* TODO no implemented yet */
    return TRUE;
}
#endif
