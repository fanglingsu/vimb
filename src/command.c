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

/**
 * This file contains functions that are used by normal mode and command mode
 * together.
 */
#include "config.h"
#include "main.h"
#include "command.h"
#include "history.h"
#include "bookmark.h"

extern VbCore vb;

gboolean command_search(const Arg *arg)
{
    static short dir;   /* last direction 1 forward, -1 backward*/
    const char *query;
    static gboolean newsearch = true;
    gboolean forward;

    if (arg->i == 0) {
#ifdef FEATURE_SEARCH_HIGHLIGHT
        webkit_web_view_unmark_text_matches(vb.gui.webview);
        vb.state.search_matches = 0;
        vb_update_statusbar();
#endif
        newsearch = true;
        return true;
    }

    /* copy search query for later use */
    if (arg->s) {
        /* set search direction only when the searching is started */
        dir   = arg->i > 0 ? 1 : -1;
        query = arg->s;
        /* add new search query to history and search register */
        vb_register_add('/', query);
        history_add(HISTORY_SEARCH, query, NULL);
    } else {
        /* no search phrase given - continue a previous search */
        query = vb_register_get('/');
    }

    forward = (arg->i * dir) > 0;

    if (query) {
        unsigned int count = abs(arg->i);
        if (newsearch) {
#ifdef FEATURE_SEARCH_HIGHLIGHT
            /* highlight matches if the search is started new or continued
             * after switch to normal mode which calls this function with
             * COMMAND_SEARCH_OFF */
            vb.state.search_matches = webkit_web_view_mark_text_matches(vb.gui.webview, query, false, 0);
            webkit_web_view_set_highlight_text_matches(vb.gui.webview, true);
            vb_update_statusbar();
#endif
            newsearch = false;
            /* skip first search because this is done during typing in ex
             * mode, else the search will mark the next match as active */
            if (count) {
                count -= 1;
            }
        }

        while (count--) {
            if (!webkit_web_view_search_text(vb.gui.webview, query, false, forward, true)) {
                break;
            }
        };
    }

    return true;
}

gboolean command_yank(const Arg *arg, char buf)
{
    static char *tmpl = "Yanked: %s";

    if (arg->i == COMMAND_YANK_SELECTION) {
        char *text = NULL;
        /* copy current selection to clipboard */
        webkit_web_view_copy_clipboard(vb.gui.webview);
        text = gtk_clipboard_wait_for_text(PRIMARY_CLIPBOARD());
        if (!text) {
            text = gtk_clipboard_wait_for_text(SECONDARY_CLIPBOARD());
        }
        if (text) {
            /* put the text into the yank buffer */
            vb_register_add(buf, text);
            vb_echo(VB_MSG_NORMAL, false, tmpl, text);
            g_free(text);

            return true;
        }

        return false;
    }

    Arg a = {VB_CLIPBOARD_PRIMARY|VB_CLIPBOARD_SECONDARY};
    if (arg->i == COMMAND_YANK_URI) {
        /* yank current uri */
        a.s = vb.state.uri;
    } else {
        /* use current arg.s as new clipboard content */
        a.s = arg->s;
    }
    if (a.s) {
        /* put the text into the yank buffer */
        vb_set_clipboard(&a);
        vb_register_add(buf, a.s);
        vb_echo(VB_MSG_NORMAL, false, tmpl, a.s);

        return true;
    }

    return false;
}

gboolean command_save(const Arg *arg)
{
    WebKitDownload *download;
    const char *uri, *path = NULL;

    if (arg->i == COMMAND_SAVE_CURRENT) {
        uri = vb.state.uri;
        /* given string is the path to save the download to */
        if (arg->s && *(arg->s) != '\0') {
            path = arg->s;
        }
    } else {
        uri = arg->s;
    }

    if (!uri || !*uri) {
        return false;
    }

    download = webkit_download_new(webkit_network_request_new(uri));
    vb_download(vb.gui.webview, download, path);

    return true;
}

#ifdef FEATURE_QUEUE
gboolean command_queue(const Arg *arg)
{
    gboolean res = false;
    int count = 0;
    char *uri;

    switch (arg->i) {
        case COMMAND_QUEUE_POP:
            if ((uri = bookmark_queue_pop(&count))) {
                res = vb_load_uri(&(Arg){VB_TARGET_CURRENT, uri});
                g_free(uri);
            }
            vb_echo(VB_MSG_NORMAL, false, "Queue length %d", count);
            break;

        case COMMAND_QUEUE_PUSH:
            res = bookmark_queue_push(arg->s ? arg->s : vb.state.uri);
            if (res) {
                vb_echo(VB_MSG_NORMAL, false, "Pushed to queue");
            }
            break;

        case COMMAND_QUEUE_UNSHIFT:
            res = bookmark_queue_unshift(arg->s ? arg->s : vb.state.uri);
            if (res) {
                vb_echo(VB_MSG_NORMAL, false, "Pushed to queue");
            }
            break;

        case COMMAND_QUEUE_CLEAR:
            if (bookmark_queue_clear()) {
                vb_echo(VB_MSG_NORMAL, false, "Queue cleared");
            }
            break;
    }

    return res;
}
#endif
