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
    fprintf(stderr, "\n"); \
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

/* enums */
typedef enum _vp_mode {
    VP_MODE_NORMAL        = 1<<0,
    VP_MODE_COMMAND       = 1<<1,
    VP_MODE_PATH_THROUGH  = 1<<2,
    VP_MODE_INSERT        = 1<<3,
    VP_MODE_COMPLETE      = 1<<4,
    VP_MODE_SEARCH        = 1<<5,
} Mode;

enum {
    VP_NAVIG_BACK,
    VP_NAVIG_FORWARD,
    VP_NAVIG_RELOAD,
    VP_NAVIG_RELOAD_FORCE,
    VP_NAVIG_STOP_LOADING
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
    TYPE_DOUBLE,
    TYPE_COLOR,
} Type;

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
    GtkWidget*     window;
    WebKitWebView* webview;
    GtkWidget*     viewport;
    GtkBox*        box;
    GtkWidget*     eventbox;
    GtkWidget*     inputbox;
    GtkWidget*     compbox;
    StatusBar      statusbar;
    GtkScrollbar*  sb_h;
    GtkScrollbar*  sb_v;
    GtkAdjustment* adjust_h;
    GtkAdjustment* adjust_v;
} Gui;

/* state */
typedef struct {
    Mode          mode;
    gchar         modkey;
    guint         count;
} State;

/* behaviour */
typedef struct {
    /* command list: (key)name -> (value)Command  */
    GHashTable* commands;
} Behaviour;

typedef struct {
    SoupSession*    soup_session;
} Network;

typedef struct {
#ifdef FEATURE_COOKIE
    time_t cookie_timeout;
#endif
    gint   scrollstep;
} Config;

typedef struct {
    GList* completions;
    GList* active;
} Completions;

typedef struct {
    GdkColor              input_fg[VP_MSG_LAST];
    GdkColor              input_bg[VP_MSG_LAST];
    PangoFontDescription* input_font[VP_MSG_LAST];
    GdkColor              comp_fg[VP_COMP_LAST];
    GdkColor              comp_bg[VP_COMP_LAST];
    PangoFontDescription* comp_font[VP_COMP_LAST];
} Style;

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
#if 0
    Ssl           ssl;
    Communication comm;
    Info          info;
#endif
} VpCore;

/* main object */
extern VpCore vp;

/* functions */
void vp_update_statusbar(void);
void vp_update_urlbar(const gchar* uri);
void vp_echo(const MessageType type, const char* error, ...);
gboolean vp_set_mode(const Arg* arg);
void vp_set_widget_font(GtkWidget* widget, const GdkColor* fg, const GdkColor* bg, PangoFontDescription* font);
gboolean vp_load_uri(const Arg* arg);
void vp_clean_up(void);

#endif /* end of include guard: MAIN_H */
