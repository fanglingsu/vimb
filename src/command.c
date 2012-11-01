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

static CommandInfo cmd_list[] = {
    /* command           function          arg */
    {"normal",           vp_set_mode,      {VP_MODE_NORMAL, ""}},
    {"open",             vp_open,          {VP_MODE_NORMAL, ""}},
    {"input",            vp_input,         {0, ":"}},
    {"inputopen",        vp_input,         {0, ":open "}},
    {"inputopencurrent", vp_input,         {VP_INPUT_CURRENT_URI, ":open "}},
    {"quit",             vp_close_browser, {0}},
    {"source",           vp_view_source,   {0}},
    {"back",             vp_navigate,      {VP_NAVIG_BACK}},
    {"forward",          vp_navigate,      {VP_NAVIG_FORWARD}},
    {"reload",           vp_navigate,      {VP_NAVIG_RELOAD}},
    {"reload!",          vp_navigate,      {VP_NAVIG_RELOAD_FORCE}},
    {"stop",             vp_navigate,      {VP_NAVIG_STOP_LOADING}},
    {"jumpleft",         vp_scroll,        {VP_SCROLL_TYPE_JUMP | VP_SCROLL_DIRECTION_LEFT}},
    {"jumpright",        vp_scroll,        {VP_SCROLL_TYPE_JUMP | VP_SCROLL_DIRECTION_RIGHT}},
    {"jumptop",          vp_scroll,        {VP_SCROLL_TYPE_JUMP | VP_SCROLL_DIRECTION_TOP}},
    {"jumpbottom",       vp_scroll,        {VP_SCROLL_TYPE_JUMP | VP_SCROLL_DIRECTION_DOWN}},
    {"pageup",           vp_scroll,        {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_TOP | VP_SCROLL_UNIT_PAGE}},
    {"pagedown",         vp_scroll,        {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_DOWN | VP_SCROLL_UNIT_PAGE}},
    {"halfpageup",       vp_scroll,        {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_TOP | VP_SCROLL_UNIT_HALFPAGE}},
    {"halfpagedown",     vp_scroll,        {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_DOWN | VP_SCROLL_UNIT_HALFPAGE}},
    {"scrollleft",       vp_scroll,        {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_LEFT | VP_SCROLL_UNIT_LINE}},
    {"scrollright",      vp_scroll,        {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_RIGHT | VP_SCROLL_UNIT_LINE}},
    {"scrollup",         vp_scroll,        {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_TOP | VP_SCROLL_UNIT_LINE}},
    {"scrolldown",       vp_scroll,        {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_DOWN | VP_SCROLL_UNIT_LINE}},
    {"nmap",             vp_map,           {VP_MODE_NORMAL}},
    {"imap",             vp_map,           {VP_MODE_INSERT}},
    {"cmap",             vp_map,           {VP_MODE_COMMAND}},
    {"nunmap",           vp_unmap,         {VP_MODE_NORMAL}},
    {"iunmap",           vp_unmap,         {VP_MODE_INSERT}},
    {"cunmap",           vp_unmap,         {VP_MODE_COMMAND}},
    {"set",              vp_set,           {0}},
};

void command_init()
{
    guint i;
    vp.behave.commands = g_hash_table_new(g_str_hash, g_str_equal);

    for (i = 0; i < LENGTH(cmd_list); i++) {
        g_hash_table_insert(vp.behave.commands, (gpointer)cmd_list[i].name, &cmd_list[i]);
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
