/**
 * vimp - a webkit based vim like browser.
 *
 * Copyright (C) 2012 Daniel Carl
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

#ifndef MAIN_H
#define MAIN_H

#include <stdlib.h>
#include <string.h>
#include <webkit/webkit.h>

#define LENGTH(x) (sizeof x / sizeof x[0])

#ifdef DEBUG
#define PRINT_DEBUG(...) do { \
    fprintf(stderr, "\n\033[31;1mDEBUG:\033[0m %s:%d:%s()\t", __FILE__, __LINE__, __func__); \
    fprintf(stderr, __VA_ARGS__);\
} while(0);
#define TIMER_START GTimer *__timer; do {__timer = g_timer_new(); g_timer_start(__timer);} while(0)
#define TIMER_END do {gulong __debug_micro = 0; gdouble __debug_elapsed = g_timer_elapsed(__timer, &__debug_micro);\
    PRINT_DEBUG("\033[33mtimer:\033[0m elapsed: %f, micro: %lu", __debug_elapsed, __debug_micro);\
    g_timer_destroy(__timer); \
} while(0)
#else
#define PRINT_DEBUG(message, ...)
#define TIMER_START
#define TIMER_END
#endif

#define GET_TEXT() (gtk_entry_get_text(GTK_ENTRY(vp.gui.inputbox)))
#define CLEAN_MODE(mode) ((mode) & ~(VP_MODE_COMPLETE))
#define GET_CLEAN_MODE() (CLEAN_MODE(vp.state.mode))
#define CLEAR_INPUT() (vp_echo(VP_MSG_NORMAL, ""))
#define CURRENT_URL() webkit_web_view_get_uri(vp.gui.webview)
#define PRIMARY_CLIPBOARD() gtk_clipboard_get(GDK_SELECTION_PRIMARY)
#define SECONDARY_CLIPBOARD() gtk_clipboard_get(GDK_NONE)

#define IS_ESCAPE_KEY(k, s) ((k == GDK_Escape && s == 0) || (k == GDK_c && s == GDK_CONTROL_MASK))
#define CLEAN_STATE_WITH_SHIFT(e) ((e)->state & (GDK_MOD1_MASK|GDK_MOD4_MASK|GDK_SHIFT_MASK|GDK_CONTROL_MASK))
#define CLEAN_STATE(e)            ((e)->state & (GDK_MOD1_MASK|GDK_MOD4_MASK|GDK_CONTROL_MASK))

#ifdef HAS_GTK3
#define VpColor GdkRGBA
#define VP_COLOR_PARSE(color, string)   (gdk_rgba_parse(color, string))
#define VP_COLOR_TO_STRING(color)       (gdk_rgba_to_string(color))
#define VP_WIDGET_OVERRIDE_BACKGROUND   gtk_widget_override_background_color
#define VP_WIDGET_OVERRIDE_BASE         gtk_widget_override_background_color
#define VP_WIDGET_OVERRIDE_COLOR        gtk_widget_override_color
#define VP_WIDGET_OVERRIDE_TEXT         gtk_widget_override_color
#define VP_WIDGET_OVERRIDE_FONT         gtk_widget_override_font
#else
#define VpColor GdkColor
#define VP_COLOR_PARSE(color, string)   (gdk_color_parse(string, color))
#define VP_COLOR_TO_STRING(color)       (gdk_color_to_string(color))
#define VP_WIDGET_OVERRIDE_BACKGROUND   gtk_widget_modify_bg
#define VP_WIDGET_OVERRIDE_BASE         gtk_widget_modify_base
#define VP_WIDGET_OVERRIDE_COLOR        gtk_widget_modify_fg
#define VP_WIDGET_OVERRIDE_TEXT         gtk_widget_modify_text
#define VP_WIDGET_OVERRIDE_FONT         gtk_widget_modify_font
#endif

#define HEX_COLOR_LEN 8

/* enums */
typedef enum _vp_mode {
    VP_MODE_NORMAL        = 1<<0,
    VP_MODE_COMMAND       = 1<<1,
    VP_MODE_PATH_THROUGH  = 1<<2,
    VP_MODE_INSERT        = 1<<3,
    VP_MODE_SEARCH        = 1<<4,
    VP_MODE_COMPLETE      = 1<<5,
    VP_MODE_HINTING       = 1<<6,
} Mode;

enum {
    VP_NAVIG_BACK,
    VP_NAVIG_FORWARD,
    VP_NAVIG_RELOAD,
    VP_NAVIG_RELOAD_FORCE,
    VP_NAVIG_STOP_LOADING
};

enum {
    VP_TARGET_CURRENT,
    VP_TARGET_NEW
};

enum {
    VP_INPUT_CURRENT_URI = 1
};

/*
1 << 0:  0 = jump,              1 = scroll
1 << 1:  0 = vertical,          1 = horizontal
1 << 2:  0 = top/left,          1 = down/right
1 << 3:  0 = paging/halfpage,   1 = line
1 << 4:  0 = paging,            1 = halfpage
*/
enum {VP_SCROLL_TYPE_JUMP, VP_SCROLL_TYPE_SCROLL};
enum {
    VP_SCROLL_AXIS_V,
    VP_SCROLL_AXIS_H = (1 << 1)
};
enum {
    VP_SCROLL_DIRECTION_TOP,
    VP_SCROLL_DIRECTION_DOWN  = (1 << 2),
    VP_SCROLL_DIRECTION_LEFT  = VP_SCROLL_AXIS_H,
    VP_SCROLL_DIRECTION_RIGHT = VP_SCROLL_AXIS_H | (1 << 2)
};
enum {
    VP_SCROLL_UNIT_PAGE,
    VP_SCROLL_UNIT_LINE     = (1 << 3),
    VP_SCROLL_UNIT_HALFPAGE = (1 << 4)
};

typedef enum {
    VP_MSG_NORMAL,
    VP_MSG_ERROR,
    VP_MSG_LAST
} MessageType;

typedef enum {
    VP_COMP_NORMAL,
    VP_COMP_ACTIVE,
    VP_COMP_LAST
} CompletionStyle;

enum {
    FILES_FIRST = 0,
    FILES_CONFIG = 0,
    FILES_COOKIE,
    FILES_LAST
};

typedef enum {
    TYPE_CHAR,
    TYPE_BOOLEAN,
    TYPE_INTEGER,
    TYPE_FLOAT,
    TYPE_COLOR,
    TYPE_FONT,
} Type;

enum {
    VP_CLIPBOARD_PRIMARY   = (1<<0),
    VP_CLIPBOARD_SECONDARY = (1<<1)
};

/* structs */
typedef struct {
    gint     i;
    gchar*   s;
} Arg;

/* statusbar */
typedef struct {
    GtkBox*     box;
    GtkWidget*  left;
    GtkWidget*  right;
} StatusBar;

/* gui */
typedef struct {
    GtkWidget*         window;
    WebKitWebView*     webview;
    WebKitWebInspector *inspector;
    GtkWidget*         viewport;
    GtkBox*            box;
    GtkWidget*         eventbox;
    GtkWidget*         inputbox;
    GtkWidget*         compbox;
    StatusBar          statusbar;
    GtkScrollbar*      sb_h;
    GtkScrollbar*      sb_v;
    GtkAdjustment*     adjust_h;
    GtkAdjustment*     adjust_v;
} Gui;

/* state */
typedef struct {
    Mode            mode;
    gchar           modkey;
    guint           count;
    GdkNativeWindow embed;
    guint           progress;
} State;

/* behaviour */
typedef struct {
    /* command list: (key)name -> (value)Command  */
    GHashTable* commands;
    /* keybindings */
    GSList*     keys;
    GString*    modkeys;
} Behaviour;

typedef struct {
    SoupSession*    soup_session;
} Network;

typedef struct {
#ifdef FEATURE_COOKIE
    time_t cookie_timeout;
#endif
    gint   scrollstep;
    guint  max_completion_items;
} Config;

typedef struct {
    GList* completions;
    GList* active;
    gint   count;
} Completions;

typedef struct {
    VpColor               input_fg[VP_MSG_LAST];
    VpColor               input_bg[VP_MSG_LAST];
    PangoFontDescription* input_font[VP_MSG_LAST];
    /* completion */
    VpColor               comp_fg[VP_COMP_LAST];
    VpColor               comp_bg[VP_COMP_LAST];
    PangoFontDescription* comp_font[VP_COMP_LAST];
    /* hint style */
    gchar                 hint_bg[HEX_COLOR_LEN];
    gchar                 hint_bg_focus[HEX_COLOR_LEN];
    gchar                 hint_fg[HEX_COLOR_LEN];
    gchar*                hint_style;
    /* status bar */
    VpColor               status_bg;
    VpColor               status_fg;
    PangoFontDescription* status_font;
} Style;

typedef struct {
    GList* list;
    gulong focusNum;
    gulong num;
    guint  mode;
    guint  prefixLength;
} Hints;

/* core struct */
typedef struct {
    Gui           gui;
    State         state;
    Behaviour     behave;
    gchar*        files[FILES_LAST];
#ifdef FEATURE_COOKIE
    Network       net;
#endif
    Config        config;
    Completions   comps;
    Style         style;
    GHashTable*   settings;
    Hints         hints;
} VpCore;

/* main object */
extern VpCore vp;

/* functions */
void vp_update_statusbar(void);
void vp_update_urlbar(const gchar* uri);
void vp_echo(const MessageType type, gboolean hide, const char *error, ...);
gboolean vp_set_mode(Mode mode, gboolean clean);
void vp_set_widget_font(GtkWidget* widget, const VpColor* fg, const VpColor* bg, PangoFontDescription* font);
gboolean vp_load_uri(const Arg* arg);
void vp_clean_up(void);
void vp_clean_input(void);
gboolean vp_set_clipboard(const Arg* arg);

#endif /* end of include guard: MAIN_H */
