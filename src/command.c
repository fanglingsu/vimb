#include "command.h"
#include "main.h"

static CommandInfo cmd_list[] = {
    /* command          function          arg */
    {"quit",            vp_close_browser, {0}},
    {"source",          vp_view_source,   {0}},
    {"back",            vp_navigate,      {NAVIG_BACK}},
    {"forward",         vp_navigate,      {NAVIG_FORWARD}},
    {"reload",          vp_navigate,      {NAVIG_RELOAD}},
    {"reload!",         vp_navigate,      {NAVIG_RELOAD_FORCE}},
    {"stop",            vp_navigate,      {NAVIG_STOP_LOADING}},
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
