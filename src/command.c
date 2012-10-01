#include "command.h"

static CommandInfo cmd_list[] = {
    /* command   function */
    {"quit",     quit},
    {"source",   view_source},
};

void command_init()
{
    guint i;
    vp.behave.commands = g_hash_table_new(g_str_hash, g_str_equal);

    for (i = 0; i < LENGTH(cmd_list); i++) {
        g_hash_table_insert(vp.behave.commands, (gpointer)cmd_list[i].name, &cmd_list[i]);
    }
}

void command_parse_line(const gchar* line)
{
    gchar* string = g_strdup(line);

    /* strip trailing newline, and any other whitespace from left */
    g_strstrip(string);

    if (strcmp(string, "")) {
        /* ignore comment lines */
        if ((string[0] != '#')) {
            Arg* a = NULL;
            const CommandInfo* c = command_parse_parts(string, a);
            if (c) {
                command_run_command(c, a);
            }
            g_free(a->s);
        }
    }

    g_free(string);
}

/* static? */
const CommandInfo* command_parse_parts(const gchar* line, Arg* arg)
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

    arg->s = g_strdup(tokens[2]);
    g_strfreev(tokens);

    return c;
}

void command_run_command(const CommandInfo* c, Arg* arg)
{
    c->function(arg);
}

void quit(Arg* arg)
{
    vp_close_browser(); 
}

void view_source(Arg* arg)
{
    gboolean mode = webkit_web_view_get_view_source_mode(vp.gui.webview);
    webkit_web_view_set_view_source_mode(vp.gui.webview, !mode);
    webkit_web_view_reload(vp.gui.webview);
}
