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
#include "util.h"

static CommandInfo cmd_list[] = {
    /* command              function             arg                                                                                 mode */
    {"open",                command_open,        {VP_TARGET_CURRENT},                                                                VP_MODE_NORMAL},
    {"tabopen",             command_open,        {VP_TARGET_NEW},                                                                    VP_MODE_NORMAL},
    {"open-home",           command_open_home,   {VP_TARGET_CURRENT},                                                                VP_MODE_NORMAL},
    {"tabopen-home",        command_open_home,   {VP_TARGET_NEW},                                                                    VP_MODE_NORMAL},
    {"open-closed",         command_open_closed, {VP_TARGET_CURRENT},                                                                VP_MODE_NORMAL},
    {"tabopen-closed",      command_open_closed, {VP_TARGET_NEW},                                                                    VP_MODE_NORMAL},
    {"input",               command_input,       {0, ":"},                                                                           VP_MODE_COMMAND},
    {"inputopen",           command_input,       {0, ":open "},                                                                      VP_MODE_COMMAND},
    {"inputtabopen",        command_input,       {0, ":tabopen "},                                                                   VP_MODE_COMMAND},
    {"inputopencurrent",    command_input,       {VP_INPUT_CURRENT_URI, ":open "},                                                   VP_MODE_COMMAND},
    {"inputtabopencurrent", command_input,       {VP_INPUT_CURRENT_URI, ":tabopen "},                                                VP_MODE_COMMAND},
    {"quit",                command_close,       {0},                                                                                VP_MODE_NORMAL},
    {"source",              command_view_source, {0},                                                                                VP_MODE_NORMAL},
    {"back",                command_navigate,    {VP_NAVIG_BACK},                                                                    VP_MODE_NORMAL},
    {"forward",             command_navigate,    {VP_NAVIG_FORWARD},                                                                 VP_MODE_NORMAL},
    {"reload",              command_navigate,    {VP_NAVIG_RELOAD},                                                                  VP_MODE_NORMAL},
    {"reload!",             command_navigate,    {VP_NAVIG_RELOAD_FORCE},                                                            VP_MODE_NORMAL},
    {"stop",                command_navigate,    {VP_NAVIG_STOP_LOADING},                                                            VP_MODE_NORMAL},
    {"jumpleft",            command_scroll,      {VP_SCROLL_TYPE_JUMP | VP_SCROLL_DIRECTION_LEFT},                                   VP_MODE_NORMAL},
    {"jumpright",           command_scroll,      {VP_SCROLL_TYPE_JUMP | VP_SCROLL_DIRECTION_RIGHT},                                  VP_MODE_NORMAL},
    {"jumptop",             command_scroll,      {VP_SCROLL_TYPE_JUMP | VP_SCROLL_DIRECTION_TOP},                                    VP_MODE_NORMAL},
    {"jumpbottom",          command_scroll,      {VP_SCROLL_TYPE_JUMP | VP_SCROLL_DIRECTION_DOWN},                                   VP_MODE_NORMAL},
    {"pageup",              command_scroll,      {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_TOP | VP_SCROLL_UNIT_PAGE},            VP_MODE_NORMAL},
    {"pagedown",            command_scroll,      {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_DOWN | VP_SCROLL_UNIT_PAGE},           VP_MODE_NORMAL},
    {"halfpageup",          command_scroll,      {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_TOP | VP_SCROLL_UNIT_HALFPAGE},        VP_MODE_NORMAL},
    {"halfpagedown",        command_scroll,      {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_DOWN | VP_SCROLL_UNIT_HALFPAGE},       VP_MODE_NORMAL},
    {"scrollleft",          command_scroll,      {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_LEFT | VP_SCROLL_UNIT_LINE},           VP_MODE_NORMAL},
    {"scrollright",         command_scroll,      {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_RIGHT | VP_SCROLL_UNIT_LINE},          VP_MODE_NORMAL},
    {"scrollup",            command_scroll,      {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_TOP | VP_SCROLL_UNIT_LINE},            VP_MODE_NORMAL},
    {"scrolldown",          command_scroll,      {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_DOWN | VP_SCROLL_UNIT_LINE},           VP_MODE_NORMAL},
    {"nmap",                command_map,         {VP_MODE_NORMAL},                                                                   VP_MODE_NORMAL},
    {"imap",                command_map,         {VP_MODE_INSERT},                                                                   VP_MODE_NORMAL},
    {"cmap",                command_map,         {VP_MODE_COMMAND},                                                                  VP_MODE_NORMAL},
    {"hmap",                command_map,         {VP_MODE_HINTING},                                                                  VP_MODE_NORMAL},
    {"smap",                command_map,         {VP_MODE_SEARCH},                                                                   VP_MODE_NORMAL},
    {"nunmap",              command_unmap,       {VP_MODE_NORMAL},                                                                   VP_MODE_NORMAL},
    {"iunmap",              command_unmap,       {VP_MODE_INSERT},                                                                   VP_MODE_NORMAL},
    {"cunmap",              command_unmap,       {VP_MODE_COMMAND},                                                                  VP_MODE_NORMAL},
    {"hunmap",              command_unmap,       {VP_MODE_HINTING},                                                                  VP_MODE_NORMAL},
    {"sunmap",              command_map,         {VP_MODE_SEARCH},                                                                   VP_MODE_NORMAL},
    {"set",                 command_set,         {0},                                                                                VP_MODE_NORMAL},
    {"complete",            command_complete,    {0},                                                                                VP_MODE_COMMAND | VP_MODE_COMPLETE},
    {"complete-back",       command_complete,    {1},                                                                                VP_MODE_COMMAND | VP_MODE_COMPLETE},
    {"inspect",             command_inspect,     {0},                                                                                VP_MODE_NORMAL},
    {"hint-link",           command_hints,       {HINTS_TYPE_LINK, "."},                                                             VP_MODE_HINTING},
    {"hint-link-new",       command_hints,       {HINTS_TYPE_LINK | HINTS_TARGET_BLANK, ","},                                        VP_MODE_HINTING},
    {"hint-input-open",     command_hints,       {HINTS_TYPE_LINK | HINTS_PROCESS | HINTS_PROCESS_INPUT, ";o"},                      VP_MODE_HINTING},
    {"hint-input-tabopen",  command_hints,       {HINTS_TYPE_LINK | HINTS_TARGET_BLANK | HINTS_PROCESS | HINTS_PROCESS_INPUT, ";t"}, VP_MODE_HINTING},
    {"hint-yank",           command_hints,       {HINTS_TYPE_LINK | HINTS_PROCESS | HINTS_PROCESS_YANK, ";y"},                       VP_MODE_HINTING},
    {"hint-focus-next",     command_hints_focus, {0},                                                                                VP_MODE_HINTING},
    {"hint-focus-prev",     command_hints_focus, {1},                                                                                VP_MODE_HINTING},
    {"yank-uri",            command_yank,        {COMMAND_YANK_PRIMARY | COMMAND_YANK_SECONDARY | COMMAND_YANK_URI},                 VP_MODE_NORMAL},
    {"yank-selection",      command_yank,        {COMMAND_YANK_PRIMARY | COMMAND_YANK_SECONDARY | COMMAND_YANK_SELECTION},           VP_MODE_NORMAL},
    {"search-forward",      command_search,      {VP_SEARCH_FORWARD},                                                                VP_MODE_SEARCH},
    {"search-backward",     command_search,      {VP_SEARCH_BACKWARD},                                                               VP_MODE_SEARCH},
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

gboolean command_open_home(const Arg* arg)
{
    Arg a = {arg->i, vp.config.home_page};
    return vp_load_uri(&a);
}

/**
 * Reopens the last closed page.
 */
gboolean command_open_closed(const Arg* arg)
{
    gboolean result;

    Arg a = {arg->i};
    a.s = util_get_file_contents(vp.files[FILES_CLOSED], NULL);
    result = vp_load_uri(&a);
    g_free(a.s);

    return result;
}

gboolean command_input(const Arg* arg)
{
    const gchar* url;

    /* add current url if requested */
    if (VP_INPUT_CURRENT_URI == arg->i
        && (url = CURRENT_URL())
    ) {
        /* append the current url to the input message */
        gchar* input = g_strconcat(arg->s, url, NULL);
        command_write_input(input);
        g_free(input);
    } else {
        command_write_input(arg->s);
    }

    return TRUE;
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
        gint count = vp.state.count ? vp.state.count : 1;
        webkit_web_view_go_back_or_forward(
            vp.gui.webview, (arg->i == VP_NAVIG_BACK ? -count : count)
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
        if (vp.state.is_inspecting) {
            webkit_web_inspector_close(vp.gui.inspector);
        } else {
            webkit_web_inspector_show(vp.gui.inspector);
        }
        return TRUE;
    } else {
        vp_echo(VP_MSG_ERROR, TRUE, "enable-developer-extras not enabled");
        return FALSE;
    }
}

gboolean command_hints(const Arg* arg)
{
    command_write_input(arg->s);
    hints_create(NULL, arg->i, (arg->s ? strlen(arg->s) : 0));

    return TRUE;
}

gboolean command_hints_focus(const Arg* arg)
{
    hints_focus_next(arg->i ? TRUE : FALSE);

    return TRUE;
}

gboolean command_yank(const Arg* arg)
{
    if (arg->i & COMMAND_YANK_SELECTION) {
        gchar* text = NULL;
        /* copy current selection to clipboard */
        webkit_web_view_copy_clipboard(vp.gui.webview);
        text = gtk_clipboard_wait_for_text(PRIMARY_CLIPBOARD());
        if (!text) {
            text = gtk_clipboard_wait_for_text(SECONDARY_CLIPBOARD());
        }
        if (text) {
            /* TODO is this the rigth place to switch the focus */
            gtk_widget_grab_focus(GTK_WIDGET(vp.gui.webview));
            vp_echo(VP_MSG_NORMAL, FALSE, "Yanked: %s", text);
            g_free(text);

            return TRUE;
        }

        return FALSE;
    }
    /* use current arg.s a new clipboard content */
    Arg a = {arg->i};
    if (arg->i & COMMAND_YANK_URI) {
        /* yank current url */
        a.s = g_strdup(CURRENT_URL());
    } else {
        a.s = arg->s ? g_strdup(arg->s) : NULL;
    }
    if (a.s) {
        vp_set_clipboard(&a);
        /* TODO is this the rigth place to switch the focus */
        gtk_widget_grab_focus(GTK_WIDGET(vp.gui.webview));
        vp_echo(VP_MSG_NORMAL, FALSE, "Yanked: %s", a.s);
        g_free(a.s);

        return TRUE;
    }

    return FALSE;
}

gboolean command_search(const Arg* arg)
{
    State* state     = &vp.state;
    gboolean forward = !(arg->i ^ state->search_dir);

    if (arg->i == VP_SEARCH_OFF && state->search_query) {
        OVERWRITE_STRING(state->search_query, NULL);
#ifdef FEATURE_SEARCH_HIGHLIGHT
        webkit_web_view_unmark_text_matches(vp.gui.webview);
#endif

        return TRUE;
    }

    /* copy search query for later use */
    if (arg->s) {
        OVERWRITE_STRING(state->search_query, arg->s);
        /* set dearch dir only when the searching is started */
        vp.state.search_dir = arg->i;

        vp_set_mode(VP_MODE_SEARCH, FALSE);
    }

    if (state->search_query) {
#ifdef FEATURE_SEARCH_HIGHLIGHT
        webkit_web_view_mark_text_matches(vp.gui.webview, state->search_query, FALSE, 0);
        webkit_web_view_set_highlight_text_matches(vp.gui.webview, TRUE);
#endif
        /* make sure we have a count greater than 0 */
        vp.state.count = vp.state.count ? vp.state.count : 1;
        do {
            webkit_web_view_search_text(vp.gui.webview, state->search_query, FALSE, forward, TRUE);
        } while (--vp.state.count);
    }

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
