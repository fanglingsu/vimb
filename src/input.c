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

#include <glib.h>
#include <glib/gstdio.h>

#include "ascii.h"
#include "config.h"
#include "input.h"
#include "main.h"
#include "normal.h"
#include "util.h"
#include "ext-proxy.h"

typedef struct {
    Client *c;
    char   *file;
	 char   *element_id;
} EditorData;

static void resume_editor(GPid pid, int status, EditorData *data);

/**
 * Function called when vimb enters the input mode.
 */
void input_enter(Client *c)
{
    /* switch focus first to make sure we can write to the inputbox without
     * disturbing the user */
    gtk_widget_grab_focus(GTK_WIDGET(c->webview));
    vb_modelabel_update(c, "-- INPUT --");
    ext_proxy_eval_script(c, "var vimb_input_mode_element = document.activeElement;", NULL);
}

/**
 * Called when the input mode is left.
 */
void input_leave(Client *c)
{
    ext_proxy_eval_script(c, "vimb_input_mode_element.blur();", NULL);
    vb_modelabel_update(c, "");
}

/**
 * Handles the keypress events from webview and inputbox.
 */
VbResult input_keypress(Client *c, int key)
{
    static gboolean ctrlo = FALSE;

    if (ctrlo) {
        /* if we are in ctrl-O mode perform the next keys as normal mode
         * commands until the command is complete or error */
        VbResult res = normal_keypress(c, key);
        if (res != RESULT_MORE) {
            ctrlo = FALSE;
            /* Don't overwrite the mode label in case we landed in another
             * mode. This might occurre by CTRL-0 CTRL-Z or after running ex
             * command, where we mainly end up in normal mode. */
            if (c->mode->id == 'i') {
                /* reenter the input mode */
                input_enter(c);
            }
        }
        return res;
    }

    switch (key) {
        case CTRL('['): /* esc */
            vb_enter(c, 'n');
            return RESULT_COMPLETE;

        case CTRL('O'):
            /* enter CTRL-0 mode to execute next command in normal mode */
            ctrlo           = TRUE;
            c->mode->flags |= FLAG_NOMAP;
            vb_modelabel_update(c, "-- (input) --");
            return RESULT_MORE;

        case CTRL('T'):
            return input_open_editor(c);

        case CTRL('Z'):
            vb_enter(c, 'p');
            return RESULT_COMPLETE;
    }

    c->state.processed_key = FALSE;
    return RESULT_ERROR;
}

VbResult input_open_editor(Client *c)
{
    char **argv, *file_path = NULL;
    const char *text = NULL, *id = NULL, *editor_command;
    int argc;
    GPid pid;
    gboolean success, idSuccess;
    GVariant *jsreturn;
    GVariant *idreturn;
    GError *error = NULL;

    g_assert(c);

    /* get the editor command */
    editor_command = GET_CHAR(c, "editor-command");
    if (!editor_command || !*editor_command) {
        vb_echo(c, MSG_ERROR, TRUE, "No editor-command configured");
        return RESULT_ERROR;
    }

    /* get the selected input element */
    jsreturn = ext_proxy_eval_script_sync(c, "vimb_input_mode_element.value");
    g_variant_get(jsreturn, "(bs)", &success, &text);

    if (!success || !text) {
        return RESULT_ERROR;
    }

    idreturn = ext_proxy_eval_script_sync(c, "vimb_input_mode_element.id");
    g_variant_get(idreturn, "(bs)", &idSuccess, &id);

    /* create a temp file to pass text to and from editor */
    if (!util_create_tmp_file(text, &file_path)) {
        return RESULT_ERROR;
    }

    /* spawn editor */
    char *command = g_strdup_printf(editor_command, file_path);
    if (!g_shell_parse_argv(command, &argc, &argv, NULL)) {
        g_critical("Could not parse editor-command '%s'", command);
        g_free(command);
        return RESULT_ERROR;
    }
    g_free(command);

    success = g_spawn_async(
        NULL, argv, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
        NULL, NULL, &pid, &error
    );
    g_strfreev(argv);

    if (!success) {
        unlink(file_path);
        g_free(file_path);
        g_warning("Could not spawn editor-command: %s", error->message);
        g_error_free(error);
        return RESULT_ERROR;
    }

    /* disable the active element */
    ext_proxy_eval_script(c, "vimb_input_mode_element.disabled=true", NULL);

    /* watch the editor process */
    EditorData *data = g_slice_new0(EditorData);
    data->file = file_path;
    data->c    = c;
	 data->element_id   = g_strdup(id);
    g_child_watch_add(pid, (GChildWatchFunc)resume_editor, data);

    return RESULT_COMPLETE;
}

static void resume_editor(GPid pid, int status, EditorData *data)
{
    char *text, *escaped;
    char *jscode;

    g_assert(pid);
    g_assert(data);
    g_assert(data->c);
    g_assert(data->file);

    if (status == 0) {
        /* get the text the editor stored */
        text = util_get_file_contents(data->file, NULL);

        if (text) {
            escaped = g_strescape(text, NULL);

            /* put the text back into the element */
            jscode = g_strdup_printf("document.getElementById(\"%s\").value=\"%s\"", data->element_id, escaped);
            ext_proxy_eval_script(data->c, jscode, NULL);

            g_free(jscode);
            g_free(escaped);
            g_free(text);
        }
    }

	 char *jscode_enable = g_strdup_printf(
				 "document.getElementById(\"%s\").disabled=false;"
				 "document.getElementById(\"%s\").focus()"
			 , data->element_id, data->element_id);
    ext_proxy_eval_script(data->c,
			 jscode_enable, NULL);

    g_unlink(data->file);
    g_free(data->file);
    g_slice_free(EditorData, data);
    g_spawn_close_pid(pid);
}
