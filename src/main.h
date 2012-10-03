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
    VP_MODE_NORMAL       = (1 << 0),
    VP_MODE_PATH_THROUGH = (1 << 1),
    VP_MODE_INSERT       = (1 << 2),
} Mode;

enum {
    NAVIG_BACK,
    NAVIG_FORWARD,
    NAVIG_RELOAD,
    NAVIG_RELOAD_FORCE,
    NAVIG_STOP_LOADING
};

/* structs */
typedef struct {
    gint  i;
    char* s;
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
    StatusBar      statusbar;
} Gui;

/* state */
typedef struct {
    Mode          mode;
    gchar         modkey;
    int           count;
} State;

/* behaviour */
typedef struct {
    /* command list: (key)name -> (value)Command  */
    GHashTable* commands;
} Behaviour;

/* core struct */
typedef struct {
    Gui           gui;
    State         state;
    Behaviour     behave;
#if 0
    Network       net;
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
gboolean vp_load_uri(const Arg* arg);
void vp_navigate(const Arg* arg);
void vp_close_browser(const Arg* arg);
void vp_view_source(const Arg* arg);

#endif /* end of include guard: MAIN_H */
