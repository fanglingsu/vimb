/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2015 Daniel Carl
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

#ifndef _MAIN_H
#define _MAIN_H

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <webkit/webkit.h>
#include <JavaScriptCore/JavaScript.h>
#include <fcntl.h>
#include <stdio.h>
#ifdef HAS_GTK3
#include <gdk/gdkx.h>
#include <gtk/gtkx.h>
#endif
#include "config.h"
#ifdef FEATURE_HSTS
#include "hsts.h"
#endif

/* size of some I/O buffer */
#define BUF_SIZE  512

#define LENGTH(x) (sizeof x / sizeof x[0])

#define FLOCK(fd, cmd) { \
    struct flock lock = {.l_type=cmd,.l_start=0,.l_whence=SEEK_SET,.l_len=0}; \
    fcntl(fd, F_SETLK, lock); \
}

#ifdef DEBUG
#define PRINT_DEBUG(...) { \
    fprintf(stderr, "\n\033[31;1mDEBUG:\033[0m %s:%d:%s()\t", __FILE__, __LINE__, __func__); \
    fprintf(stderr, __VA_ARGS__);\
}
#define TIMER_START GTimer *__timer; {__timer = g_timer_new(); g_timer_start(__timer);}
#define TIMER_END {gdouble __debug_elapsed = g_timer_elapsed(__timer, NULL);\
    PRINT_DEBUG("\033[33mtimer:\033[0m %fs", __debug_elapsed);\
    g_timer_destroy(__timer); \
}
#else
#define PRINT_DEBUG(message, ...)
#define TIMER_START
#define TIMER_END
#endif

#define PRIMARY_CLIPBOARD() gtk_clipboard_get(GDK_SELECTION_PRIMARY)
#define SECONDARY_CLIPBOARD() gtk_clipboard_get(GDK_NONE)

#define OVERWRITE_STRING(t, s) {if (t) {g_free(t); t = NULL;} t = g_strdup(s);}
#define OVERWRITE_NSTRING(t, s, l) {if (t) {g_free(t); t = NULL;} t = g_strndup(s, l);}

#define GET_CHAR(n)  (((Setting*)g_hash_table_lookup(vb.config.settings, n))->value.s)
#define GET_INT(n)   (((Setting*)g_hash_table_lookup(vb.config.settings, n))->value.i)
#define GET_BOOL(n)  (((Setting*)g_hash_table_lookup(vb.config.settings, n))->value.b)

#ifdef HAS_GTK3
#define VbColor GdkRGBA
#define VB_COLOR_PARSE(color, string)           (gdk_rgba_parse(color, string))
#define VB_COLOR_TO_STRING(color)               (gdk_rgba_to_string(color))
#define VB_WIDGET_OVERRIDE_BACKGROUND(w, s, c)
#define VB_WIDGET_OVERRIDE_BASE(w, s, c)        (gtk_widget_override_background_color(w, s, c))
#define VB_WIDGET_OVERRIDE_COLOR(w, s, c)
#define VB_WIDGET_OVERRIDE_TEXT(w, s, c)        (gtk_widget_override_color(w, s, c))
#define VB_WIDGET_OVERRIDE_FONT(w, f)           (gtk_widget_override_font(w, f))

#define VB_GTK_STATE_NORMAL                     GTK_STATE_FLAG_NORMAL
#define VB_GTK_STATE_ACTIVE                     GTK_STATE_FLAG_ACTIVE
#define VB_GTK_STATE_SELECTED                   GTK_STATE_FLAG_SELECTED
#define VB_WIDGET_SET_STATE(w, s)               (gtk_widget_set_state_flags(w, s, true))

#else

#define VbColor GdkColor
#define VB_COLOR_PARSE(color, string)           (gdk_color_parse(string, color))
#define VB_COLOR_TO_STRING(color)               (gdk_color_to_string(color))
#define VB_WIDGET_OVERRIDE_BACKGROUND(w, s, c)  (gtk_widget_modify_bg(w, s, c))
#define VB_WIDGET_OVERRIDE_BASE(w, s, c)        (gtk_widget_modify_base(w, s, c))
#define VB_WIDGET_OVERRIDE_COLOR(w, s, c)       (gtk_widget_modify_fg(w, s, c))
#define VB_WIDGET_OVERRIDE_TEXT(w, s, c)        (gtk_widget_modify_text(w, s, c))
#define VB_WIDGET_OVERRIDE_FONT(w, f)           (gtk_widget_modify_font(w, f))

#define VB_GTK_STATE_NORMAL                     GTK_STATE_NORMAL
#define VB_GTK_STATE_ACTIVE                     GTK_STATE_ACTIVE
#define VB_GTK_STATE_SELECTED                   GTK_STATE_SELECTED
#define VB_WIDGET_SET_STATE(w, s)               (gtk_widget_set_state(w, s))
#endif

#ifndef SOUP_CHECK_VERSION
#define SOUP_CHECK_VERSION(major, minor, micro) (0)
#endif

/* the special mark ' must be the first in the list for easiest lookup */
#define VB_MARK_CHARS   "'abcdefghijklmnopqrstuvwxyz"
#define VB_MARK_TICK    0
#define VB_MARK_SIZE    (sizeof(VB_MARK_CHARS) - 1)

#define VB_USER_REG     "abcdefghijklmnopqrstuvwxyz"
/* registers in order displayed for :register command */
#define VB_REG_CHARS    "\"" VB_USER_REG ":%/;"
#define VB_REG_SIZE     (sizeof(VB_REG_CHARS) - 1)

/* enums */
typedef enum {
    RESULT_COMPLETE,
    RESULT_MORE,
    RESULT_ERROR
} VbResult;

typedef enum {
    VB_INPUT_UNKNOWN,
    VB_INPUT_SET              = 0x01,
    VB_INPUT_OPEN             = 0x02,
    VB_INPUT_TABOPEN          = 0x04,
    VB_INPUT_COMMAND          = 0x08,
    VB_INPUT_SEARCH_FORWARD   = 0x10,
    VB_INPUT_SEARCH_BACKWARD  = 0x20,
    VB_INPUT_BOOKMARK_ADD     = 0x40,
    VB_INPUT_ALL              = 0xff, /* map to match all input types */
} VbInputType;

enum {
    VB_NAVIG_BACK,
    VB_NAVIG_FORWARD,
    VB_NAVIG_RELOAD,
    VB_NAVIG_RELOAD_FORCE,
    VB_NAVIG_STOP_LOADING
};

enum {
    VB_TARGET_CURRENT,
    VB_TARGET_NEW
};

enum {
    VB_INPUT_CURRENT_URI = 1
};

/*
1 << 0:  0 = jump,              1 = scroll
1 << 1:  0 = vertical,          1 = horizontal
1 << 2:  0 = top/left,          1 = down/right
1 << 3:  0 = paging/halfpage,   1 = line
1 << 4:  0 = paging,            1 = halfpage
*/
enum {VB_SCROLL_TYPE_JUMP, VB_SCROLL_TYPE_SCROLL};
enum {
    VB_SCROLL_AXIS_V,
    VB_SCROLL_AXIS_H = (1 << 1)
};
enum {
    VB_SCROLL_DIRECTION_TOP,
    VB_SCROLL_DIRECTION_DOWN  = (1 << 2),
    VB_SCROLL_DIRECTION_LEFT  = VB_SCROLL_AXIS_H,
    VB_SCROLL_DIRECTION_RIGHT = VB_SCROLL_AXIS_H | (1 << 2)
};
enum {
    VB_SCROLL_UNIT_PAGE,
    VB_SCROLL_UNIT_LINE     = (1 << 3),
    VB_SCROLL_UNIT_HALFPAGE = (1 << 4)
};

typedef enum {
    VB_MSG_NORMAL,
    VB_MSG_ERROR,
    VB_MSG_LAST
} MessageType;

typedef enum {
    VB_STATUS_NORMAL,
    VB_STATUS_SSL_VALID,
    VB_STATUS_SSL_INVALID,
    VB_STATUS_LAST
} StatusType;

typedef enum {
    VB_CMD_ERROR,               /* command could not be parses or executed */
    VB_CMD_SUCCESS   = 0x01,    /* command runned successfully */
    VB_CMD_KEEPINPUT = 0x02,    /* don't clear inputbox after command run */
} VbCmdResult;

typedef enum {
    VB_COMP_NORMAL,
    VB_COMP_ACTIVE,
    VB_COMP_LAST
} CompletionStyle;

typedef enum {
    FILES_CONFIG,
#ifdef FEATURE_COOKIE
    FILES_COOKIE,
#endif
    FILES_CLOSED,
    FILES_SCRIPT,
    FILES_HISTORY,
    FILES_COMMAND,
    FILES_SEARCH,
    FILES_BOOKMARK,
#ifdef FEATURE_QUEUE
    FILES_QUEUE,
#endif
    FILES_USER_STYLE,
#ifdef FEATURE_HSTS
    FILES_HSTS,
#endif
    FILES_LAST
} VbFile;

enum {
    VB_CLIPBOARD_PRIMARY   = (1<<1),
    VB_CLIPBOARD_SECONDARY = (1<<2)
};

typedef int (*SettingFunction)(const char *name, int type, void *value, void *data);
typedef union {
    gboolean b;
    int      i;
    char     *s;
} SettingValue;

typedef struct {
    const char      *name;
    int             type;
    SettingValue    value;
    SettingFunction setter;
    int             flags;
    void            *data;  /* data given to the setter */
} Setting;

/* structs */
typedef struct {
    int  i;
    char *s;
} Arg;

typedef void (*ModeTransitionFunc) (void);
typedef VbResult (*ModeKeyFunc) (int);
typedef void (*ModeInputChangedFunc) (const char*);
typedef struct {
    char                 id;
    ModeTransitionFunc   enter;         /* is called if the mode is entered */
    ModeTransitionFunc   leave;         /* is called if the mode is left */
    ModeKeyFunc          keypress;      /* receives key to process */
    ModeInputChangedFunc input_changed; /* is triggered if input textbuffer is changed */
#define FLAG_NOMAP       0x0001  /* disables mapping for key strokes */
#define FLAG_HINTING     0x0002  /* marks active hinting submode */
#define FLAG_COMPLETION  0x0004  /* marks active completion submode */
#define FLAG_PASSTHROUGH 0x0008  /* don't handle any other keybind than <esc> */
    unsigned int         flags;
} Mode;

/* statusbar */
typedef struct {
    GtkBox    *box;
    GtkWidget *mode;
    GtkWidget *left;
    GtkWidget *right;
    GtkWidget *cmd;
} StatusBar;

/* gui */
typedef struct {
    GtkWidget          *window;
    WebKitWebView      *webview;
    WebKitWebInspector *inspector;
    GtkBox             *box;
    GtkWidget          *eventbox;
    GtkWidget          *input;
    GtkTextBuffer      *buffer; /* text buffer associated with the input for fast access */
    GtkWidget          *pane;
    StatusBar          statusbar;
    GtkAdjustment      *adjust_h;
    GtkAdjustment      *adjust_v;
} Gui;

/* state */
typedef struct {
    char            *uri;                   /* holds current  uri or the new to open uri */
    guint           progress;
    StatusType      status_type;
    MessageType     input_type;
    gboolean        is_inspecting;
    GList           *downloads;
    gboolean        processed_key;
    char            *title;                 /* holds the window title */
#define PROMPT_SIZE 4
    char            prompt[PROMPT_SIZE];    /* current prompt ':', 'g;t', '/' including nul */
    gdouble         marks[VB_MARK_SIZE];    /* holds marks set to page with 'm{markchar}' */
    char            *linkhover;             /* the uri of the curret hovered link */
    char            *reg[VB_REG_SIZE];      /* holds the yank buffer */
    gboolean        enable_register;        /* indicates if registers are filled */
    char            current_register;       /* holds char for current register to be used */
    gboolean        typed;                  /* indicates if th euser type the keys processed as command */
#ifdef FEATURE_SEARCH_HIGHLIGHT
    int             search_matches;         /* number of matches search results */
#endif
    char            *fifo_path;             /* holds the path to the control fifo */
    char            *socket_path;           /* holds the path to the control socket */
    char            *pid_str;               /* holds the pid as string */
    gboolean        done_loading_page;
    gboolean        window_has_focus;
} State;

typedef struct {
#ifdef FEATURE_COOKIE
    time_t       cookie_timeout;
    int          cookie_expire_time;
#endif
    int          scrollstep;
    char         *download_dir;
    guint        history_max;
    guint        timeoutlen;       /* timeout for ambiguous mappings */
    gboolean     strict_focus;
    GHashTable   *headers;         /* holds user defined header appended to requests */
#ifdef FEATURE_ARH
    GSList       *autoresponseheader; /* holds user defined list of auto-response-header */
#endif
    char         *nextpattern;     /* regex patter nfor prev link matching */
    char         *prevpattern;     /* regex patter nfor next link matching */
    char         *file;            /* path to the custome config file */
    char         *profile;         /* profile name */
    GSList       *cmdargs;         /* list of commands given by --cmd option */
    char         *cafile;          /* path to the ca file */
    GTlsDatabase *tls_db;          /* tls database */
    float        default_zoom;     /* default zoomlevel that is applied on zz zoom reset */
    gboolean     kioskmode;
    gboolean     input_autohide;   /* indicates if the inputbox should be hidden if it's empty */
#ifdef FEATURE_SOCKET
    gboolean     socket;           /* indicates if the socket is used */
#endif
#ifdef FEATURE_HSTS
    HSTSProvider *hsts_provider;   /* the hsts session feature that is added to soup session */
#endif
#ifdef FEATURE_SOUP_CACHE
    SoupCache    *soup_cache;      /* soup caching feature model */
#endif
    GHashTable   *settings;
} Config;

typedef struct {
    VbColor              input_fg[VB_MSG_LAST];
    VbColor              input_bg[VB_MSG_LAST];
    PangoFontDescription *input_font[VB_MSG_LAST];
    /* completion */
    VbColor              comp_fg[VB_COMP_LAST];
    VbColor              comp_bg[VB_COMP_LAST];
    PangoFontDescription *comp_font;
    /* status bar */
    VbColor              status_bg[VB_STATUS_LAST];
    VbColor              status_fg[VB_STATUS_LAST];
    PangoFontDescription *status_font[VB_STATUS_LAST];
} Style;

typedef struct {
    Gui             gui;
    State           state;

    char            *files[FILES_LAST];
    Mode            *mode;
    Config          config;
    Style           style;
    SoupSession     *session;
#ifdef HAS_GTK3
    Window          embed;
#else
    GdkNativeWindow embed;
#endif
    GHashTable      *modes; /* all available browser main modes */
} VbCore;

/* main object */
extern VbCore core;

/* functions */
void vb_add_mode(char id, ModeTransitionFunc enter, ModeTransitionFunc leave,
    ModeKeyFunc keypress, ModeInputChangedFunc input_changed);
void vb_echo_force(const MessageType type,gboolean hide, const char *error, ...);
void vb_echo(const MessageType type, gboolean hide, const char *error, ...);
void vb_enter(char id);
void vb_enter_prompt(char id, const char *prompt, gboolean print_prompt);
VbResult vb_handle_key(int key);
void vb_set_input_text(const char *text);
char *vb_get_input_text(void);
void vb_input_activate(void);
gboolean vb_load_uri(const Arg *arg);
gboolean vb_set_clipboard(const Arg *arg);
void vb_set_widget_font(GtkWidget *widget, const VbColor *fg, const VbColor *bg, PangoFontDescription *font);
void vb_update_statusbar(void);
void vb_update_status_style(void);
void vb_update_input_style(void);
void vb_update_urlbar(const char *uri);
void vb_update_mode_label(const char *label);
void vb_register_add(char buf, const char *value);
const char *vb_register_get(char buf);
gboolean vb_download(WebKitWebView *view, WebKitDownload *download, const char *path);
void vb_quit(gboolean force);

#endif /* end of include guard: _MAIN_H */
