/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2018 Daniel Carl
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
#include <stdlib.h>
#include <string.h>

#include "config.h"
#ifdef FEATURE_QUEUE
#include "bookmark.h"
#endif
#include "command.h"
#include "history.h"
#include "util.h"
#include "main.h"

typedef struct {
    Client   *c;
    char     *file;
    gpointer *data;
    PostEditFunc func;
} EditorData;

static void resume_editor(GPid pid, int status, gpointer edata);

/**
 * Start/perform/stop searching in webview.
 *
 * @commit: If TRUE, the search query is registered into register "/
 *          In case searching is stopped the commit of value TRUE
 *          is used to clear the inputbox if search is active. This is needed
 *          in case a link is fired by <CR> on highlighted link.
 */
gboolean command_search(Client *c, const Arg *arg, bool commit)
{
    const char *query;
    guint count;
    int direction;

    g_assert(c);
    g_assert(arg);

    if (arg->i == 0) {
        webkit_find_controller_search_finish(c->finder);

        /* Clear the input only if the search is active and commit flag is
         * set. This allows us to stop searching with and without cleaning
         * inputbox */
        if (commit && c->state.search.active) {
            vb_echo(c, MSG_NORMAL, FALSE, "");
        }

        c->state.search.active  = FALSE;
        c->state.search.matches = 0;

        vb_statusbar_update(c);

        return TRUE;
    }

    query = arg->s;
    count = abs(arg->i);
    direction = arg->i > 0 ? 1 : -1;

    /* restart the last search if a search result is asked for and no
     * search was active */
    if (!query && !c->state.search.active) {
        query = c->state.search.last_query;
        direction = c->state.search.direction;
    }

    /* Only committed search strings adjust registers and are recorded in
     * history, intermediate strings (while using incsearch) don't. */
    if (commit) {
        if (query) {
            history_add(c, HISTORY_SEARCH, query, NULL);
            vb_register_add(c, '/', query);
        } else {
            /* Committed search without string re-searches last string. */
            query = vb_register_get(c, '/');
        }
    }

    /* Hand the query string to webkit's find controller. */
    if (query) {
        /* Force a fresh start in order to have webkit select the first match
         * on the page. Without this workaround the first selected match
         * depends on the most recent selection or caret position (even when
         * caret browsing is disabled). */
        if (commit) {
            webkit_find_controller_search(c->finder, "", WEBKIT_FIND_OPTIONS_NONE, G_MAXUINT);
        }

        if (!c->state.search.last_query) {
            c->state.search.last_query = g_strdup(query);
        } else if (strcmp(c->state.search.last_query, query)) {
            g_free(c->state.search.last_query);
            c->state.search.last_query = g_strdup(query);
        }

        webkit_find_controller_count_matches(c->finder, query,
                WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE |
                WEBKIT_FIND_OPTIONS_WRAP_AROUND,
                G_MAXUINT);
        webkit_find_controller_search(c->finder, query,
                WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE |
                WEBKIT_FIND_OPTIONS_WRAP_AROUND |
                (direction > 0 ?  WEBKIT_FIND_OPTIONS_NONE : WEBKIT_FIND_OPTIONS_BACKWARDS),
                G_MAXUINT);

        c->state.search.active    = TRUE;
        c->state.search.direction = direction;

        /* Skip first search because the first match is already
         * highlighted on search start. */
        count -= 1;
    }

    /* Step through searchs result focus according to arg->i. */
    if (c->state.search.active) {
        if (arg->i * c->state.search.direction > 0) {
            while (count--) {
                webkit_find_controller_search_next(c->finder);
            }
        } else {
            while (count--) {
                webkit_find_controller_search_previous(c->finder);
            }
        }
    }

    return TRUE;
}

gboolean command_yank(Client *c, const Arg *arg, char buf)
{
    /**
     * This implementation is quite 'brute force', same as in vimb2
     *  - both X clipboards are always set, PRIMARY and CLIPBOARD
     *  - the X clipboards are always set, even though a vimb register was given
     */

    const char *uri = NULL;
    char *yanked = NULL;

    g_assert(c);
    g_assert(arg);
    g_assert(c->webview);
    g_assert(
        arg->i == COMMAND_YANK_URI ||
        arg->i == COMMAND_YANK_SELECTION ||
        arg->i == COMMAND_YANK_ARG);

    if (arg->i == COMMAND_YANK_URI) {
        if ((uri = webkit_web_view_get_uri(c->webview))) {
            yanked = g_strdup(uri);
        }
    } else if (arg->i == COMMAND_YANK_SELECTION) {
        /* copy web view selection to clipboard */
        webkit_web_view_execute_editing_command(c->webview, WEBKIT_EDITING_COMMAND_COPY);
        /* read back copy from clipboard */
        yanked = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY));
    } else {
        /* use current arg.s as new clipboard content */
        yanked = g_strdup(arg->s);
    }

    if(!yanked) {
        return FALSE;
    }

    /* store in vimb default register */
    vb_register_add(c, '"', yanked);
    /* store in vimb register buf if buf != 0 */
    vb_register_add(c, buf, yanked);

    /* store in X clipboard primary (selected text copy, middle mouse paste) */
    gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY), yanked, -1);
    /* store in X "windows style" clipboard */
    gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), yanked, -1);

    vb_echo(c, MSG_NORMAL, FALSE, "Yanked: %s", yanked);

    g_free(yanked);

    return TRUE;
}

gboolean command_save(Client *c, const Arg *arg)
{
    const char *uri, *path = NULL;
    WebKitDownload *download;

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

    /* Start the download to given path. */
    download = webkit_web_view_download_uri(c->webview, uri);

    return vb_download_set_destination(c, download, NULL, path);
}

#ifdef FEATURE_QUEUE
gboolean command_queue(Client *c, const Arg *arg)
{
    gboolean res = FALSE;
    int count    = 0;
    char *uri;

    switch (arg->i) {
        case COMMAND_QUEUE_POP:
            if ((uri = bookmark_queue_pop(&count))) {
                res = vb_load_uri(c, &(Arg){TARGET_CURRENT, uri});
                g_free(uri);
            }
            vb_echo(c, MSG_NORMAL, FALSE, "Queue length %d", count);
            break;

        case COMMAND_QUEUE_PUSH:
            res = bookmark_queue_push(arg->s ? arg->s : c->state.uri);
            if (res) {
                vb_echo(c, MSG_NORMAL, FALSE, "Pushed to queue");
            }
            break;

        case COMMAND_QUEUE_UNSHIFT:
            res = bookmark_queue_unshift(arg->s ? arg->s : c->state.uri);
            if (res) {
                vb_echo(c, MSG_NORMAL, FALSE, "Pushed to queue");
            }
            break;

        case COMMAND_QUEUE_CLEAR:
            if (bookmark_queue_clear()) {
                vb_echo(c, MSG_NORMAL, FALSE, "Queue cleared");
            }
            break;
    }

    return res;
}
#endif

/**
 * Asynchronously spawn editor.
 *
 * @posteditfunc: If not NULL posteditfunc is called and the following arguments
 *                are passed:
 *                 - const char *text: text contents of the temporary file (or
 *                                     NULL)
 *                 - Client *c: current client passed to command_spawn_editor()
 *                 - gpointer data: pointer that can be used for any local
 *                                  purposes
 * @data:         Generic pointer used to pass data to posteditfunc
 */
gboolean command_spawn_editor(Client *c, const Arg *arg,
    PostEditFunc posteditfunc, gpointer data)
{
    char **argv = NULL, *file_path = NULL;
    char *command = NULL;
    int argc;
    GPid pid;
    gboolean success, result;
    char *editor_command;
    GError *error = NULL;

    /* get the editor command */
    editor_command = GET_CHAR(c, "editor-command");
    if (!editor_command || !*editor_command) {
        vb_echo(c, MSG_ERROR, TRUE, "No editor-command configured");
        return FALSE;
    }

    /* create a temp file to pass text in/to editor */
    if (!util_create_tmp_file(arg->s, &file_path)) {
        return FALSE;
    }

    /* spawn editor */
    command = g_strdup_printf(editor_command, file_path);
    if (g_shell_parse_argv(command, &argc, &argv, NULL)) {
        success = g_spawn_async(
            NULL, argv, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
            NULL, NULL, &pid, &error
        );
        g_strfreev(argv);

        if (success) {
            EditorData *ed = g_slice_new0(EditorData);
            ed->file = file_path;
            ed->c    = c;
            ed->data = data;
            ed->func = posteditfunc;

            g_child_watch_add(pid, resume_editor, ed);

            result = TRUE;
        } else {
            g_warning("Could not spawn editor-command: %s", error->message);
            g_error_free(error);
            result = FALSE;
        }
    } else {
        g_critical("Could not parse editor-command '%s'", command);
        result = FALSE;
    }
    g_free(command);

    if (!result) {
        unlink(file_path);
        g_free(file_path);
    }
    return result;
}

static void resume_editor(GPid pid, int status, gpointer edata)
{
    char *text = NULL;
    EditorData *ed = edata;

    g_assert(pid);
    g_assert(ed);
    g_assert(ed->c);
    g_assert(ed->file);

    if (ed->func != NULL) {
        text = util_get_file_contents(ed->file, NULL);
        ed->func(text, ed->c, ed->data);
        g_free(text);
    }

    unlink(ed->file);
    g_free(ed->file);
    g_slice_free(EditorData, ed);
    g_spawn_close_pid(pid);
}
