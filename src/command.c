#include "command.h"
#include "main.h"

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
};

void command_init()
{
    guint i;
    vp.behave.commands = g_hash_table_new(g_str_hash, g_str_equal);

    for (i = 0; i < LENGTH(cmd_list); i++) {
        g_hash_table_insert(vp.behave.commands, (gpointer)cmd_list[i].name, &cmd_list[i]);
    }
}

gboolean command_run(const gchar* name, const gchar* param)
{
    CommandInfo* c = NULL;
    Arg a;
    c = g_hash_table_lookup(vp.behave.commands, name);
    if (!c) {
        return FALSE;
    }
    a.i = c->arg.i;
    a.s = g_strdup(param ? param : c->arg.s);
    c->function(&a);
    g_free(a.s);

    return TRUE;
}
