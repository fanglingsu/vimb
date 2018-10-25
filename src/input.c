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
#include "command.h"
#include "config.h"
#include "input.h"
#include "main.h"
#include "normal.h"
#include "util.h"
#include "scripts/scripts.h"
#include "ext-proxy.h"

typedef struct {
    char   *element_id;
    unsigned long element_map_key;
} ElementEditorData;

static void input_editor_formfiller(const char *text, Client *c, gpointer data);

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
    char *element_id = NULL;
    char *text = NULL, *id = NULL;
    gboolean success;
    GVariant *jsreturn;
    GVariant *idreturn;
    ElementEditorData *data = NULL;

    g_assert(c);

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

    data                    = g_slice_new0(ElementEditorData);
    data->element_id        = element_id;
    data->element_map_key   = element_map_key;

    if (command_spawn_editor(c, &((Arg){0, text}), input_editor_formfiller, data)) {
        /* disable the active element */
        ext_proxy_lock_input(c, element_id);

        return RESULT_COMPLETE;
    }

    g_free(element_id);
    g_slice_free(ElementEditorData, data);
    return RESULT_ERROR;
}

static void input_editor_formfiller(const char *text, Client *c, gpointer data)
{
    char *escaped;
    char *jscode;
    char *jscode_enable;
    ElementEditorData *eed = (ElementEditorData *)data;

    if (text) {
        escaped = util_strescape(text, NULL);

        /* put the text back into the element */
        if (eed->element_id && strlen(eed->element_id) > 0) {
            jscode = g_strdup_printf("document.getElementById(\"%s\").value=\"%s\"", eed->element_id, escaped);
        } else {
            jscode = g_strdup_printf("vimb_editor_map.get(\"%lu\").value=\"%s\"", eed->element_map_key, escaped);
        }

        ext_proxy_eval_script(c, jscode, NULL);

        g_free(jscode);
        g_free(escaped);
    }

    if (eed->element_id && strlen(eed->element_id) > 0) {
        ext_proxy_unlock_input(c, eed->element_id);
    } else {
        jscode_enable = g_strdup_printf(JS_FOCUS_EDITOR_MAP_ELEMENT,
                eed->element_map_key, eed->element_map_key);
        ext_proxy_eval_script(c, jscode_enable, NULL);
        g_free(jscode_enable);
    }

    g_free(eed->element_id);
    g_slice_free(ElementEditorData, eed);
}
