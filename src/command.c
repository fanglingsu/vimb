/**
 * vimb - a webkit based vim like browser.
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

extern VbCore vb;
extern const unsigned int INPUT_LENGTH;

static CommandInfo cmd_list[] = {
    /* command              function             arg                                                                                 mode */
    {"open",                command_open,        {VB_TARGET_CURRENT, ""}},
    {"tabopen",             command_open,        {VB_TARGET_NEW, ""}},
    {"open-closed",         command_open_closed, {VB_TARGET_CURRENT}},
    {"tabopen-closed",      command_open_closed, {VB_TARGET_NEW}},
    {"input",               command_input,       {0, ":"}},
    {"inputuri",            command_input,       {VB_INPUT_CURRENT_URI, ":"}},
    {"quit",                command_close,       {0}},
    {"source",              command_view_source, {0}},
    {"back",                command_navigate,    {VB_NAVIG_BACK}},
    {"forward",             command_navigate,    {VB_NAVIG_FORWARD}},
    {"reload",              command_navigate,    {VB_NAVIG_RELOAD}},
    {"reload!",             command_navigate,    {VB_NAVIG_RELOAD_FORCE}},
    {"stop",                command_navigate,    {VB_NAVIG_STOP_LOADING}},
    {"jumpleft",            command_scroll,      {VB_SCROLL_TYPE_JUMP | VB_SCROLL_DIRECTION_LEFT}},
    {"jumpright",           command_scroll,      {VB_SCROLL_TYPE_JUMP | VB_SCROLL_DIRECTION_RIGHT}},
    {"jumptop",             command_scroll,      {VB_SCROLL_TYPE_JUMP | VB_SCROLL_DIRECTION_TOP}},
    {"jumpbottom",          command_scroll,      {VB_SCROLL_TYPE_JUMP | VB_SCROLL_DIRECTION_DOWN}},
    {"pageup",              command_scroll,      {VB_SCROLL_TYPE_SCROLL | VB_SCROLL_DIRECTION_TOP | VB_SCROLL_UNIT_PAGE}},
    {"pagedown",            command_scroll,      {VB_SCROLL_TYPE_SCROLL | VB_SCROLL_DIRECTION_DOWN | VB_SCROLL_UNIT_PAGE}},
    {"halfpageup",          command_scroll,      {VB_SCROLL_TYPE_SCROLL | VB_SCROLL_DIRECTION_TOP | VB_SCROLL_UNIT_HALFPAGE}},
    {"halfpagedown",        command_scroll,      {VB_SCROLL_TYPE_SCROLL | VB_SCROLL_DIRECTION_DOWN | VB_SCROLL_UNIT_HALFPAGE}},
    {"scrollleft",          command_scroll,      {VB_SCROLL_TYPE_SCROLL | VB_SCROLL_DIRECTION_LEFT | VB_SCROLL_UNIT_LINE}},
    {"scrollright",         command_scroll,      {VB_SCROLL_TYPE_SCROLL | VB_SCROLL_DIRECTION_RIGHT | VB_SCROLL_UNIT_LINE}},
    {"scrollup",            command_scroll,      {VB_SCROLL_TYPE_SCROLL | VB_SCROLL_DIRECTION_TOP | VB_SCROLL_UNIT_LINE}},
    {"scrolldown",          command_scroll,      {VB_SCROLL_TYPE_SCROLL | VB_SCROLL_DIRECTION_DOWN | VB_SCROLL_UNIT_LINE}},
    {"nmap",                command_map,         {VB_MODE_NORMAL}},
    {"imap",                command_map,         {VB_MODE_INSERT}},
    {"cmap",                command_map,         {VB_MODE_COMMAND}},
    {"hmap",                command_map,         {VB_MODE_HINTING}},
    {"smap",                command_map,         {VB_MODE_SEARCH}},
    {"nunmap",              command_unmap,       {VB_MODE_NORMAL}},
    {"iunmap",              command_unmap,       {VB_MODE_INSERT}},
    {"cunmap",              command_unmap,       {VB_MODE_COMMAND}},
    {"hunmap",              command_unmap,       {VB_MODE_HINTING}},
    {"sunmap",              command_map,         {VB_MODE_SEARCH}},
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
    {"open-clipboard",      command_paste,       {VB_CLIPBOARD_PRIMARY | VB_CLIPBOARD_SECONDARY | VB_TARGET_CURRENT}},
    {"tabopen-clipboard",   command_paste,       {VB_CLIPBOARD_PRIMARY | VB_CLIPBOARD_SECONDARY | VB_TARGET_NEW}},
    {"search-forward",      command_search,      {VB_SEARCH_FORWARD}},
    {"search-backward",     command_search,      {VB_SEARCH_BACKWARD}},
    {"searchengine-add",    command_searchengine,{1}},
    {"searchengine-remove", command_searchengine,{0}},
    {"zoomin",              command_zoom,        {COMMAND_ZOOM_IN}},
    {"zoomout",             command_zoom,        {COMMAND_ZOOM_OUT}},
    {"zoominfull",          command_zoom,        {COMMAND_ZOOM_IN | COMMAND_ZOOM_FULL}},
    {"zoomoutfull",         command_zoom,        {COMMAND_ZOOM_OUT | COMMAND_ZOOM_FULL}},
    {"zoomreset",           command_zoom,        {COMMAND_ZOOM_RESET}},
    {"command-hist-next",   command_history,     {VB_SEARCH_FORWARD}},
    {"command-hist-prev",   command_history,     {VB_SEARCH_BACKWARD}},
};

static void command_write_input(const char* str);


void command_init(void)
{
    guint i;
    vb.behave.commands = g_hash_table_new(g_str_hash, g_str_equal);

    for (i = 0; i < LENGTH(cmd_list); i++) {
        g_hash_table_insert(vb.behave.commands, (gpointer)cmd_list[i].name, &cmd_list[i]);
    }
}

void command_cleanup(void)
{
    if (vb.behave.commands) {
        g_hash_table_destroy(vb.behave.commands);
    }
}

gboolean command_exists(const char* name)
{
    return g_hash_table_contains(vb.behave.commands, name);
}

gboolean command_run(const char* name, const char* param)
{
    CommandInfo* command = NULL;
    gboolean result;
    Arg a;
    command = g_hash_table_lookup(vb.behave.commands, name);
    if (!command) {
        vb_echo(VB_MSG_ERROR, TRUE, "Command '%s' not found", name);
        vb_set_mode(VB_MODE_NORMAL, FALSE);

        return FALSE;
    }
    a.i = command->arg.i;
    a.s = g_strdup(param ? param : command->arg.s);
    result = command->function(&a);
    g_free(a.s);

    return result;
}

gboolean command_open(const Arg* arg)
{
    char* uri = NULL;
    gboolean result;

    if (!arg->s || arg->s[0] == '\0') {
        Arg a = {arg->i, vb.config.home_page};
        return vb_load_uri(&a);
    }
    /* check for searchengine handles */
    /* split into handle and searchterms */
    char **string = g_strsplit(arg->s, " ", 2);
    if (g_strv_length(string) == 2
        && (uri = searchengine_get_uri(string[0]))
    ) {
        char* term = soup_uri_encode(string[1], "&");
        Arg a  = {arg->i, g_strdup_printf(uri, term)};
        result = vb_load_uri(&a);
        g_free(term);
        g_free(a.s);
    } else {
        result = vb_load_uri(arg);
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
    a.s = util_get_file_contents(vb.files[FILES_CLOSED], NULL);
    result = vb_load_uri(&a);
    g_free(a.s);

    return result;
}

gboolean command_input(const Arg* arg)
{
    const char* url;

    /* add current url if requested */
    if (VB_INPUT_CURRENT_URI == arg->i
        && (url = webkit_web_view_get_uri(vb.gui.webview))
    ) {
        /* append the current url to the input message */
        char* input = g_strconcat(arg->s, url, NULL);
        command_write_input(input);
        g_free(input);
    } else {
        command_write_input(arg->s);
    }

    vb_set_mode(VB_MODE_COMMAND, FALSE);

    return TRUE;
}

gboolean command_close(const Arg* arg)
{
    gtk_widget_destroy(GTK_WIDGET(vb.gui.window));

    return TRUE;
}

gboolean command_view_source(const Arg* arg)
{
    gboolean mode = webkit_web_view_get_view_source_mode(vb.gui.webview);
    webkit_web_view_set_view_source_mode(vb.gui.webview, !mode);
    webkit_web_view_reload(vb.gui.webview);

    vb_set_mode(VB_MODE_NORMAL, FALSE);

    return TRUE;
}

gboolean command_navigate(const Arg* arg)
{
    WebKitWebView* view = vb.gui.webview;
    if (arg->i <= VB_NAVIG_FORWARD) {
        int count = vb.state.count ? vb.state.count : 1;
        webkit_web_view_go_back_or_forward(
            view, (arg->i == VB_NAVIG_BACK ? -count : count)
        );
    } else if (arg->i == VB_NAVIG_RELOAD) {
        webkit_web_view_reload(view);
    } else if (arg->i == VB_NAVIG_RELOAD_FORCE) {
        webkit_web_view_reload_bypass_cache(view);
    } else {
        webkit_web_view_stop_loading(view);
    }

    vb_set_mode(VB_MODE_NORMAL, FALSE);

    return TRUE;
}

gboolean command_scroll(const Arg* arg)
{
    GtkAdjustment *adjust = (arg->i & VB_SCROLL_AXIS_H) ? vb.gui.adjust_h : vb.gui.adjust_v;

    int direction  = (arg->i & (1 << 2)) ? 1 : -1;

    /* type scroll */
    if (arg->i & VB_SCROLL_TYPE_SCROLL) {
        gdouble value;
        int count = vb.state.count ? vb.state.count : 1;
        if (arg->i & VB_SCROLL_UNIT_LINE) {
            value = vb.config.scrollstep;
        } else if (arg->i & VB_SCROLL_UNIT_HALFPAGE) {
            value = gtk_adjustment_get_page_size(adjust) / 2;
        } else {
            value = gtk_adjustment_get_page_size(adjust);
        }
        gtk_adjustment_set_value(adjust, gtk_adjustment_get_value(adjust) + direction * value * count);
    } else if (vb.state.count) {
        /* jump - if count is set to count% of page */
        gdouble max = gtk_adjustment_get_upper(adjust) - gtk_adjustment_get_page_size(adjust);
        gtk_adjustment_set_value(adjust, max * vb.state.count / 100);
    } else if (direction == 1) {
        /* jump to top */
        gtk_adjustment_set_value(adjust, gtk_adjustment_get_upper(adjust));
    } else {
        /* jump to bottom */
        gtk_adjustment_set_value(adjust, gtk_adjustment_get_lower(adjust));
    }

    vb_set_mode(VB_MODE_NORMAL, FALSE);

    return TRUE;
}

gboolean command_map(const Arg* arg)
{
    gboolean result;
    vb_set_mode(VB_MODE_NORMAL, FALSE);

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
    vb_set_mode(VB_MODE_NORMAL, FALSE);

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

    vb_set_mode(VB_MODE_NORMAL, FALSE);

    return success;
}

gboolean command_complete(const Arg* arg)
{
    completion_complete(arg->i ? TRUE : FALSE);

    vb_set_mode(VB_MODE_COMMAND | VB_MODE_COMPLETE, FALSE);

    return TRUE;
}

gboolean command_inspect(const Arg* arg)
{
    gboolean enabled;
    WebKitWebSettings* settings = NULL;

    vb_set_mode(VB_MODE_NORMAL, FALSE);

    settings = webkit_web_view_get_settings(vb.gui.webview);
    g_object_get(G_OBJECT(settings), "enable-developer-extras", &enabled, NULL);
    if (enabled) {
        if (vb.state.is_inspecting) {
            webkit_web_inspector_close(vb.gui.inspector);
        } else {
            webkit_web_inspector_show(vb.gui.inspector);
        }
        return TRUE;
    }

    vb_echo(VB_MSG_ERROR, TRUE, "webinspector is not enabled");

    return FALSE;
}

gboolean command_hints(const Arg* arg)
{
    command_write_input(arg->s);
    /* mode will be set in hints_create - so we don't neet to do it here */
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
    vb_set_mode(VB_MODE_NORMAL, TRUE);

    if (arg->i & COMMAND_YANK_SELECTION) {
        char* text = NULL;
        /* copy current selection to clipboard */
        webkit_web_view_copy_clipboard(vb.gui.webview);
        text = gtk_clipboard_wait_for_text(PRIMARY_CLIPBOARD());
        if (!text) {
            text = gtk_clipboard_wait_for_text(SECONDARY_CLIPBOARD());
        }
        if (text) {
            vb_echo_force(VB_MSG_NORMAL, "Yanked: %s", text);
            g_free(text);

            return TRUE;
        }

        return FALSE;
    }
    /* use current arg.s a new clipboard content */
    Arg a = {arg->i};
    if (arg->i & COMMAND_YANK_URI) {
        /* yank current url */
        a.s = g_strdup(webkit_web_view_get_uri(vb.gui.webview));
    } else {
        a.s = arg->s ? g_strdup(arg->s) : NULL;
    }
    if (a.s) {
        vb_set_clipboard(&a);
        vb_echo_force(VB_MSG_NORMAL, "Yanked: %s", a.s);
        g_free(a.s);

        return TRUE;
    }

    return FALSE;
}

gboolean command_paste(const Arg* arg)
{
    Arg a = {.i = arg->i & VB_TARGET_NEW};
    if (arg->i & VB_CLIPBOARD_PRIMARY) {
        a.s = gtk_clipboard_wait_for_text(PRIMARY_CLIPBOARD());
    }
    if (!a.s && arg->i & VB_CLIPBOARD_SECONDARY) {
        a.s = gtk_clipboard_wait_for_text(SECONDARY_CLIPBOARD());
    }

    if (a.s) {
        vb_load_uri(&a);
        g_free(a.s);

        return TRUE;
    }
    return FALSE;
}

gboolean command_search(const Arg* arg)
{
    gboolean forward = !(arg->i ^ vb.state.search_dir);

    if (arg->i == VB_SEARCH_OFF && vb.state.search_query) {
        OVERWRITE_STRING(vb.state.search_query, NULL);
#ifdef FEATURE_SEARCH_HIGHLIGHT
        webkit_web_view_unmark_text_matches(vb.gui.webview);
#endif

        return TRUE;
    }

    /* copy search query for later use */
    if (arg->s) {
        OVERWRITE_STRING(vb.state.search_query, arg->s);
        /* set dearch dir only when the searching is started */
        vb.state.search_dir = arg->i;
    }

    if (vb.state.search_query) {
#ifdef FEATURE_SEARCH_HIGHLIGHT
        webkit_web_view_mark_text_matches(vb.gui.webview, vb.state.search_query, FALSE, 0);
        webkit_web_view_set_highlight_text_matches(vb.gui.webview, TRUE);
#endif
        /* make sure we have a count greater than 0 */
        vb.state.count = vb.state.count ? vb.state.count : 1;
        do {
            webkit_web_view_search_text(vb.gui.webview, vb.state.search_query, FALSE, forward, TRUE);
        } while (--vb.state.count);
    }

    vb_set_mode(VB_MODE_SEARCH, FALSE);

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

    vb_set_mode(VB_MODE_NORMAL, FALSE);

    return result;
}

gboolean command_zoom(const Arg* arg)
{
    float step, level;
    int count;

    if (arg->i & COMMAND_ZOOM_RESET) {
        webkit_web_view_set_zoom_level(vb.gui.webview, 1.0);
        vb_set_mode(VB_MODE_NORMAL, FALSE);

        return TRUE;
    }

    count = vb.state.count ? vb.state.count : 1;
    level = webkit_web_view_get_zoom_level(vb.gui.webview);

    WebKitWebSettings* setting = webkit_web_view_get_settings(vb.gui.webview);
    g_object_get(G_OBJECT(setting), "zoom-step", &step, NULL);

    webkit_web_view_set_full_content_zoom(
        vb.gui.webview, (arg->i & COMMAND_ZOOM_FULL) > 0
    );

    webkit_web_view_set_zoom_level(
        vb.gui.webview,
        level + (float)(count *step) * (arg->i & COMMAND_ZOOM_IN ? 1.0 : -1.0)
    );

    vb_set_mode(VB_MODE_NORMAL, FALSE);

    return TRUE;

}

gboolean command_history(const Arg* arg)
{
    const char* input = GET_TEXT();
    int step          = vb.state.count ? vb.state.count * arg->i : arg->i;
    const char* entry;
    char* prefix = NULL;

    /* use the right history type according to current input text */
    if (!strncmp(input, ":open ", 6)) {
        entry  = history_get(HISTORY_URL, step, input + 6);
        prefix = ":open ";
    } else if (!strncmp(input, ":tabopen ", 9)) {
        entry  = history_get(HISTORY_URL, step, input + 9);
        prefix = ":tabopen ";
    } else if (*input == ':') {
        entry  = history_get(HISTORY_COMMAND, step, input + 1);
        prefix = ":";
    } else if (*input == '/' || *input == '?') {
        entry  = history_get(HISTORY_SEARCH, step, input + 1);
        prefix = *input == '/' ? "/" : "?";
    }

    if (!entry) {
        return FALSE;
    }

    char* value = g_strconcat(prefix, entry, NULL);
    command_write_input(value);
    g_free(value);

    return TRUE;
}

static void command_write_input(const char* str)
{
    int pos = 0;
    GtkEditable* box = GTK_EDITABLE(vb.gui.inputbox);
    /* reset the colors and fonts to defalts */
    vb_set_widget_font(
        vb.gui.inputbox,
        &vb.style.input_fg[VB_MSG_NORMAL],
        &vb.style.input_bg[VB_MSG_NORMAL],
        vb.style.input_font[VB_MSG_NORMAL]
    );

    /* remove content from input box */
    gtk_editable_delete_text(box, 0, -1);

    /* insert string from arg */
    gtk_editable_insert_text(box, str, -1, &pos);
    gtk_editable_set_position(box, strlen(str) > INPUT_LENGTH ? 0 : -1);
}
