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
#include "shortcut.h"
#include "history.h"
#include "bookmark.h"
#include "dom.h"

#define SHELL_CMD "/bin/sh -c '%s'"

typedef struct {
    char    *file;
    Element *element;
} OpenEditorData;

typedef struct {
    const char *name;
    const char *alias;
    Command    function;
    const Arg  arg;       /* arguments to call the command with */
} CommandInfo;

extern VbCore vb;
extern const unsigned int INPUT_LENGTH;

static GHashTable *commands;
static GHashTable *short_commands;

static CommandInfo cmd_list[] = {
    /* command                    alias    function                      arg */
    {"open",                      "o",     command_open,                 {VB_TARGET_CURRENT, ""}},
    {"tabopen",                   "t",     command_open,                 {VB_TARGET_NEW, ""}},
    {"open-closed",               NULL,    command_open_closed,          {VB_TARGET_CURRENT}},
    {"tabopen-closed",            NULL,    command_open_closed,          {VB_TARGET_NEW}},
    {"open-clipboard",            "oc",    command_paste,                {VB_CLIPBOARD_PRIMARY | VB_CLIPBOARD_SECONDARY | VB_TARGET_CURRENT}},
    {"tabopen-clipboard",         "toc",   command_paste,                {VB_CLIPBOARD_PRIMARY | VB_CLIPBOARD_SECONDARY | VB_TARGET_NEW}},
    {"input",                     "in",    command_input,                {0, ":"}},
    {"inputuri",                  NULL,    command_input,                {VB_INPUT_CURRENT_URI, ":"}},
    {"quit",                      "q",     command_close,                {0}},
    {"source",                    NULL,    command_view_source,          {0}},
    {"back",                      "ba",    command_navigate,             {VB_NAVIG_BACK}},
    {"forward",                   "fo",    command_navigate,             {VB_NAVIG_FORWARD}},
    {"reload",                    "re",    command_navigate,             {VB_NAVIG_RELOAD}},
    {"reload!",                   "re!",   command_navigate,             {VB_NAVIG_RELOAD_FORCE}},
    {"stop",                      "st",    command_navigate,             {VB_NAVIG_STOP_LOADING}},
    {"jumpleft",                  NULL,    command_scroll,               {VB_SCROLL_TYPE_JUMP | VB_SCROLL_DIRECTION_LEFT}},
    {"jumpright",                 NULL,    command_scroll,               {VB_SCROLL_TYPE_JUMP | VB_SCROLL_DIRECTION_RIGHT}},
    {"jumptop",                   NULL,    command_scroll,               {VB_SCROLL_TYPE_JUMP | VB_SCROLL_DIRECTION_TOP}},
    {"jumpbottom",                NULL,    command_scroll,               {VB_SCROLL_TYPE_JUMP | VB_SCROLL_DIRECTION_DOWN}},
    {"pageup",                    NULL,    command_scroll,               {VB_SCROLL_TYPE_SCROLL | VB_SCROLL_DIRECTION_TOP | VB_SCROLL_UNIT_PAGE}},
    {"pagedown",                  NULL,    command_scroll,               {VB_SCROLL_TYPE_SCROLL | VB_SCROLL_DIRECTION_DOWN | VB_SCROLL_UNIT_PAGE}},
    {"halfpageup",                NULL,    command_scroll,               {VB_SCROLL_TYPE_SCROLL | VB_SCROLL_DIRECTION_TOP | VB_SCROLL_UNIT_HALFPAGE}},
    {"halfpagedown",              NULL,    command_scroll,               {VB_SCROLL_TYPE_SCROLL | VB_SCROLL_DIRECTION_DOWN | VB_SCROLL_UNIT_HALFPAGE}},
    {"scrollleft",                NULL,    command_scroll,               {VB_SCROLL_TYPE_SCROLL | VB_SCROLL_DIRECTION_LEFT | VB_SCROLL_UNIT_LINE}},
    {"scrollright",               NULL,    command_scroll,               {VB_SCROLL_TYPE_SCROLL | VB_SCROLL_DIRECTION_RIGHT | VB_SCROLL_UNIT_LINE}},
    {"scrollup",                  NULL,    command_scroll,               {VB_SCROLL_TYPE_SCROLL | VB_SCROLL_DIRECTION_TOP | VB_SCROLL_UNIT_LINE}},
    {"scrolldown",                NULL,    command_scroll,               {VB_SCROLL_TYPE_SCROLL | VB_SCROLL_DIRECTION_DOWN | VB_SCROLL_UNIT_LINE}},
    {"nmap",                      NULL,    command_map,                  {VB_MODE_NORMAL}},
    {"imap",                      NULL,    command_map,                  {VB_MODE_INSERT}},
    {"cmap",                      NULL,    command_map,                  {VB_MODE_COMMAND}},
    {"nunmap",                    NULL,    command_unmap,                {VB_MODE_NORMAL}},
    {"iunmap",                    NULL,    command_unmap,                {VB_MODE_INSERT}},
    {"cunmap",                    NULL,    command_unmap,                {VB_MODE_COMMAND}},
    {"set",                       NULL,    command_set,                  {0}},
    {"inspect",                   NULL,    command_inspect,              {0}},
    {"hint-link",                 NULL,    command_hints,                {HINTS_TYPE_LINK | HINTS_PROCESS_OPEN}},
    {"hint-link-new",             NULL,    command_hints,                {HINTS_TYPE_LINK | HINTS_PROCESS_OPEN | HINTS_OPEN_NEW}},
    {"hint-input-open",           NULL,    command_hints,                {HINTS_TYPE_LINK | HINTS_PROCESS_INPUT}},
    {"hint-input-tabopen",        NULL,    command_hints,                {HINTS_TYPE_LINK | HINTS_PROCESS_INPUT | HINTS_OPEN_NEW}},
    {"hint-yank",                 NULL,    command_hints,                {HINTS_TYPE_LINK | HINTS_PROCESS_YANK}},
    {"hint-image-open",           NULL,    command_hints,                {HINTS_TYPE_IMAGE | HINTS_PROCESS_OPEN}},
    {"hint-image-tabopen",        NULL,    command_hints,                {HINTS_TYPE_IMAGE | HINTS_PROCESS_OPEN | HINTS_OPEN_NEW}},
    {"hint-editor",               NULL,    command_hints,                {HINTS_TYPE_EDITABLE}},
    {"hint-save",                 NULL,    command_hints,                {HINTS_TYPE_LINK | HINTS_PROCESS_SAVE}},
    {"yank-uri",                  "yu",    command_yank,                 {VB_CLIPBOARD_PRIMARY | VB_CLIPBOARD_SECONDARY | COMMAND_YANK_URI}},
    {"yank-selection",            "ys",    command_yank,                 {VB_CLIPBOARD_PRIMARY | VB_CLIPBOARD_SECONDARY | COMMAND_YANK_SELECTION}},
    {"search-forward",            NULL,    command_search,               {VB_SEARCH_FORWARD}},
    {"search-backward",           NULL,    command_search,               {VB_SEARCH_BACKWARD}},
    {"search-selection-forward",  NULL,    command_selsearch,            {VB_SEARCH_FORWARD}},
    {"search-selection-backward", NULL,    command_selsearch,            {VB_SEARCH_BACKWARD}},
    {"shortcut-add",              NULL,    command_shortcut,             {1}},
    {"shortcut-remove",           NULL,    command_shortcut,             {0}},
    {"shortcut-default",          NULL,    command_shortcut_default,     {0}},
    {"zoomin",                    "zi",    command_zoom,                 {COMMAND_ZOOM_IN}},
    {"zoomout",                   "zo",    command_zoom,                 {COMMAND_ZOOM_OUT}},
    {"zoominfull",                "zif",   command_zoom,                 {COMMAND_ZOOM_IN | COMMAND_ZOOM_FULL}},
    {"zoomoutfull",               "zof",   command_zoom,                 {COMMAND_ZOOM_OUT | COMMAND_ZOOM_FULL}},
    {"zoomreset",                 "zr",    command_zoom,                 {COMMAND_ZOOM_RESET}},
    {"hist-next",                 NULL,    command_history,              {0}},
    {"hist-prev",                 NULL,    command_history,              {1}},
    {"run",                       NULL,    command_run_multi,            {0}},
    {"bookmark-add",              "bma",   command_bookmark,             {1}},
    {"bookmark-remove",           "bmr",   command_bookmark,             {0}},
    {"eval",                      "e",     command_eval,                 {0}},
    {"editor",                    NULL,    command_editor,               {0}},
    {"next",                      "n",     command_nextprev,             {0}},
    {"prev",                      "p",     command_nextprev,             {1}},
    {"descent",                   NULL,    command_descent,              {0}},
    {"descent!",                  NULL,    command_descent,              {1}},
    {"save",                      NULL,    command_save,                 {COMMAND_SAVE_CURRENT}},
    {"shellcmd",                  NULL,    command_shellcmd,             {0}},
};

static void editor_resume(GPid pid, int status, OpenEditorData *data);
static CommandInfo *command_lookup(const char* name);
static char *expand_string(const char *str);


void command_init(void)
{
    guint i;
    commands       = g_hash_table_new(g_str_hash, g_str_equal);
    short_commands = g_hash_table_new(g_str_hash, g_str_equal);

    for (i = 0; i < LENGTH(cmd_list); i++) {
        g_hash_table_insert(commands, (gpointer)cmd_list[i].name, &cmd_list[i]);
        /* save the commands by their alias in extra hash table */
        if (cmd_list[i].alias) {
            g_hash_table_insert(short_commands, (gpointer)cmd_list[i].alias, &cmd_list[i]);
        }
    }
}

void command_cleanup(void)
{
    if (commands) {
        g_hash_table_destroy(commands);
    }
    if (short_commands) {
        g_hash_table_destroy(short_commands);
    }
}

/**
 * Parses given string and put corresponding command arg and command count in
 * also given pointers.
 * Returns true if parsing was successful.
 * Dont forget to g_free arg->s.
 */
gboolean command_parse_from_string(const char *input, Command *func, Arg *arg, guint *count)
{
    char *command, *name, *str, **token;
    CommandInfo *info;

    if (!input || *input == '\0') {
        return false;
    }

    str = g_strdup(input);
    /* remove leading whitespace */
    g_strchug(str);

    /* get a possible command count */
    *count = g_ascii_strtoll(str, &command, 10);

    /* split the input string into command and parameter part */
    token = g_strsplit(command, " ", 2);
    g_free(str);

    if (!token[0]) {
        g_strfreev(token);
        return false;
    }

    name = token[0];
    info = command_lookup(name);
    if (!info) {
        vb_echo(VB_MSG_ERROR, true, "Command '%s' not found", name);
        g_strfreev(token);
        return false;
    }

    /* assigne the data to given pointers */
    arg->i = info->arg.i;
    arg->s = g_strdup(token[1] ? token[1] : info->arg.s);
    *func = info->function;

    g_strfreev(token);

    return true;
}

/**
 * Runs a single command form string containing the command and possible
 * parameters.
 */
gboolean command_run_string(const char *input)
{
    gboolean success;
    Command command = NULL;
    Arg arg = {0};
    if (!command_parse_from_string(input, &command, &arg, &vb.state.count)) {
        vb_set_mode(VB_MODE_NORMAL, false);
        return false;
    }

    success = command(&arg);
    g_free(arg.s);

    return success;
}

/**
 * Runs multiple commands that are seperated by |.
 */
gboolean command_run_multi(const Arg *arg)
{
    gboolean result = true;
    char **commands;
    unsigned int len, i;

    vb_set_mode(VB_MODE_NORMAL, false);
    if (!arg->s || *(arg->s) == '\0') {
        return false;
    }

    /* splits the commands */
    commands = g_strsplit(arg->s, "|", 0);

    len = g_strv_length(commands);
    if (!len) {
        g_strfreev(commands);
        return false;
    }

    for (i = 0; i < len; i++) {
        /* run the single commands */
        result = (result && command_run_string(commands[i]));
    }
    g_strfreev(commands);

    return result;
}

gboolean command_fill_completion(GtkListStore *store, const char *input)
{
    gboolean found = false;
    GtkTreeIter iter;
    /* according to vim we return only the long commands here */
    GList *src = g_hash_table_get_keys(commands);

    if (!input || input == '\0') {
        for (GList *l = src; l; l = l->next) {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, COMPLETION_STORE_FIRST, l->data, -1);
            found = true;
        }
    } else {
        for (GList *l = src; l; l = l->next) {
            char *value = (char*)l->data;
            if (g_str_has_prefix(value, input)) {
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter, COMPLETION_STORE_FIRST, l->data, -1);
                found = true;
            }
        }
    }
    g_list_free(src);

    return found;
}

gboolean command_open(const Arg *arg)
{
    vb_set_mode(VB_MODE_NORMAL, false);
    return vb_load_uri(arg);
}

/**
 * Reopens the last closed page.
 */
gboolean command_open_closed(const Arg *arg)
{
    gboolean result;

    Arg a = {arg->i};
    a.s = util_get_file_contents(vb.files[FILES_CLOSED], NULL);
    result = vb_load_uri(&a);
    g_free(a.s);

    return result;
}

gboolean command_input(const Arg *arg)
{
    const char *url;

    /* add current url if requested */
    if (VB_INPUT_CURRENT_URI == arg->i && (url = GET_URI())) {
        /* append the current url to the input message */
        char *input = g_strconcat(arg->s, url, NULL);
        vb_echo_force(VB_MSG_NORMAL, false, "%s", input);
        g_free(input);
    } else {
        vb_echo_force(VB_MSG_NORMAL, false, "%s", arg->s);
    }

    vb_set_mode(VB_MODE_COMMAND, false);

    return true;
}

gboolean command_close(const Arg *arg)
{
    vb_quit();
    return true;
}

gboolean command_view_source(const Arg *arg)
{
    vb_set_mode(VB_MODE_NORMAL, false);

    gboolean mode = webkit_web_view_get_view_source_mode(vb.gui.webview);
    webkit_web_view_set_view_source_mode(vb.gui.webview, !mode);
    webkit_web_view_reload(vb.gui.webview);

    return true;
}

gboolean command_navigate(const Arg *arg)
{
    int count = vb.state.count ? vb.state.count : 1;
    vb_set_mode(VB_MODE_NORMAL, false);

    WebKitWebView *view = vb.gui.webview;
    if (arg->i <= VB_NAVIG_FORWARD) {
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

    return true;
}

gboolean command_scroll(const Arg *arg)
{
    gdouble max, new;
    int count = vb.state.count ? vb.state.count : 1;
    int direction = (arg->i & (1 << 2)) ? 1 : -1;
    GtkAdjustment *adjust = (arg->i & VB_SCROLL_AXIS_H) ? vb.gui.adjust_h : vb.gui.adjust_v;

    /* keep possible search mode */
    vb_set_mode(VB_MODE_NORMAL | (vb.state.mode & VB_MODE_SEARCH), false);

    max = gtk_adjustment_get_upper(adjust) - gtk_adjustment_get_page_size(adjust);
    /* type scroll */
    if (arg->i & VB_SCROLL_TYPE_SCROLL) {
        gdouble value;
        if (arg->i & VB_SCROLL_UNIT_LINE) {
            value = vb.config.scrollstep;
        } else if (arg->i & VB_SCROLL_UNIT_HALFPAGE) {
            value = gtk_adjustment_get_page_size(adjust) / 2;
        } else {
            value = gtk_adjustment_get_page_size(adjust);
        }
        new = gtk_adjustment_get_value(adjust) + direction * value * count;
    } else if (vb.state.count) {
        /* jump - if count is set to count% of page */
        new = max * vb.state.count / 100;
    } else if (direction == 1) {
        /* jump to top */
        new = gtk_adjustment_get_upper(adjust);
    } else {
        /* jump to bottom */
        new = gtk_adjustment_get_lower(adjust);
    }
    gtk_adjustment_set_value(adjust, new > max ? max : new);

    return true;
}

gboolean command_map(const Arg *arg)
{
    char *key;

    vb_set_mode(VB_MODE_NORMAL, false);

    if (!arg->s) {
        return false;
    }
    if ((key = strchr(arg->s, '='))) {
        *key = '\0';
        return keybind_add_from_string(arg->s, key + 1, arg->i);
    }
    return false;
}

gboolean command_unmap(const Arg *arg)
{
    vb_set_mode(VB_MODE_NORMAL, false);

    return keybind_remove_from_string(arg->s, arg->i);
}

gboolean command_set(const Arg *arg)
{
    gboolean success;
    char *param = NULL, *line = NULL;

    vb_set_mode(VB_MODE_NORMAL, false);
    if (!arg->s || *(arg->s) == '\0') {
        return false;
    }

    line = g_strdup(arg->s);
    g_strstrip(line);

    /* split the input string into parameter and value part */
    if ((param = strchr(line, '='))) {
        *param = '\0';
        param++;
        success = setting_run(line, param ? param : NULL);
    } else {
        success = setting_run(line, NULL);
    }
    g_free(line);

    return success;
}

gboolean command_inspect(const Arg *arg)
{
    gboolean enabled;
    WebKitWebSettings *settings = NULL;

    vb_set_mode(VB_MODE_NORMAL, false);

    settings = webkit_web_view_get_settings(vb.gui.webview);
    g_object_get(G_OBJECT(settings), "enable-developer-extras", &enabled, NULL);
    if (enabled) {
        if (vb.state.is_inspecting) {
            webkit_web_inspector_close(vb.gui.inspector);
        } else {
            webkit_web_inspector_show(vb.gui.inspector);
        }
        return true;
    }

    vb_echo(VB_MSG_ERROR, true, "webinspector is not enabled");

    return false;
}

gboolean command_hints(const Arg *arg)
{
    int mode = arg->i;
    char *prefix = "";
    /* set prefix string according to hint type */
    switch (HINTS_GET_TYPE(mode)) {
        case HINTS_TYPE_LINK:
            if (mode & HINTS_PROCESS_OPEN) {
                prefix = mode & HINTS_OPEN_NEW ? "," : ".";
            } else if (mode & HINTS_PROCESS_INPUT) {
                prefix = mode & HINTS_OPEN_NEW ? ";t" : ";o";
            } else if (mode & HINTS_PROCESS_YANK) {
                prefix = ";y";
            } else if (mode & HINTS_PROCESS_SAVE) {
                prefix = ";s";
            }
            break;

        case HINTS_TYPE_IMAGE:
            if (mode & HINTS_PROCESS_OPEN) {
                prefix = mode & HINTS_OPEN_NEW ? ";I" : ";i";
            }
            break;

        case HINTS_TYPE_EDITABLE:
            prefix = ";e";
            break;
    }

    vb_echo_force(VB_MSG_NORMAL, false, "%s%s", prefix, arg->s ? arg->s : "");

    /* mode will be set in hints_create - so we don't neet to do it here */
    hints_create(arg->s, arg->i, strlen(prefix));

    return true;
}

gboolean command_yank(const Arg *arg)
{
    vb_set_mode(VB_MODE_NORMAL, true);

    if (arg->i & COMMAND_YANK_SELECTION) {
        char *text = NULL;
        /* copy current selection to clipboard */
        webkit_web_view_copy_clipboard(vb.gui.webview);
        text = gtk_clipboard_wait_for_text(PRIMARY_CLIPBOARD());
        if (!text) {
            text = gtk_clipboard_wait_for_text(SECONDARY_CLIPBOARD());
        }
        if (text) {
            vb_echo_force(VB_MSG_NORMAL, false, "Yanked: %s", text);
            g_free(text);

            return true;
        }

        return false;
    }
    /* use current arg.s a new clipboard content */
    Arg a = {arg->i};
    if (arg->i & COMMAND_YANK_URI) {
        /* yank current url */
        a.s = g_strdup(GET_URI());
    } else {
        a.s = arg->s ? g_strdup(arg->s) : NULL;
    }
    if (a.s) {
        vb_set_clipboard(&a);
        vb_echo_force(VB_MSG_NORMAL, false, "Yanked: %s", a.s);
        g_free(a.s);

        return true;
    }

    return false;
}

gboolean command_paste(const Arg *arg)
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

        return true;
    }
    return false;
}

gboolean command_search(const Arg *arg)
{
    static SearchDirection dir;
    static char *query = NULL;
#ifdef FEATURE_SEARCH_HIGHLIGHT
    static gboolean highlight = false;
#endif
    gboolean forward;

    if (arg->i == VB_SEARCH_OFF) {
#ifdef FEATURE_SEARCH_HIGHLIGHT
        webkit_web_view_unmark_text_matches(vb.gui.webview);
        highlight = false;
#endif
        return true;
    }

    /* copy search query for later use */
    if (arg->s) {
        OVERWRITE_STRING(query, arg->s);
        /* set dearch dir only when the searching is started */
        dir = arg->i;
    }

    forward = !(arg->i ^ dir);

    if (query) {
#ifdef FEATURE_SEARCH_HIGHLIGHT
        if (!highlight) {
            /* highlight matches if the search is started new or continued
             * after switch to normal mode which calls this function with
             * VB_SEARCH_OFF */
            webkit_web_view_mark_text_matches(vb.gui.webview, query, false, 0);
            webkit_web_view_set_highlight_text_matches(vb.gui.webview, true);

            /* mark the search as active */
            highlight = true;
        }
#endif

        /* make sure we have a count greater than 0 */
        vb.state.count = vb.state.count ? vb.state.count : 1;
        do {
            if (!webkit_web_view_search_text(vb.gui.webview, query, false, forward, true)) {
                break;
            }
        } while (--vb.state.count);
    }

    vb_set_mode(VB_MODE_NORMAL | VB_MODE_SEARCH, false);

    return true;
}

gboolean command_selsearch(const Arg *arg)
{
    char *query = NULL;
    gboolean res;

    /* don't start a new search if already in search mode */
    if (vb.state.mode & VB_MODE_SEARCH) {
        /* don't set a mode - keep the search mode like it is like in vim */
        return false;
    }

    /* there is no function to get the selected text so we copy current
     * selection to clipboard */
    webkit_web_view_copy_clipboard(vb.gui.webview);
    query = gtk_clipboard_wait_for_text(PRIMARY_CLIPBOARD());
    if (!query) {
        vb_set_mode(VB_MODE_NORMAL, false);

        return false;
    }

    res = command_search(&((Arg){arg->i, query}));
    g_free(query);

    return res;
}

gboolean command_shortcut(const Arg *arg)
{
    gboolean result;

    vb_set_mode(VB_MODE_NORMAL, false);

    if (arg->i) {
        char *handle;

        if (arg->s && (handle = strchr(arg->s, '='))) {
            *handle = '\0';
            handle++;
            result = shortcut_add(arg->s, handle);
        } else {
            return false;
        }
    } else {
        /* remove the shortcut */
        result = shortcut_remove(arg->s);
    }

    return result;
}

gboolean command_shortcut_default(const Arg *arg)
{
    vb_set_mode(VB_MODE_NORMAL, false);

    return shortcut_set_default(arg->s);
}

gboolean command_zoom(const Arg *arg)
{
    float step, level;
    int count = vb.state.count ? vb.state.count : 1;

    vb_set_mode(VB_MODE_NORMAL, false);

    if (arg->i & COMMAND_ZOOM_RESET) {
        webkit_web_view_set_zoom_level(vb.gui.webview, 1.0);

        return true;
    }

    level = webkit_web_view_get_zoom_level(vb.gui.webview);

    WebKitWebSettings *setting = webkit_web_view_get_settings(vb.gui.webview);
    g_object_get(G_OBJECT(setting), "zoom-step", &step, NULL);

    webkit_web_view_set_full_content_zoom(
        vb.gui.webview, (arg->i & COMMAND_ZOOM_FULL) > 0
    );

    webkit_web_view_set_zoom_level(
        vb.gui.webview,
        level + (float)(count *step) * (arg->i & COMMAND_ZOOM_IN ? 1.0 : -1.0)
    );

    return true;
}

gboolean command_history(const Arg *arg)
{
    const char *input = GET_TEXT();
    char *entry       = history_get(input, arg->i);

    if (!entry) {
        return false;
    }

    vb_echo_force(VB_MSG_NORMAL, false, "%s", entry);
    g_free(entry);

    return true;
}

gboolean command_bookmark(const Arg *arg)
{
    vb_set_mode(VB_MODE_NORMAL, false);

    if (!arg->i) {
        if (bookmark_remove(arg->s ? arg->s : GET_URI())) {
            vb_echo_force(VB_MSG_NORMAL, false, "Bookmark removed");

            return true;
        }
    } else if (bookmark_add(GET_URI(), webkit_web_view_get_title(vb.gui.webview), arg->s)) {
        vb_echo_force(VB_MSG_NORMAL, false, "Bookmark added");

        return true;
    }

    return false;
}

gboolean command_eval(const Arg *arg)
{
    gboolean success;
    char *value = NULL;

    vb_set_mode(VB_MODE_NORMAL, false);

    success = vb_eval_script(
        webkit_web_view_get_main_frame(vb.gui.webview), arg->s, NULL, &value
    );
    if (success) {
        vb_echo_force(VB_MSG_NORMAL, false, "%s", value);
    } else {
        vb_echo_force(VB_MSG_ERROR, true, "%s", value);
    }
    g_free(value);

    return success;
}

gboolean command_nextprev(const Arg *arg)
{
    if (vb.state.mode & VB_MODE_HINTING) {
        hints_focus_next(arg->i ? true : false);
    } else {
        /* mode will be set in completion_complete */
        completion_complete(arg->i ? true : false);
    }

    return true;
}

gboolean command_descent(const Arg *arg)
{
    gboolean result;
    int count = vb.state.count ? vb.state.count : 1;
    const char *uri, *p = NULL, *domain = NULL;

    uri = GET_URI();

    vb_set_mode(VB_MODE_NORMAL, false);
    if (!uri || *uri == '\0') {
        return false;
    }

    /* get domain part */
    if (!(domain = strstr(uri, "://")) || !(domain = strchr(domain + 3, '/'))) {
        return false;
    }

    if (arg->i) {
        p = domain;
    } else {
        /* start at the end */
        p = uri + strlen(uri);
        /* if last char is / increment count to step over this first */
        if (*(p - 1) == '/') {
            count++;
        }
        for (int i = 0; i < count; i++) {
            while (*(p--) != '/') {
                if (p == uri) {
                    /* reach the beginning */
                    return false;
                }
            }
        }
        /* keep the last / in uri */
        p++;
    }

    /* if the url is shorter than the domain use the domain instead */
    if (p < domain) {
        p = domain;
    }

    Arg a = {VB_TARGET_CURRENT};
    a.s = g_strndup(uri, p - uri + 1);
    result = vb_load_uri(&a);
    g_free(a.s);

    return result;
}

gboolean command_save(const Arg *arg)
{
    WebKitDownload *download;
    const char *uri, *path = NULL;

    vb_set_mode(VB_MODE_NORMAL, false);
    if (arg->i == COMMAND_SAVE_CURRENT) {
        uri = GET_URI();
        /* given string is the path to save the download to */
        if (arg->s && *(arg->s) != '\0') {
            path = arg->s;
        }
    } else {
        uri = arg->s;
    }

    if (!uri || *uri == '\0') {
        return false;
    }

    download = webkit_download_new(webkit_network_request_new(uri));
    vb_download(vb.gui.webview, download, path);

    return true;
}

gboolean command_shellcmd(const Arg *arg)
{
    int status, argc;
    char *cmd, *exp, *error = NULL, *out = NULL, **argv;

    vb_set_mode(VB_MODE_NORMAL, false);
    if (!arg->s || *(arg->s) == '\0') {
        return false;
    }

    exp = expand_string(arg->s);
    cmd = g_strdup_printf(SHELL_CMD, exp);
    g_free(exp);
    if (!g_shell_parse_argv(cmd, &argc, &argv, NULL)) {
        vb_echo(VB_MSG_ERROR, true, "Could not parse command args");
        g_free(cmd);

        return false;
    }
    g_free(cmd);

    g_spawn_sync(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &out, &error, &status, NULL);
    g_strfreev(argv);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        vb_echo(VB_MSG_NORMAL, true, "  %s", out);
        return true;
    }

    vb_echo(VB_MSG_ERROR, true, "[%d] %s", WEXITSTATUS(status), error);
    return false;
}

gboolean command_editor(const Arg *arg)
{
    char *file_path = NULL;
    const char *text;
    char **argv;
    int argc;
    GPid pid;
    gboolean success;

    if (!vb.config.editor_command) {
        vb_echo(VB_MSG_ERROR, true, "No editor-command configured");
        return false;
    }
    Element* active = dom_get_active_element(vb.gui.webview);

    /* check if element is suitable for editing */
    if (!active || !dom_is_editable(active)) {
        return false;
    }

    text = dom_editable_element_get_value(active);
    if (!text) {
        return false;
    }

    if (!util_create_tmp_file(text, &file_path)) {
        return false;
    }

    /* spawn editor */
    char* command = g_strdup_printf(vb.config.editor_command, file_path);
    if (!g_shell_parse_argv(command, &argc, &argv, NULL)) {
        fprintf(stderr, "Could not parse editor-command");
        g_free(command);
        return false;
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
        return false;
    }

    /* disable the active element */
    dom_editable_element_set_disable(active, true);

    OpenEditorData *data = g_new0(OpenEditorData, 1);
    data->file    = file_path;
    data->element = active;

    g_child_watch_add(pid, (GChildWatchFunc)editor_resume, data);

    return true;
}

static void editor_resume(GPid pid, int status, OpenEditorData *data)
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

    g_free(data->file);
    g_free(data);
    g_spawn_close_pid(pid);
}

static CommandInfo *command_lookup(const char* name)
{
    CommandInfo *c;
    c = g_hash_table_lookup(short_commands, name);
    if (!c) {
        c = g_hash_table_lookup(commands, name);
    }

    return c;
}

/**
 * Expands paceholders in given string.
 * % - expanded to current uri
 * TODO allow modifiers like :p :h :e :r like in vim expand()
 *
 * Returned string must be freed.
 */
static char *expand_string(const char *str)
{
    if (!str) {
        return NULL;
    }
    return util_str_replace("%", GET_URI(), str);
}
