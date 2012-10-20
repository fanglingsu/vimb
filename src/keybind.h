#ifndef KEYBIND_H
#define KEYBIND_H

#include <gdk/gdkkeysyms.h>

/* the CLEAN_MOD_*_MASK defines have all the bits set that will be stripped from the modifier bit field */
#define CLEAN_MOD_NUMLOCK_MASK (GDK_MOD2_MASK)
#define CLEAN_MOD_BUTTON_MASK (GDK_BUTTON1_MASK|GDK_BUTTON2_MASK|GDK_BUTTON3_MASK|GDK_BUTTON4_MASK|GDK_BUTTON5_MASK)

/* remove unused bits, numlock symbol and buttons from keymask */
#define CLEAN(mask) (mask & (GDK_MODIFIER_MASK) & ~(CLEAN_MOD_NUMLOCK_MASK) & ~(CLEAN_MOD_BUTTON_MASK))
#define IS_ESCAPE(event) (IS_ESCAPE_KEY(CLEAN(event->state), event->keyval))
#define IS_ESCAPE_KEY(s, k) ((s == 0 && k == GDK_Escape) || (s == GDK_CONTROL_MASK && k == GDK_c))

typedef struct {
    int    mode;        /* mode maks for allowed browser modes */
    guint  modkey;
    guint  modmask;     /* modemask for the kayval */
    guint  keyval;
    gchar* command;     /* command to run */
} Keybind;

void keybind_init(void);
void keybind_add_from_string(const gchar* str, const Mode mode);
void keybind_remove_from_string(const gchar* str, const Mode mode);

#endif /* end of include guard: KEYBIND_H */
