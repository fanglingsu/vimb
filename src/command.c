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

extern const char *inputbox_font[2];
extern const char *inputbox_fg[2];
extern const char *inputbox_bg[2];

static CommandInfo cmd_list[] = {
    /* command           function               arg */
    {"open",             command_open,          {VP_MODE_NORMAL, ""}},
    {"input",            command_input,         {0, ":"}},
    {"inputopen",        command_input,         {0, ":open "}},
    {"inputopencurrent", command_input,         {VP_INPUT_CURRENT_URI, ":open "}},
    {"quit",             command_close,         {0}},
    {"source",           command_view_source,   {0}},
    {"back",             command_navigate,      {VP_NAVIG_BACK}},
    {"forward",          command_navigate,      {VP_NAVIG_FORWARD}},
    {"reload",           command_navigate,      {VP_NAVIG_RELOAD}},
    {"reload!",          command_navigate,      {VP_NAVIG_RELOAD_FORCE}},
    {"stop",             command_navigate,      {VP_NAVIG_STOP_LOADING}},
    {"jumpleft",         command_scroll,        {VP_SCROLL_TYPE_JUMP | VP_SCROLL_DIRECTION_LEFT}},
    {"jumpright",        command_scroll,        {VP_SCROLL_TYPE_JUMP | VP_SCROLL_DIRECTION_RIGHT}},
    {"jumptop",          command_scroll,        {VP_SCROLL_TYPE_JUMP | VP_SCROLL_DIRECTION_TOP}},
    {"jumpbottom",       command_scroll,        {VP_SCROLL_TYPE_JUMP | VP_SCROLL_DIRECTION_DOWN}},
    {"pageup",           command_scroll,        {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_TOP | VP_SCROLL_UNIT_PAGE}},
    {"pagedown",         command_scroll,        {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_DOWN | VP_SCROLL_UNIT_PAGE}},
    {"halfpageup",       command_scroll,        {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_TOP | VP_SCROLL_UNIT_HALFPAGE}},
    {"halfpagedown",     command_scroll,        {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_DOWN | VP_SCROLL_UNIT_HALFPAGE}},
    {"scrollleft",       command_scroll,        {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_LEFT | VP_SCROLL_UNIT_LINE}},
    {"scrollright",      command_scroll,        {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_RIGHT | VP_SCROLL_UNIT_LINE}},
    {"scrollup",         command_scroll,        {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_TOP | VP_SCROLL_UNIT_LINE}},
    {"scrolldown",       command_scroll,        {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_DOWN | VP_SCROLL_UNIT_LINE}},
    {"nmap",             command_map,           {VP_MODE_NORMAL}},
    {"imap",             command_map,           {VP_MODE_INSERT}},
    {"cmap",             command_map,           {VP_MODE_COMMAND}},
    {"nunmap",           command_unmap,         {VP_MODE_NORMAL}},
    {"iunmap",           command_unmap,         {VP_MODE_INSERT}},
    {"cunmap",           command_unmap,         {VP_MODE_COMMAND}},
    {"set",              command_set,           {0}},
};

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
        vp_echo(VP_MSG_ERROR, "Command '%s' not found", name);
        return FALSE;
    }
    a.i = c->arg.i;
    a.s = g_strdup(param ? param : c->arg.s);
    result = c->function(&a);
    g_free(a.s);

    /* if command was run, remove the modkey and count */
    vp.state.modkey = vp.state.count = 0;
    vp_update_statusbar();

    return result;
}

gboolean command_open(const Arg* arg)
{
    return vp_load_uri(arg);
}

gboolean command_input(const Arg* arg)
{
    gint pos = 0;
    const gchar* url;

    /* reset the colors and fonts to defalts */
    vp_set_widget_font(vp.gui.inputbox, inputbox_font[0], inputbox_bg[0], inputbox_fg[0]);

    /* remove content from input box */
    gtk_entry_set_text(GTK_ENTRY(vp.gui.inputbox), "");

    /* insert string from arg */
    gtk_editable_insert_text(GTK_EDITABLE(vp.gui.inputbox), arg->s, -1, &pos);

    /* add current url if requested */
    if (VP_INPUT_CURRENT_URI == arg->i
            && (url = webkit_web_view_get_uri(vp.gui.webview))) {
        gtk_editable_insert_text(GTK_EDITABLE(vp.gui.inputbox), url, -1, &pos);
    }

    gtk_editable_set_position(GTK_EDITABLE(vp.gui.inputbox), -1);

    Arg a = {VP_MODE_COMMAND};
    return vp_set_mode(&a);
}

gboolean command_close(const Arg* arg)
{
    vp_clean_up();
    gtk_main_quit();

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
        vp_echo(VP_MSG_ERROR, "No param given");
        return FALSE;
    }
    success = setting_run(token[0], token[1] ? token[1] : NULL);
    g_strfreev(token);

    return success;
}
