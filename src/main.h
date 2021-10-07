/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2018 Daniel Carl
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

#include <fcntl.h>
#include "config.h"
#ifndef FEATURE_NO_XEMBED
#include <gtk/gtkx.h>
#endif
#include <stdio.h>
#include <webkit2/webkit2.h>
#include "shortcut.h"
#include "handler.h"
#include "file-storage.h"


#define LENGTH(x) (sizeof x / sizeof x[0])
#define OVERWRITE_STRING(t, s) {if (t) g_free(t); t = g_strdup(s);}
#define OVERWRITE_NSTRING(t, s, l) {if (t) {g_free(t); t = NULL;} t = g_strndup(s, l);}
#define GET_CHAR(c, n)  (((Setting*)g_hash_table_lookup(c->config.settings, n))->value.s)
#define GET_INT(c, n)   (((Setting*)g_hash_table_lookup(c->config.settings, n))->value.i)
#define GET_BOOL(c, n)  (((Setting*)g_hash_table_lookup(c->config.settings, n))->value.b)


#ifdef DEBUG
#define PRINT_DEBUG(...) { \
    fprintf(stderr, "\n\033[31;1mDEBUG: \033[32;1m%s +%d %s()\033[0m\t", __FILE__, __LINE__, __func__); \
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

/* the special mark ' must be the first in the list for easiest lookup */
#define MARK_CHARS   "'abcdefghijklmnopqrstuvwxyz"
#define MARK_TICK    0
#define MARK_SIZE    (sizeof(MARK_CHARS) - 1)

#define USER_REG     "abcdefghijklmnopqrstuvwxyz"
/* registers in order displayed for :register command */
#define REG_CHARS    "\"" USER_REG ":%/;"
#define REG_SIZE     (sizeof(REG_CHARS) - 1)

#define FILE_CLOSED  "closed"
#define FILE_COOKIES "cookies"

enum { TARGET_CURRENT, TARGET_RELATED, TARGET_NEW };

typedef enum {
    RESULT_COMPLETE, RESULT_MORE, RESULT_ERROR
} VbResult;

typedef enum {
    CMD_ERROR,               /* command could not be parses or executed */
    CMD_SUCCESS   = 0x01,    /* command runned successfully */
    CMD_KEEPINPUT = 0x02,    /* don't clear inputbox after command run */
} VbCmdResult;

typedef enum {
    TYPE_BOOLEAN, TYPE_INTEGER, TYPE_CHAR, TYPE_COLOR, TYPE_FONT
} DataType;

typedef enum {
    MSG_NORMAL, MSG_ERROR
} MessageType;

typedef enum {
    STATUS_NORMAL, STATUS_SSL_VALID, STATUS_SSL_INVALID
} StatusType;

typedef enum {
    INPUT_UNKNOWN,
    INPUT_SET              = 0x01,
    INPUT_OPEN             = 0x02,
    INPUT_TABOPEN          = 0x04,
    INPUT_COMMAND          = 0x08,
    INPUT_SEARCH_FORWARD   = 0x10,
    INPUT_SEARCH_BACKWARD  = 0x20,
    INPUT_BOOKMARK_ADD     = 0x40,
    INPUT_ALL              = 0xff, /* map to match all input types */
} VbInputType;

enum {
    FILES_BOOKMARK,
    FILES_CLOSED,
    FILES_CONFIG,
    FILES_COOKIE,
    FILES_QUEUE,
    FILES_SCRIPT,
    FILES_USER_STYLE,
    FILES_LAST
};

enum {
    STORAGE_CLOSED,
    STORAGE_COMMAND,
    STORAGE_CONFIG,
    STORAGE_HISTORY,
    STORAGE_SEARCH,
    STORAGE_LAST
};

typedef enum {
    LINK_TYPE_NONE,
    LINK_TYPE_LINK,
    LINK_TYPE_IMAGE,
} VbLinkType;

typedef struct Client Client;
typedef struct State State;
typedef struct Map Map;
typedef struct Mode Mode;
typedef struct Arg Arg;
typedef void (*ModeTransitionFunc)(Client*);
typedef VbResult (*ModeKeyFunc)(Client*, int);
typedef void (*ModeInputChangedFunc)(Client*, const char*);

struct Arg {
    int  i;
    char *s;
};

typedef int (*SettingFunction)(Client *c, const char *name, DataType type, void *value, void *data);
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

struct State {
    char                *uri;
    gboolean            typed;              /* indicates if the user typed the keys */
    gboolean            processed_key;      /* indicates if a key press was handled and should not bubbled up */
    gboolean            ctrlv;              /* indicates if the CTRL-V temorary submode is on */

#define PROMPT_SIZE 4
    char                prompt[PROMPT_SIZE];/* current prompt ':', 'g;t', '/' including nul */
    guint64             marks[MARK_SIZE];   /* holds marks set to page with 'm{markchar}' */
    guint               input_timer;
    MessageType         input_type;
    StatusType          status_type;
    guint64             scroll_max;         /* Maxmimum scrollable height of the document. */
    guint               scroll_percent;     /* Current position of the viewport in document (percent). */
    guint64             scroll_top;         /* Current position of the viewport in document (pixel). */
    char                *title;             /* Window title of the client. */

    char                *reg[REG_SIZE];     /* holds the yank buffers */
    /* TODO rename to reg_{enabled,current} */
    gboolean            enable_register;    /* indicates if registers are filled */
    char                current_register;   /* holds char for current register to be used */

    GList               *downloads;
    guint               progress;
    WebKitHitTestResult *hit_test_result;
    gboolean            is_fullscreen;

    struct {
        gboolean    active;         /* indicate if there is a active search */
        short       direction;      /* last direction 1 forward, -1 backward */
        int         matches;        /* number of matching search results */
        char        *last_query;    /* last search query */
    } search;
};

struct Map {
    char        *in;                /* input keys */
    int         inlen;              /* length of the input keys */
    char        *mapped;            /* mapped keys */
    int         mappedlen;          /* length of the mapped keys string */
    char        mode;               /* mode for which the map is available */
    gboolean    remap;              /* if FALSE do not remap the {rhs} of this map */
};

struct Mode {
    char                 id;
    ModeTransitionFunc   enter;         /* is called if the mode is entered */
    ModeTransitionFunc   leave;         /* is called if the mode is left */
    ModeKeyFunc          keypress;      /* receives key to process */
    ModeInputChangedFunc input_changed; /* is triggered if input textbuffer is changed */
#define FLAG_NOMAP          0x0001  /* disables mapping for key strokes */
#define FLAG_HINTING        0x0002  /* marks active hinting submode */
#define FLAG_COMPLETION     0x0004  /* marks active completion submode */
#define FLAG_PASSTHROUGH    0x0008  /* don't handle any other keybind than <esc> */
#define FLAG_NEW_WIN        0x0010  /* enforce opening of pages into new window */
#define FLAG_IGNORE_FOCUS   0x0012  /* do not listen for focus change messages */
    unsigned int         flags;
};

struct Statusbar {
    GtkBox    *box;
    GtkWidget *mode, *left, *right, *cmd;
};

struct AuGroup;

struct Client {
    struct Client       *next;
    struct State        state;
    struct Statusbar    statusbar;
    void                *comp;                  /* pointer to data used in completion.c */
    Mode                *mode;                  /* current active browser mode */
    /* WebKitWebContext    *webctx; */          /* not used atm, use webkit_web_context_get_default() instead */
    GtkWidget           *window, *input;
    WebKitWebView       *webview;
    WebKitFindController *finder;
    WebKitWebInspector  *inspector;
    guint64             page_id;                /* page id of the webview */
    GtkTextBuffer       *buffer;
    GDBusProxy          *dbusproxy;
    GDBusServer         *dbusserver;
    Handler             *handler;               /* the protocoll handlers */
    struct {
        /* TODO split in global setting definitions and set values on a per
         * client base. */
        GHashTable              *settings;
        guint                   scrollstep;
        guint                   scrollmultiplier;
        gboolean                input_autohide;
        gboolean                incsearch;
        gboolean                prevent_newwindow;
        guint                   default_zoom;   /* default zoom level in percent */
        Shortcut                *shortcuts;
        gboolean                statusbar_show_settings;
    } config;
    struct {
        GSList      *list;
        GString     *queue;                     /* queue holding typed keys */
        int         qlen;                       /* pointer to last char in queue */
        int         resolved;                   /* number of resolved keys (no mapping required) */
        guint       timout_id;                  /* source id of the timeout function */
        char        showcmd[SHOWCMD_LEN + 1];   /* buffer to show ambiguous key sequence */
        guint       timeoutlen;                 /* timeout for ambiguous mappings */
    } map;
    struct {
        struct AuGroup *curgroup;
        GSList         *groups;
        guint          usedbits;                /* holds all used event bits */
    } autocmd;
};

struct Vimb {
    char        *argv0;
    Client      *clients;
#ifndef FEATURE_NO_XEMBED
    Window      embed;
#endif
    GHashTable  *modes;             /* all available browser main modes */
    char        *configfile;        /* config file given as option on startup */
    char        *files[FILES_LAST];
    FileStorage *storage[STORAGE_LAST];
    char        *profile;           /* profile name */
    GSList      *cmdargs;           /* ex commands given asl --command, -C option */
    struct {
        guint   history_max;
        guint   closed_max;
    } config;
    GtkCssProvider *style_provider;
    gboolean    no_maximize;
    gboolean    incognito;

    WebKitWebContext *webcontext;
};

gboolean vb_download_set_destination(Client *c, WebKitDownload *download,
    char *suggested_filename, const char *path);
void vb_echo(Client *c, MessageType type, gboolean hide, const char *error, ...);
void vb_echo_force(Client *c, MessageType type, gboolean hide, const char *error, ...);
void vb_enter(Client *c, char id);
void vb_enter_prompt(Client *c, char id, const char *prompt, gboolean print_prompt);
Client *vb_get_client_for_page_id(guint64 pageid);
char *vb_input_get_text(Client *c);
void vb_input_set_text(Client *c, const char *text);
void vb_input_update_style(Client *c);
gboolean vb_load_uri(Client *c, const Arg *arg);
void vb_mode_add(char id, ModeTransitionFunc enter, ModeTransitionFunc leave,
    ModeKeyFunc keypress, ModeInputChangedFunc input_changed);
VbResult vb_mode_handle_key(Client *c, int key);
void vb_modelabel_update(Client *c, const char *label);
gboolean vb_quit(Client *c, gboolean force);
void vb_register_add(Client *c, char buf, const char *value);
const char *vb_register_get(Client *c, char buf);
void vb_statusbar_update(Client *c);
void vb_statusbar_show_hover_url(Client *c, VbLinkType type, const char *uri);
void vb_gui_style_update(Client *c, const char *name, const char *value);

#endif /* end of include guard: _MAIN_H */
