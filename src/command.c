/**
 * vimp - a webkit based vim like browser.
 *
 * Copyright (C) 2012 Daniel Carl
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

#include "main.h"
#include "command.h"
#include "keybind.h"
#include "setting.h"
#include "completion.h"
#include "hints.h"

extern const char *inputbox_font[2];
extern const char *inputbox_fg[2];
extern const char *inputbox_bg[2];

static CommandInfo cmd_list[] = {
    /* command           function             arg                                                                           mode */
    {"open",             command_open,        {VP_MODE_NORMAL, ""},                                                         VP_MODE_NORMAL},
    {"input",            command_input,       {0, ":"},                                                                     VP_MODE_COMMAND},
    {"inputopen",        command_input,       {0, ":open "},                                                                VP_MODE_COMMAND},
    {"inputopencurrent", command_input,       {VP_INPUT_CURRENT_URI, ":open "},                                             VP_MODE_NORMAL},
    {"quit",             command_close,       {0},                                                                          VP_MODE_NORMAL},
    {"source",           command_view_source, {0},                                                                          VP_MODE_NORMAL},
    {"back",             command_navigate,    {VP_NAVIG_BACK},                                                              VP_MODE_NORMAL},
    {"forward",          command_navigate,    {VP_NAVIG_FORWARD},                                                           VP_MODE_NORMAL},
    {"reload",           command_navigate,    {VP_NAVIG_RELOAD},                                                            VP_MODE_NORMAL},
    {"reload!",          command_navigate,    {VP_NAVIG_RELOAD_FORCE},                                                      VP_MODE_NORMAL},
    {"stop",             command_navigate,    {VP_NAVIG_STOP_LOADING},                                                      VP_MODE_NORMAL},
    {"jumpleft",         command_scroll,      {VP_SCROLL_TYPE_JUMP | VP_SCROLL_DIRECTION_LEFT},                             VP_MODE_NORMAL},
    {"jumpright",        command_scroll,      {VP_SCROLL_TYPE_JUMP | VP_SCROLL_DIRECTION_RIGHT},                            VP_MODE_NORMAL},
    {"jumptop",          command_scroll,      {VP_SCROLL_TYPE_JUMP | VP_SCROLL_DIRECTION_TOP},                              VP_MODE_NORMAL},
    {"jumpbottom",       command_scroll,      {VP_SCROLL_TYPE_JUMP | VP_SCROLL_DIRECTION_DOWN},                             VP_MODE_NORMAL},
    {"pageup",           command_scroll,      {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_TOP | VP_SCROLL_UNIT_PAGE},      VP_MODE_NORMAL},
    {"pagedown",         command_scroll,      {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_DOWN | VP_SCROLL_UNIT_PAGE},     VP_MODE_NORMAL},
    {"halfpageup",       command_scroll,      {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_TOP | VP_SCROLL_UNIT_HALFPAGE},  VP_MODE_NORMAL},
    {"halfpagedown",     command_scroll,      {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_DOWN | VP_SCROLL_UNIT_HALFPAGE}, VP_MODE_NORMAL},
    {"scrollleft",       command_scroll,      {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_LEFT | VP_SCROLL_UNIT_LINE},     VP_MODE_NORMAL},
    {"scrollright",      command_scroll,      {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_RIGHT | VP_SCROLL_UNIT_LINE},    VP_MODE_NORMAL},
    {"scrollup",         command_scroll,      {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_TOP | VP_SCROLL_UNIT_LINE},      VP_MODE_NORMAL},
    {"scrolldown",       command_scroll,      {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_DOWN | VP_SCROLL_UNIT_LINE},     VP_MODE_NORMAL},
    {"nmap",             command_map,         {VP_MODE_NORMAL},                                                             VP_MODE_NORMAL},
    {"imap",             command_map,         {VP_MODE_INSERT},                                                             VP_MODE_NORMAL},
    {"cmap",             command_map,         {VP_MODE_COMMAND},                                                            VP_MODE_NORMAL},
    {"hmap",             command_map,         {VP_MODE_HINTING},                                                            VP_MODE_NORMAL},
    {"nunmap",           command_unmap,       {VP_MODE_NORMAL},                                                             VP_MODE_NORMAL},
    {"iunmap",           command_unmap,       {VP_MODE_INSERT},                                                             VP_MODE_NORMAL},
    {"cunmap",           command_unmap,       {VP_MODE_COMMAND},                                                            VP_MODE_NORMAL},
    {"hunmap",           command_unmap,       {VP_MODE_HINTING},                                                            VP_MODE_NORMAL},
    {"set",              command_set,         {0},                                                                          VP_MODE_NORMAL},
    {"complete",         command_complete,    {0},                                                                          VP_MODE_COMMAND | VP_MODE_COMPLETE},
    {"complete-back",    command_complete,    {1},                                                                          VP_MODE_COMMAND | VP_MODE_COMPLETE},
    {"inspect",          command_inspect,     {0},                                                                          VP_MODE_NORMAL},
    {"hint-link",        command_hints,       {0},                                                                          VP_MODE_HINTING},
    {"hint-focus-next",  command_hints_focus, {0},                                                                          VP_MODE_HINTING},
    {"hint-focus-prev",  command_hints_focus, {1},                                                                          VP_MODE_HINTING},
};

static void command_write_input(const gchar* str);


void command_init(void)
{
    guint i;
    vp.behave.commands = g_hash_table_new(g_str_hash, g_str_equal);

    for (i = 0; i < LENGTH(cmd_list); i++) {
        g_hash_table_insert(vp.behave.commands, (gpointer)cmd_list[i].name, &cmd_list[i]);
    }
}

void command_cleanup(void)
{
    if (vp.behave.commands) {
        g_hash_table_destroy(vp.behave.commands);
    }
}

gboolean command_exists(const gchar* name)
{
    return g_hash_table_contains(vp.behave.commands, name);
}

gboolean command_run(const gchar* name, const gchar* param)
{
    CommandInfo* c = NULL;
    gboolean result;
    Arg a;
    c = g_hash_table_lookup(vp.behave.commands, name);
    if (!c) {
        vp_echo(VP_MSG_ERROR, TRUE, "Command '%s' not found", name);
        return FALSE;
    }
    a.i = c->arg.i;
    a.s = g_strdup(param ? param : c->arg.s);
    result = c->function(&a);
    g_free(a.s);

    /* set the new mode */
    vp_set_mode(c->mode, FALSE);

    return result;
}

gboolean command_open(const Arg* arg)
{
    return vp_load_uri(arg);
}

gboolean command_input(const Arg* arg)
{
    const gchar* url;
    gchar* input = NULL;

    /* add current url if requested */
    if (VP_INPUT_CURRENT_URI == arg->i
        && (url = webkit_web_view_get_uri(vp.gui.webview))
    ) {
        /* append the crrent url to the input message */
        input = g_strconcat(arg->s, url, NULL);
        command_write_input(input);
        g_free(input);
    } else {
        command_write_input(arg->s);
    }

    return vp_set_mode(VP_MODE_COMMAND, FALSE);
}

gboolean command_close(const Arg* arg)
{
    gtk_main_quit();
    vp_clean_up();

    return TRUE;
}

gboolean command_view_source(const Arg* arg)
{
    gboolean mode = webkit_web_view_get_view_source_mode(vp.gui.webview);
    webkit_web_view_set_view_source_mode(vp.gui.webview, !mode);
    webkit_web_view_reload(vp.gui.webview);

    return TRUE;
}

gboolean command_navigate(const Arg* arg)
{
    if (arg->i <= VP_NAVIG_FORWARD) {
        /* TODO allow to set a count for the navigation */
        webkit_web_view_go_back_or_forward(
            vp.gui.webview, (arg->i == VP_NAVIG_BACK ? -1 : 1)
        );
    } else if (arg->i == VP_NAVIG_RELOAD) {
        webkit_web_view_reload(vp.gui.webview);
    } else if (arg->i == VP_NAVIG_RELOAD_FORCE) {
        webkit_web_view_reload_bypass_cache(vp.gui.webview);
    } else {
        webkit_web_view_stop_loading(vp.gui.webview);
    }

    return TRUE;
}

gboolean command_scroll(const Arg* arg)
{
    GtkAdjustment *adjust = (arg->i & VP_SCROLL_AXIS_H) ? vp.gui.adjust_h : vp.gui.adjust_v;

    gint direction  = (arg->i & (1 << 2)) ? 1 : -1;

    /* type scroll */
    if (arg->i & VP_SCROLL_TYPE_SCROLL) {
        gdouble value;
        gint count = vp.state.count ? vp.state.count : 1;
        if (arg->i & VP_SCROLL_UNIT_LINE) {
            value = vp.config.scrollstep;
        } else if (arg->i & VP_SCROLL_UNIT_HALFPAGE) {
            value = gtk_adjustment_get_page_size(adjust) / 2;
        } else {
            value = gtk_adjustment_get_page_size(adjust);
        }
        gtk_adjustment_set_value(adjust, gtk_adjustment_get_value(adjust) + direction * value * count);
    } else if (vp.state.count) {
        /* jump - if count is set to count% of page */
        gdouble max = gtk_adjustment_get_upper(adjust) - gtk_adjustment_get_page_size(adjust);
        gtk_adjustment_set_value(adjust, max * vp.state.count / 100);
    } else if (direction == 1) {
        /* jump to top */
        gtk_adjustment_set_value(adjust, gtk_adjustment_get_upper(adjust));
    } else {
        /* jump to bottom */
        gtk_adjustment_set_value(adjust, gtk_adjustment_get_lower(adjust));
    }

    return TRUE;
}

gboolean command_map(const Arg* arg)
{
    return keybind_add_from_string(arg->s, arg->i);
}

gboolean command_unmap(const Arg* arg)
{
    return keybind_remove_from_string(arg->s, arg->i);
}

gboolean command_set(const Arg* arg)
{
    gboolean success;
    gchar* line = NULL;
    gchar** token;

    if (!arg->s || !strlen(arg->s)) {
        return FALSE;
    }

    line = g_strdup(arg->s);
    g_strstrip(line);

    /* split the input string into paramete and value part */
    token = g_strsplit(line, "=", 2);
    g_free(line);

    if (!token[1]) {
        /* TODO display current value */
        g_strfreev(token);
        vp_echo(VP_MSG_ERROR, TRUE, "No param given");
        return FALSE;
    }
    success = setting_run(token[0], token[1] ? token[1] : NULL);
    g_strfreev(token);

    return success;
}

gboolean command_complete(const Arg* arg)
{
    completion_complete(arg->i ? TRUE : FALSE);

    return TRUE;
}

gboolean command_inspect(const Arg* arg)
{
    gboolean enabled;
    WebKitWebSettings* settings = NULL;

    settings = webkit_web_view_get_settings(vp.gui.webview);
    g_object_get(G_OBJECT(settings), "enable-developer-extras", &enabled, NULL);
    if (enabled) {
        webkit_web_inspector_show(vp.gui.inspector);
        return TRUE;
    } else {
        vp_echo(VP_MSG_ERROR, TRUE, "enable-developer-extras not enabled");
        return FALSE;
    }
}

gboolean command_hints(const Arg* arg)
{
    hints_create(NULL, arg->i);

    return TRUE;
}

gboolean command_hints_focus(const Arg* arg)
{
    hints_focus_next(arg->i ? TRUE : FALSE);

    return TRUE;
}

static void command_write_input(const gchar* str)
{
    gint pos = 0;
    /* reset the colors and fonts to defalts */
    vp_set_widget_font(
        vp.gui.inputbox,
        &vp.style.input_fg[VP_MSG_NORMAL],
        &vp.style.input_bg[VP_MSG_NORMAL],
        vp.style.input_font[VP_MSG_NORMAL]
    );

    /* remove content from input box */
    gtk_entry_set_text(GTK_ENTRY(vp.gui.inputbox), "");

    /* insert string from arg */
    gtk_editable_insert_text(GTK_EDITABLE(vp.gui.inputbox), str, -1, &pos);
    gtk_editable_set_position(GTK_EDITABLE(vp.gui.inputbox), -1);
}
