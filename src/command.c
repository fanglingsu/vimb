/**
 * vimp - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2013 Daniel Carl
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
#include "searchengine.h"
#include "history.h"

static CommandInfo cmd_list[] = {
    /* command              function             arg                                                                                 mode */
    {"open",                command_open,        {VP_TARGET_CURRENT, ""}},
    {"tabopen",             command_open,        {VP_TARGET_NEW, ""}},
    {"open-closed",         command_open_closed, {VP_TARGET_CURRENT}},
    {"tabopen-closed",      command_open_closed, {VP_TARGET_NEW}},
    {"input",               command_input,       {0, ":"}},
    {"inputuri",            command_input,       {VP_INPUT_CURRENT_URI, ":"}},
    {"quit",                command_close,       {0}},
    {"source",              command_view_source, {0}},
    {"back",                command_navigate,    {VP_NAVIG_BACK}},
    {"forward",             command_navigate,    {VP_NAVIG_FORWARD}},
    {"reload",              command_navigate,    {VP_NAVIG_RELOAD}},
    {"reload!",             command_navigate,    {VP_NAVIG_RELOAD_FORCE}},
    {"stop",                command_navigate,    {VP_NAVIG_STOP_LOADING}},
    {"jumpleft",            command_scroll,      {VP_SCROLL_TYPE_JUMP | VP_SCROLL_DIRECTION_LEFT}},
    {"jumpright",           command_scroll,      {VP_SCROLL_TYPE_JUMP | VP_SCROLL_DIRECTION_RIGHT}},
    {"jumptop",             command_scroll,      {VP_SCROLL_TYPE_JUMP | VP_SCROLL_DIRECTION_TOP}},
    {"jumpbottom",          command_scroll,      {VP_SCROLL_TYPE_JUMP | VP_SCROLL_DIRECTION_DOWN}},
    {"pageup",              command_scroll,      {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_TOP | VP_SCROLL_UNIT_PAGE}},
    {"pagedown",            command_scroll,      {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_DOWN | VP_SCROLL_UNIT_PAGE}},
    {"halfpageup",          command_scroll,      {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_TOP | VP_SCROLL_UNIT_HALFPAGE}},
    {"halfpagedown",        command_scroll,      {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_DOWN | VP_SCROLL_UNIT_HALFPAGE}},
    {"scrollleft",          command_scroll,      {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_LEFT | VP_SCROLL_UNIT_LINE}},
    {"scrollright",         command_scroll,      {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_RIGHT | VP_SCROLL_UNIT_LINE}},
    {"scrollup",            command_scroll,      {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_TOP | VP_SCROLL_UNIT_LINE}},
    {"scrolldown",          command_scroll,      {VP_SCROLL_TYPE_SCROLL | VP_SCROLL_DIRECTION_DOWN | VP_SCROLL_UNIT_LINE}},
    {"nmap",                command_map,         {VP_MODE_NORMAL}},
    {"imap",                command_map,         {VP_MODE_INSERT}},
    {"cmap",                command_map,         {VP_MODE_COMMAND}},
    {"hmap",                command_map,         {VP_MODE_HINTING}},
    {"smap",                command_map,         {VP_MODE_SEARCH}},
    {"nunmap",              command_unmap,       {VP_MODE_NORMAL}},
    {"iunmap",              command_unmap,       {VP_MODE_INSERT}},
    {"cunmap",              command_unmap,       {VP_MODE_COMMAND}},
    {"hunmap",              command_unmap,       {VP_MODE_HINTING}},
    {"sunmap",              command_map,         {VP_MODE_SEARCH}},
    {"set",                 command_set,         {0}},
    {"complete",            command_complete,    {0}},
    {"complete-back",       command_complete,    {1}},
    {"inspect",             command_inspect,     {0}},
    {"hint-link",           command_hints,       {HINTS_TYPE_LINK | HINTS_PROCESS_OPEN, "."}},
    {"hint-link-new",       command_hints,       {HINTS_TYPE_LINK | HINTS_PROCESS_OPEN | HINTS_OPEN_NEW, ","}},
    {"hint-input-open",     command_hints,       {HINTS_TYPE_LINK | HINTS_PROCESS_INPUT, ";o"}},
    {"hint-input-tabopen",  command_hints,       {HINTS_TYPE_LINK | HINTS_PROCESS_INPUT | HINTS_OPEN_NEW, ";t"}},
    {"hint-yank",           command_hints,       {HINTS_TYPE_LINK | HINTS_PROCESS_YANK, ";y"}},
    {"hint-image-open",     command_hints,       {HINTS_TYPE_IMAGE | HINTS_PROCESS_OPEN, ";i"}},
    {"hint-image-tabopen",  command_hints,       {HINTS_TYPE_IMAGE | HINTS_PROCESS_OPEN | HINTS_OPEN_NEW, ";I"}},
    {"hint-focus-next",     command_hints_focus, {0}},
    {"hint-focus-prev",     command_hints_focus, {1}},
    {"yank-uri",            command_yank,        {COMMAND_YANK_PRIMARY | COMMAND_YANK_SECONDARY | COMMAND_YANK_URI}},
    {"yank-selection",      command_yank,        {COMMAND_YANK_PRIMARY | COMMAND_YANK_SECONDARY | COMMAND_YANK_SELECTION}},
    {"open-clipboard",      command_paste,       {VP_CLIPBOARD_PRIMARY | VP_CLIPBOARD_SECONDARY | VP_TARGET_CURRENT}},
    {"tabopen-clipboard",   command_paste,       {VP_CLIPBOARD_PRIMARY | VP_CLIPBOARD_SECONDARY | VP_TARGET_NEW}},
    {"search-forward",      command_search,      {VP_SEARCH_FORWARD}},
    {"search-backward",     command_search,      {VP_SEARCH_BACKWARD}},
    {"searchengine-add",    command_searchengine,{1}},
    {"searchengine-remove", command_searchengine,{0}},
    {"zoomin",              command_zoom,        {COMMAND_ZOOM_IN}},
    {"zoomout",             command_zoom,        {COMMAND_ZOOM_OUT}},
    {"zoominfull",          command_zoom,        {COMMAND_ZOOM_IN | COMMAND_ZOOM_FULL}},
    {"zoomoutfull",         command_zoom,        {COMMAND_ZOOM_OUT | COMMAND_ZOOM_FULL}},
    {"zoomreset",           command_zoom,        {COMMAND_ZOOM_RESET}},
    {"command-hist-next",   command_history,     {VP_SEARCH_FORWARD}},
    {"command-hist-prev",   command_history,     {VP_SEARCH_BACKWARD}},
};

static void command_write_input(const char* str);


void command_init(void)
{
    guint i;
    core.behave.commands = g_hash_table_new(g_str_hash, g_str_equal);

    for (i = 0; i < LENGTH(cmd_list); i++) {
        g_hash_table_insert(core.behave.commands, (gpointer)cmd_list[i].name, &cmd_list[i]);
    }
}

void command_cleanup(void)
{
    if (core.behave.commands) {
        g_hash_table_destroy(core.behave.commands);
    }
}

gboolean command_exists(const char* name)
{
    return g_hash_table_contains(core.behave.commands, name);
}

gboolean command_run(const char* name, const char* param)
{
    CommandInfo* c = NULL;
    gboolean result;
    Arg a;
    c = g_hash_table_lookup(core.behave.commands, name);
    if (!c) {
        vp_echo(VP_MSG_ERROR, TRUE, "Command '%s' not found", name);
        vp_set_mode(VP_MODE_NORMAL, FALSE);

        return FALSE;
    }
    a.i = c->arg.i;
    a.s = g_strdup(param ? param : c->arg.s);
    result = c->function(&a);
    g_free(a.s);

    return result;
}

gboolean command_open(const Arg* arg)
{
    char* uri = NULL;
    gboolean result;

    if (!arg->s || arg->s[0] == '\0') {
        Arg a = {arg->i, core.config.home_page};
        return vp_load_uri(&a);
    }
    /* check for searchengine handles */
    /* split into handle and searchterms */
    char **string = g_strsplit(arg->s, " ", 2);
    if (g_strv_length(string) == 2
        && (uri = searchengine_get_uri(string[0]))
    ) {
        char* term = soup_uri_encode(string[1], "&");
        Arg a  = {arg->i, g_strdup_printf(uri, term)};
        result = vp_load_uri(&a);
        g_free(term);
        g_free(a.s);
    } else {
        result = vp_load_uri(arg);
    }
    g_strfreev(string);

    return result;
}

/**
 * Reopens the last closed page.
 */
gboolean command_open_closed(const Arg* arg)
{
    gboolean result;

    Arg a = {arg->i};
    a.s = util_get_file_contents(core.files[FILES_CLOSED], NULL);
    result = vp_load_uri(&a);
    g_free(a.s);

    return result;
}

gboolean command_input(const Arg* arg)
{
    const char* url;

    /* add current url if requested */
    if (VP_INPUT_CURRENT_URI == arg->i
        && (url = CURRENT_URL())
    ) {
        /* append the current url to the input message */
        char* input = g_strconcat(arg->s, url, NULL);
        command_write_input(input);
        g_free(input);
    } else {
        command_write_input(arg->s);
    }

    vp_set_mode(VP_MODE_COMMAND, FALSE);

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

    vp_set_mode(VP_MODE_NORMAL, FALSE);

    return TRUE;
}

gboolean command_navigate(const Arg* arg)
{
    if (arg->i <= VP_NAVIG_FORWARD) {
        int count = vp.state.count ? vp.state.count : 1;
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

    vp_set_mode(VP_MODE_NORMAL, FALSE);

    return TRUE;
}

gboolean command_scroll(const Arg* arg)
{
    GtkAdjustment *adjust = (arg->i & VP_SCROLL_AXIS_H) ? vp.gui.adjust_h : vp.gui.adjust_v;

    int direction  = (arg->i & (1 << 2)) ? 1 : -1;

    /* type scroll */
    if (arg->i & VP_SCROLL_TYPE_SCROLL) {
        gdouble value;
        int count = vp.state.count ? vp.state.count : 1;
        if (arg->i & VP_SCROLL_UNIT_LINE) {
            value = core.config.scrollstep;
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

    vp_set_mode(VP_MODE_NORMAL, FALSE);

    return TRUE;
}

gboolean command_map(const Arg* arg)
{
    gboolean result;
    vp_set_mode(VP_MODE_NORMAL, FALSE);

    char **string = g_strsplit(arg->s, "=", 2);
    if (g_strv_length(string) != 2) {
        return FALSE;
    }
    result = keybind_add_from_string(string[0], string[1], arg->i);
    g_strfreev(string);

    return result;
}

gboolean command_unmap(const Arg* arg)
{
    vp_set_mode(VP_MODE_NORMAL, FALSE);

    return keybind_remove_from_string(arg->s, arg->i);
}

gboolean command_set(const Arg* arg)
{
    gboolean success;
    char* line = NULL;
    char** token;

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

    vp_set_mode(VP_MODE_NORMAL, FALSE);

    return success;
}

gboolean command_complete(const Arg* arg)
{
    completion_complete(arg->i ? TRUE : FALSE);

    vp_set_mode(VP_MODE_COMMAND | VP_MODE_COMPLETE, FALSE);

    return TRUE;
}

gboolean command_inspect(const Arg* arg)
{
    gboolean enabled;
    WebKitWebSettings* settings = NULL;

    vp_set_mode(VP_MODE_NORMAL, FALSE);

    settings = webkit_web_view_get_settings(vp.gui.webview);
    g_object_get(G_OBJECT(settings), "enable-developer-extras", &enabled, NULL);
    if (enabled) {
        if (vp.state.is_inspecting) {
            webkit_web_inspector_close(vp.gui.inspector);
        } else {
            webkit_web_inspector_show(vp.gui.inspector);
        }
        return TRUE;
    }

    vp_echo(VP_MSG_ERROR, TRUE, "enable-developer-extras not enabled");

    return FALSE;
}

gboolean command_hints(const Arg* arg)
{
    command_write_input(arg->s);
    hints_create(NULL, arg->i, (arg->s ? strlen(arg->s) : 0));

    vp_set_mode(VP_MODE_HINTING, FALSE);

    return TRUE;
}

gboolean command_hints_focus(const Arg* arg)
{
    hints_focus_next(arg->i ? TRUE : FALSE);

    vp_set_mode(VP_MODE_HINTING, FALSE);

    return TRUE;
}

gboolean command_yank(const Arg* arg)
{
    vp_set_mode(VP_MODE_NORMAL, TRUE);

    if (arg->i & COMMAND_YANK_SELECTION) {
        char* text = NULL;
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

gboolean command_paste(const Arg* arg)
{
    Arg a = {.i = arg->i & VP_TARGET_NEW};
    if (arg->i & VP_CLIPBOARD_PRIMARY) {
        a.s = gtk_clipboard_wait_for_text(PRIMARY_CLIPBOARD());
    }
    if (!a.s && arg->i & VP_CLIPBOARD_SECONDARY) {
        a.s = gtk_clipboard_wait_for_text(SECONDARY_CLIPBOARD());
    }

    if (a.s) {
        vp_load_uri(&a);
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

    vp_set_mode(VP_MODE_SEARCH, FALSE);

    return TRUE;
}

gboolean command_searchengine(const Arg* arg)
{
    gboolean result;
    if (arg->i) {
        /* add the searchengine */
        char **string = g_strsplit(arg->s, "=", 2);
        if (g_strv_length(string) != 2) {
            return FALSE;
        }
        result = searchengine_add(string[0], string[1]);
        g_strfreev(string);
    } else {
        /* remove the search engine */
        result = searchengine_remove(arg->s);
    }

    vp_set_mode(VP_MODE_NORMAL, FALSE);

    return result;
}

gboolean command_zoom(const Arg* arg)
{
    float step, level;
    int count;

    if (arg->i & COMMAND_ZOOM_RESET) {
        webkit_web_view_set_zoom_level(vp.gui.webview, 1.0);
        vp_set_mode(VP_MODE_NORMAL, FALSE);

        return TRUE;
    }

    count = vp.state.count ? vp.state.count : 1;
    level = webkit_web_view_get_zoom_level(vp.gui.webview);

    WebKitWebSettings* setting = webkit_web_view_get_settings(vp.gui.webview);
    g_object_get(G_OBJECT(setting), "zoom-step", &step, NULL);

    webkit_web_view_set_full_content_zoom(
        vp.gui.webview, (arg->i & COMMAND_ZOOM_FULL) > 0
    );

    webkit_web_view_set_zoom_level(
        vp.gui.webview,
        level + (float)(count *step) * (arg->i & COMMAND_ZOOM_IN ? 1.0 : -1.0)
    );

    vp_set_mode(VP_MODE_NORMAL, FALSE);

    return TRUE;

}

gboolean command_history(const Arg* arg)
{
    const int count = vp.state.count ? vp.state.count : 1;
    const gint step = count * (arg->i == VP_SEARCH_BACKWARD ? -1 : 1);
    const char* entry = history_get(step);

    if (!entry) {
        return FALSE;
    }
    command_write_input(entry);

    return TRUE;
}

static void command_write_input(const char* str)
{
    int pos = 0;
    /* reset the colors and fonts to defalts */
    vp_set_widget_font(
        vp.gui.inputbox,
        &core.style.input_fg[VP_MSG_NORMAL],
        &core.style.input_bg[VP_MSG_NORMAL],
        core.style.input_font[VP_MSG_NORMAL]
    );

    /* remove content from input box */
    gtk_editable_delete_text(GTK_EDITABLE(vp.gui.inputbox), 0, -1);

    /* insert string from arg */
    gtk_editable_insert_text(GTK_EDITABLE(vp.gui.inputbox), str, -1, &pos);
    gtk_editable_set_position(GTK_EDITABLE(vp.gui.inputbox), -1);
}
