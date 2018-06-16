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
#include <string.h>

#include "ascii.h"
#include "config.h"
#include "input.h"
#include "main.h"
#include "normal.h"
#include "util.h"
#include "scripts/scripts.h"
#include "ext-proxy.h"

typedef struct {
    Client *c;
    char   *file;
    char   *element_id;
    unsigned long element_map_key;
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
    static unsigned long element_map_key = 0;
    char *element_id = NULL, *command = NULL;
    char **argv, *file_path = NULL;
    const char *text = NULL, *id = NULL, *editor_command;
    int argc;
    GPid pid;
    gboolean success;
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
    g_variant_get(idreturn, "(bs)", &success, &id);

    /* Special case: the input element does not have an id assigned to it */
    if (!success || !*id) {
        char *js_command = g_strdup_printf(JS_SET_EDITOR_MAP_ELEMENT, ++element_map_key);
        ext_proxy_eval_script(c, js_command, NULL);
        g_free(js_command);
    } else {
        element_id = g_strdup(id);
    }

    /* create a temp file to pass text to and from editor */
    if (!util_create_tmp_file(text, &file_path)) {
        goto error;
    }

    /* spawn editor */
    command = g_strdup_printf(editor_command, file_path);
    if (!g_shell_parse_argv(command, &argc, &argv, NULL)) {
        g_critical("Could not parse editor-command '%s'", command);
        goto error;
    }

    success = g_spawn_async(
        NULL, argv, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
        NULL, NULL, &pid, &error
    );

    if (!success) {
        g_warning("Could not spawn editor-command: %s", error->message);
        g_error_free(error);
        goto error;
    }
    g_strfreev(argv);

    /* disable the active element */
    ext_proxy_eval_script(c, "vimb_input_mode_element.disabled=true", NULL);

    /* watch the editor process */
    EditorData *data = g_slice_new0(EditorData);
    data->file            = file_path;
    data->c               = c;
    data->element_id      = element_id;
    data->element_map_key = element_map_key;

    g_child_watch_add(pid, (GChildWatchFunc)resume_editor, data);

    return RESULT_COMPLETE;

error:
    unlink(file_path);
    g_free(file_path);
    g_strfreev(argv);
    g_free(element_id);
    return RESULT_ERROR;
}

static void resume_editor(GPid pid, int status, EditorData *data)
{
    char *text, *escaped;
    char *jscode;
    char *jscode_enable;

    g_assert(pid);
    g_assert(data);
    g_assert(data->c);
    g_assert(data->file);

    if (status == 0) {
        /* get the text the editor stored */
        text = util_get_file_contents(data->file, NULL);

        if (text) {
            escaped = util_strescape(text, NULL);

            /* put the text back into the element */
            if (data->element_id && strlen(data->element_id) > 0) {
                jscode = g_strdup_printf("document.getElementById(\"%s\").value=\"%s\"", data->element_id, escaped);
            } else {
                jscode = g_strdup_printf("vimb_editor_map.get(\"%lu\").value=\"%s\"", data->element_map_key, escaped);
            }

            ext_proxy_eval_script(data->c, jscode, NULL);

            g_free(jscode);
            g_free(escaped);
            g_free(text);
        }
    }

    if (data->element_id && strlen(data->element_id) > 0) {
        jscode_enable = g_strdup_printf(JS_FOCUS_ELEMENT_BY_ID,
                data->element_id, data->element_id);
    } else {
        jscode_enable = g_strdup_printf(JS_FOCUS_EDITOR_MAP_ELEMENT,
                data->element_map_key, data->element_map_key);
    }
    ext_proxy_eval_script(data->c, jscode_enable, NULL);
    g_free(jscode_enable);

    g_unlink(data->file);
    g_free(data->file);
    g_free(data->element_id);
    g_slice_free(EditorData, data);
    g_spawn_close_pid(pid);
}
