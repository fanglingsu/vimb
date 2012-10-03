#include "command.h"

static CommandInfo cmd_list[] = {
    /* command   function     arg */
    {"quit",     quit,        {0}},
    {"source",   view_source, {0}},
};

void command_init()
{
    guint i;
    vp.behave.commands = g_hash_table_new(g_str_hash, g_str_equal);

    for (i = 0; i < LENGTH(cmd_list); i++) {
        g_hash_table_insert(vp.behave.commands, (gpointer)cmd_list[i].name, &cmd_list[i]);
    }
}

void command_run(const gchar* name)
{
    CommandInfo* c = NULL;
    Arg a;
    c = g_hash_table_lookup(vp.behave.commands, name);
    if (!c) {
        return;
    }
    a.i = c->arg.i;
    a.s = g_strdup(c->arg.s);
    c->function(&a);
    g_free(a.s);
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
