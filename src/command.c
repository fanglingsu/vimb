#include "command.h"

static CommandInfo cmd_list[] = {
    /* command   function */
    {"quit",     quit},
    {"source",   view_source},
};

static void command_sharg_append(GArray* a, const gchar* str);


void command_init()
{
    guint i;
    vp.behave.commands = g_hash_table_new(g_str_hash, g_str_equal);

    for (i = 0; i < LENGTH(cmd_list); i++) {
        g_hash_table_insert(vp.behave.commands, (gpointer)cmd_list[i].name, &cmd_list[i]);
    }
}

void command_parse_line(const gchar* line, GString* result)
{
    gchar* string = g_strdup(line);

    /* strip trailing newline, and any other whitespace from left */
    g_strstrip(string);

    if (strcmp(string, "")) {
        /* ignore comment lines */
        if ((string[0] != '#')) {
            GArray* a = g_array_new(TRUE, FALSE, sizeof(gchar*));
            const CommandInfo* c = command_parse_parts(string, a);
            if (c) {
                command_run_command(c, a, result);
            }
            g_array_free(a, TRUE);
        }
    }

    g_free(string);
}

/* static? */
const CommandInfo* command_parse_parts(const gchar* line, GArray* a)
{
    CommandInfo* c = NULL;

    /* split the line into the command and its parameters */
    gchar **tokens = g_strsplit(line, " ", 2);

    /* look up the command */
    c = g_hash_table_lookup(vp.behave.commands, tokens[0]);
    if (!c) {
        g_strfreev(tokens);
        return NULL;
    }

    gchar* p = g_strdup(tokens[1]);
    command_sharg_append(a, p);

    g_strfreev(tokens);
    g_free(p);

    return c;
}

void command_run_command(const CommandInfo* c, GArray* a, GString* result)
{
    c->function(a, result);
}

static void command_sharg_append(GArray* a, const gchar* str)
{
    const gchar* s = (str ? str : "");
    g_array_append_val(a, s);
}

void quit(GArray* argv, GString* result)
{
    vp_close_browser(); 
}

void view_source(GArray* argv, GString* result)
{
    gboolean mode = webkit_web_view_get_view_source_mode(vp.gui.webview);
    webkit_web_view_set_view_source_mode(vp.gui.webview, !mode);
    webkit_web_view_reload(vp.gui.webview);
}
