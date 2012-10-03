#include "main.h"
#include "keybind.h"
#include "command.h"

static GSList* keys = NULL;
static GString* modkeys = NULL;

static gboolean keybind_keypress_callback(WebKitWebView* webview, GdkEventKey* event);


void keybind_init(void)
{
    modkeys = g_string_new("");
    g_signal_connect(G_OBJECT(vp.gui.webview), "key-press-event", G_CALLBACK(keybind_keypress_callback), NULL);
}

void keybind_add(int mode, guint modkey, guint modmask, guint keyval, const gchar* command)
{
    struct _keybind_key* keybind = g_new0(struct _keybind_key, 1);

    keybind->mode    = mode;
    keybind->modkey  = modkey;
    keybind->modmask = modmask;
    keybind->keyval  = keyval;
    keybind->command = g_strdup(command);

    keys = g_slist_append(keys, keybind);

     /* save the modkey also in the modkey string */
    if (modkey) {
        g_string_append_c(modkeys, modkey);
    }
}

static gboolean keybind_keypress_callback(WebKitWebView* webview, GdkEventKey* event)
{
    GSList* tmp;
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
            /* TODO add function to main.c to switch the mode */
            /* switch to normal mode */
            vp.state.mode = VP_MODE_NORMAL;
            /* remove current modkey and set count back to 0 */
            vp.state.modkey = 0;
            vp.state.count  = 0;
            vp_update_statusbar();

            return TRUE;
        } else if (vp.state.modkey == 0 && ((event->keyval >= GDK_1 && event->keyval <= GDK_9)
                || (event->keyval == GDK_0 && vp.state.count))) {
            /* append the new entered count to previous one */
            vp.state.count = (vp.state.count ? vp.state.count * 10 : 0) + (event->keyval - GDK_0);
            vp_update_statusbar();

            return TRUE;
        } else if (strchr(modkeys->str, event->keyval) && vp.state.modkey != event->keyval) {
            vp.state.modkey = (gchar)event->keyval;
            vp_update_statusbar();

            return TRUE;
        }
    }
    /* check for keybinding */
    for (tmp = keys; tmp != NULL; tmp = tmp->next) {
        struct _keybind_key* keybind = (struct _keybind_key*)tmp->data;

        /* handle key presses */
        if (gdk_keyval_to_lower(event->keyval) == keybind->keyval
            && (event->state & keybind->modmask) == keybind->modmask
            && keybind->modkey == vp.state.modkey
            && keybind->command
        ) {
            command_run(keybind->command);

            /* if key binding used, remove the modkey */
            vp.state.modkey = 0;
            vp_update_statusbar();

            return TRUE;
        }
    }

    return FALSE;
}
