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

#include "config.h"
#include <glib/gstdio.h>
#include "mode.h"
#include "main.h"
#include "input.h"
#include "dom.h"
#include "util.h"
#include "ascii.h"
#include "normal.h"

typedef struct {
    char    *file;
    Element *element;
} EditorData;

static void resume_editor(GPid pid, int status, EditorData *data);

extern VbCore vb;

/**
 * Function called when vimb enters the input mode.
 */
void input_enter(void)
{
    /* switch focus first to make sure we can write to the inputbox without
     * disturbing the user */
    gtk_widget_grab_focus(GTK_WIDGET(vb.gui.webview));
    vb_echo(VB_MSG_NORMAL, false, "-- INPUT --");
}

/**
 * Called when the input mode is left.
 */
void input_leave(void)
{
    vb_set_input_text("");
}

/**
 * Handles the keypress events from webview and inputbox.
 */
VbResult input_keypress(int key)
{
    static gboolean ctrlo = false;

    if (ctrlo) {
        /* if we are in ctrl-O mode perform the next keys as normal mode
         * commands until the command is complete or error */
        VbResult res = normal_keypress(key);
        if (res != RESULT_MORE) {
            ctrlo = false;
            vb_echo(VB_MSG_NORMAL, false, "-- INPUT --");
        }
        return res;
    }

    switch (key) {
        case CTRL('['): /* esc */
            mode_enter('n');
            return RESULT_COMPLETE;

        case CTRL('O'):
            /* enter CTRL-0 mode to execute next command in normal mode */
            ctrlo           = true;
            vb.mode->flags |= FLAG_NOMAP;
            vb_echo(VB_MSG_NORMAL, false, "-- (input) --");
            return RESULT_MORE;

        case CTRL('T'):
            return input_open_editor();

        case CTRL('Z'):
            mode_enter('p');
            return RESULT_COMPLETE;
    }

    vb.state.processed_key = false;
    return RESULT_ERROR;
}

VbResult input_open_editor(void)
{
    char **argv, *file_path = NULL;
    const char *text, *editor_command;
    int argc;
    GPid pid;
    gboolean success;

    editor_command = GET_CHAR("editor-command");
    if (!editor_command || !*editor_command) {
        vb_echo(VB_MSG_ERROR, true, "No editor-command configured");
        return RESULT_ERROR;
    }
    Element* active = dom_get_active_element(vb.gui.webview);

    /* check if element is suitable for editing */
    if (!active || !dom_is_editable(active)) {
        return RESULT_ERROR;
    }

    text = dom_editable_element_get_value(active);
    if (!text) {
        return RESULT_ERROR;
    }

    if (!util_create_tmp_file(text, &file_path)) {
        return RESULT_ERROR;
    }

    /* spawn editor */
    char* command = g_strdup_printf(editor_command, file_path);
    if (!g_shell_parse_argv(command, &argc, &argv, NULL)) {
        g_critical("Could not parse editor-command '%s'", command);
        g_free(command);
        return RESULT_ERROR;
    }
    g_free(command);

    success = g_spawn_async(
        NULL, argv, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
        NULL, NULL, &pid, NULL
    );
    g_strfreev(argv);

    if (!success) {
        unlink(file_path);
        g_free(file_path);
        g_warning("Could not spawn editor-command");
        return RESULT_ERROR;
    }

    /* disable the active element */
    dom_editable_element_set_disable(active, true);

    EditorData *data = g_slice_new0(EditorData);
    data->file    = file_path;
    data->element = active;

    g_child_watch_add(pid, (GChildWatchFunc)resume_editor, data);

    return RESULT_COMPLETE;
}

static void resume_editor(GPid pid, int status, EditorData *data)
{
    char *text;
    if (status == 0) {
        text = util_get_file_contents(data->file, NULL);
        if (text) {
            dom_editable_element_set_value(data->element, text);
        }
        g_free(text);
    }
    dom_editable_element_set_disable(data->element, false);

    g_unlink(data->file);
    g_free(data->file);
    g_slice_free(EditorData, data);
    g_spawn_close_pid(pid);
}
