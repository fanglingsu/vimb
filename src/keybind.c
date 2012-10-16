#include "main.h"
#include "keybind.h"
#include "command.h"

static GSList* keys = NULL;
static GString* modkeys = NULL;

static GSList* keybind_find(int mode, guint modkey, guint modmask, guint keyval);
static gboolean keybind_keypress_callback(WebKitWebView* webview, GdkEventKey* event);


void keybind_init(void)
{
    modkeys = g_string_new("");
    g_signal_connect(G_OBJECT(vp.gui.window), "key-press-event", G_CALLBACK(keybind_keypress_callback), NULL);
}

void keybind_add(int mode, guint modkey, guint modmask, guint keyval, const gchar* command)
{
    Keybind* keybind = g_new0(Keybind, 1);

    keybind->mode    = mode;
    keybind->modkey  = modkey;
    keybind->modmask = modmask;
    keybind->keyval  = keyval;
    keybind->command = g_strdup(command);

    keys = g_slist_prepend(keys, keybind);

     /* save the modkey also in the modkey string */
    if (modkey) {
        g_string_append_c(modkeys, modkey);
    }
}

void keybind_remove(int mode, guint modkey, guint modmask, guint keyval)
{
    GSList* link = keybind_find(mode, modkey, modmask, keyval);
    if (link) {
        keys = g_slist_delete_link(keys, link);
    }
    /* TODO remove eventually no more used modkeys */
}

GSList* keybind_find(int mode, guint modkey, guint modmask, guint keyval)
{
    GSList* link;
    for (link = keys; link != NULL; link = link->next) {
        Keybind* keybind = (Keybind*)link->data;

        if (keybind->keyval == keyval
                && keybind->modmask == modmask
                && keybind->modkey == modkey
                && keybind->mode == mode) {
            return link;
        }
    }

    return NULL;
}

static gboolean keybind_keypress_callback(WebKitWebView* webview, GdkEventKey* event)
{
    GdkModifierType irrelevant;
    guint keyval;
    static GdkKeymap *keymap;

    keymap = gdk_keymap_get_default();

    /* Get a mask of modifiers that shouldn't be considered for this event.
     * E.g.: It shouldn't matter whether ';' is shifted or not. */
    gdk_keymap_translate_keyboard_state(keymap, event->hardware_keycode,
            event->state, event->group, &keyval, NULL, NULL, &irrelevant);

    /* check for escape or modkeys or counts */
    if ((CLEAN(event->state) & ~irrelevant) == 0) {
        if (IS_ESCAPE(event)) {
            /* switch to normal mode and clear the input box */
            Arg a = {VP_MODE_NORMAL, ""};
            vp_set_mode(&a);

            return TRUE;
        }
        /* allow mode keys and counts only in normal mode */
        if (VP_MODE_NORMAL == vp.state.mode) {
            if (vp.state.modkey == 0 && ((event->keyval >= GDK_1 && event->keyval <= GDK_9)
                    || (event->keyval == GDK_0 && vp.state.count))) {
                /* append the new entered count to previous one */
                vp.state.count = (vp.state.count ? vp.state.count * 10 : 0) + (event->keyval - GDK_0);
                vp_update_statusbar();

                return TRUE;
            }
            if (strchr(modkeys->str, event->keyval) && vp.state.modkey != event->keyval) {
                vp.state.modkey = (gchar)event->keyval;
                vp_update_statusbar();

                return TRUE;
            }
        }
    }

    /* check for keybinding */
    GSList* link = keybind_find(vp.state.mode, vp.state.modkey,
            (CLEAN(event->state) & ~irrelevant), keyval);

    if (link) {
        Keybind* keybind = (Keybind*)link->data;
        command_run(keybind->command, NULL);

        /* if key binding used, remove the modkey */
        vp.state.modkey = vp.state.count = 0;
        vp_update_statusbar();

        return TRUE;
    }

    return FALSE;
}
