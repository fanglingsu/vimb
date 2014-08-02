/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2014 Daniel Carl
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

#include "config.h"
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <gdk/gdkx.h>
#include "main.h"
#include "util.h"
#include "command.h"
#include "setting.h"
#include "completion.h"
#include "dom.h"
#include "hints.h"
#include "shortcut.h"
#include "handlers.h"
#include "history.h"
#include "cookiejar.h"
#include "hsts.h"
#include "mode.h"
#include "normal.h"
#include "ex.h"
#include "input.h"
#include "map.h"
#include "pass.h"
#include "bookmark.h"
#include "js.h"

/* variables */
static char **args;
VbCore      vb;

/* callbacks */
#if WEBKIT_CHECK_VERSION(1, 10, 0)
static gboolean context_menu_cb(WebKitWebView *view, GtkWidget *menu,
    WebKitHitTestResult *hitTestResult, gboolean keyboard, gpointer data);
#else
static void context_menu_cb(WebKitWebView *view, GtkMenu *menu, gpointer data);
#endif
static void context_menu_activate_cb(GtkMenuItem *item, gpointer data);
static void uri_change_cb(WebKitWebView *view, GParamSpec param_spec);
static void webview_progress_cb(WebKitWebView *view, GParamSpec *pspec);
static void webview_download_progress_cb(WebKitWebView *view, GParamSpec *pspec);
static void webview_load_status_cb(WebKitWebView *view, GParamSpec *pspec);
static void webview_request_starting_cb(WebKitWebView *view,
    WebKitWebFrame *frame, WebKitWebResource *res, WebKitNetworkRequest *req,
    WebKitNetworkResponse *resp, gpointer data);
static void destroy_window_cb(GtkWidget *widget);
static void scroll_cb(GtkAdjustment *adjustment);
static WebKitWebView *inspector_new(WebKitWebInspector *inspector, WebKitWebView *webview);
static gboolean inspector_show(WebKitWebInspector *inspector);
static gboolean inspector_close(WebKitWebInspector *inspector);
static void inspector_finished(WebKitWebInspector *inspector);
static gboolean button_relase_cb(WebKitWebView *webview, GdkEventButton *event);
static gboolean new_window_policy_cb(
    WebKitWebView *view, WebKitWebFrame *frame, WebKitNetworkRequest *request,
    WebKitWebNavigationAction *navig, WebKitWebPolicyDecision *policy);
static WebKitWebView *create_web_view_cb(WebKitWebView *view, WebKitWebFrame *frame);
static gboolean create_web_view_received_uri_cb(WebKitWebView *view,
    WebKitWebFrame *frame, WebKitNetworkRequest *request,
    WebKitWebNavigationAction *action, WebKitWebPolicyDecision *policy,
    gpointer data);
static gboolean navigation_decision_requested_cb(WebKitWebView *view,
    WebKitWebFrame *frame, WebKitNetworkRequest *request,
    WebKitWebNavigationAction *action, WebKitWebPolicyDecision *policy,
    gpointer data);
static void hover_link_cb(WebKitWebView *webview, const char *title, const char *link);
static void title_changed_cb(WebKitWebView *webview, WebKitWebFrame *frame, const char *title);
static gboolean mimetype_decision_cb(WebKitWebView *webview,
    WebKitWebFrame *frame, WebKitNetworkRequest *request, char*
    mime_type, WebKitWebPolicyDecision *decision);
gboolean vb_download(WebKitWebView *view, WebKitDownload *download, const char *path);
void vb_download_internal(WebKitWebView *view, WebKitDownload *download, const char *file);
void vb_download_external(WebKitWebView *view, WebKitDownload *download, const char *file);
static void download_progress_cp(WebKitDownload *download, GParamSpec *pspec);

/* functions */
#ifdef FEATURE_WGET_PROGRESS_BAR
static void wget_bar(int len, int progress, char *string);
#endif
static void update_title(void);
static void init_core(void);
static void marks_clear(void);
static void read_config(void);
static void setup_signals();
static void init_files(void);
static void session_init(void);
static void session_cleanup(void);
static void register_init(void);
static void register_cleanup(void);
static gboolean hide_message();
static void set_status(const StatusType status);
static void input_print(gboolean force, const MessageType type, gboolean hide, const char *message);

void vb_echo_force(const MessageType type, gboolean hide, const char *error, ...)
{
    char *buffer;
    va_list args;

    va_start(args, error);
    buffer = g_strdup_vprintf(error, args);
    va_end(args);

    input_print(true, type, hide, buffer);
    g_free(buffer);
}

void vb_echo(const MessageType type, gboolean hide, const char *error, ...)
{
    char *buffer;
    va_list args;

    va_start(args, error);
    buffer = g_strdup_vprintf(error, args);
    va_end(args);

    input_print(false, type, hide, buffer);
    g_free(buffer);
}

static void input_print(gboolean force, const MessageType type, gboolean hide,
    const char *message)
{
    /* don't print message if the input is focussed */
    if (!force && gtk_widget_is_focus(GTK_WIDGET(vb.gui.input))) {
        return;
    }

    /* apply input style only if the message type was changed */
    if (type != vb.state.input_type) {
        vb.state.input_type = type;
        vb_update_input_style();
    }
    vb_set_input_text(message);
    if (hide) {
        g_timeout_add_seconds(MESSAGE_TIMEOUT, (GSourceFunc)hide_message, NULL);
    }
}

/**
 * Writes given text into the command line.
 */
void vb_set_input_text(const char *text)
{
    gtk_text_buffer_set_text(vb.gui.buffer, text, -1);
}

/**
 * Retrieves the content of the command line.
 * Returned string must be freed with g_free.
 */
char *vb_get_input_text(void)
{
    GtkTextIter start, end;

    gtk_text_buffer_get_bounds(vb.gui.buffer, &start, &end);
    return gtk_text_buffer_get_text(vb.gui.buffer, &start, &end, false);
}

gboolean vb_load_uri(const Arg *arg)
{
    char *uri = NULL, *rp, *path = NULL;
    struct stat st;

    if (arg->s) {
        path = g_strstrip(arg->s);
    }
    if (!path || !*path) {
        path = GET_CHAR("home-page");
    }

    if (strstr(path, "://") || !strncmp(path, "about:", 6)) {
        uri = g_strdup(path);
    } else if (stat(path, &st) == 0) {
        /* check if the path is a file path */
        rp  = realpath(path, NULL);
        uri = g_strconcat("file://", rp, NULL);
        free(rp);
    } else if (strchr(path, ' ') || !strchr(path, '.')) {
        /* use a shortcut if path contains spaces or no dot */
        uri = shortcut_get_uri(path);
    }

    if (!uri) {
        uri = g_strconcat("http://", path, NULL);
    }

    if (arg->i == VB_TARGET_NEW) {
        guint i = 0;
        char *cmd[9];

        cmd[i++] = *args;
        if (vb.embed) {
            char xid[64];
            snprintf(xid, LENGTH(xid), "%u", (int)vb.embed);
            cmd[i++] = "-e";
            cmd[i++] = xid;
        }
        if (vb.config.file) {
            cmd[i++] = "-c";
            cmd[i++] = vb.config.file;
        }
        if (vb.config.autocmd) {
            cmd[i++] = "-C";
            cmd[i++] = vb.config.autocmd;
        }
        cmd[i++] = uri;
        cmd[i++] = NULL;

        /* spawn a new browser instance */
        g_spawn_async(NULL, cmd, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL);
    } else {
        /* Load a web page into the browser instance */
        webkit_web_view_load_uri(vb.gui.webview, uri);
        /* show the url to be opened in the window title until we receive the
         * page title */
        OVERWRITE_STRING(vb.state.title, uri);
        update_title();
    }
    g_free(uri);

    return true;
}

gboolean vb_set_clipboard(const Arg *arg)
{
    gboolean result = false;
    if (!arg->s) {
        return result;
    }

    if (arg->i & VB_CLIPBOARD_PRIMARY) {
        gtk_clipboard_set_text(PRIMARY_CLIPBOARD(), arg->s, -1);
        result = true;
    }
    if (arg->i & VB_CLIPBOARD_SECONDARY) {
        gtk_clipboard_set_text(SECONDARY_CLIPBOARD(), arg->s, -1);
        result = true;
    }

    return result;
}

void vb_set_widget_font(GtkWidget *widget, const VbColor *fg, const VbColor *bg, PangoFontDescription *font)
{
    VB_WIDGET_OVERRIDE_FONT(widget, font);
    VB_WIDGET_OVERRIDE_TEXT(widget, VB_GTK_STATE_NORMAL, fg);
    VB_WIDGET_OVERRIDE_COLOR(widget, VB_GTK_STATE_NORMAL, fg);
    VB_WIDGET_OVERRIDE_BASE(widget, VB_GTK_STATE_NORMAL, bg);
    VB_WIDGET_OVERRIDE_BACKGROUND(widget, VB_GTK_STATE_NORMAL, bg);
}

#ifdef FEATURE_WGET_PROGRESS_BAR
static void wget_bar(int len, int progress, char *string)
{
    int i, state;

    state = progress * len / 100;
    for (i = 0; i < state; i++) {
        string[i] = PROGRESS_BAR[0];
    }
    string[i++] = PROGRESS_BAR[1];
    for (; i < len; i++) {
        string[i] = PROGRESS_BAR[2];
    }
    string[i] = '\0';
}
#endif

void vb_update_statusbar()
{
    int max, val, num;
    GString *status = g_string_new("");

    /* show the active downloads */
    if (vb.state.downloads) {
        num = g_list_length(vb.state.downloads);
        g_string_append_printf(status, " %d %s", num, num == 1 ? "download" : "downloads");
    }

    /* show load status of page or the downloads */
    if (vb.state.progress != 100) {
#ifdef FEATURE_WGET_PROGRESS_BAR
        char bar[PROGRESS_BAR_LEN + 1];
        wget_bar(PROGRESS_BAR_LEN, vb.state.progress, bar);
        g_string_append_printf(status, " [%s]", bar);
#else
        g_string_append_printf(status, " [%i%%]", vb.state.progress);
#endif
    }

    /* show the scroll status */
    max = gtk_adjustment_get_upper(vb.gui.adjust_v) - gtk_adjustment_get_page_size(vb.gui.adjust_v);
    val = (int)(0.5 + (gtk_adjustment_get_value(vb.gui.adjust_v) / max * 100));

    if (max == 0) {
        g_string_append(status, " All");
    } else if (val == 0) {
        g_string_append(status, " Top");
    } else if (val >= 100) {
        g_string_append(status, " Bot");
    } else {
        g_string_append_printf(status, " %d%%", val);
    }

    gtk_label_set_text(GTK_LABEL(vb.gui.statusbar.right), status->str);
    g_string_free(status, true);
}

void vb_update_status_style(void)
{
    StatusType type = vb.state.status_type;
    vb_set_widget_font(
        vb.gui.eventbox, &vb.style.status_fg[type], &vb.style.status_bg[type], vb.style.status_font[type]
    );
    vb_set_widget_font(
        vb.gui.statusbar.left, &vb.style.status_fg[type], &vb.style.status_bg[type], vb.style.status_font[type]
    );
    vb_set_widget_font(
        vb.gui.statusbar.right, &vb.style.status_fg[type], &vb.style.status_bg[type], vb.style.status_font[type]
    );
    vb_set_widget_font(
        vb.gui.statusbar.cmd, &vb.style.status_fg[type], &vb.style.status_bg[type], vb.style.status_font[type]
    );
}

void vb_update_input_style(void)
{
    MessageType type = vb.state.input_type;
    vb_set_widget_font(
        vb.gui.input, &vb.style.input_fg[type], &vb.style.input_bg[type], vb.style.input_font[type]
    );
}

void vb_update_urlbar(const char *uri)
{
    Gui *gui = &vb.gui;
#ifdef FEATURE_HISTORY_INDICATOR
    gboolean back, fwd;
    char *str;

    back = webkit_web_view_can_go_back(gui->webview);
    fwd  = webkit_web_view_can_go_forward(gui->webview);

    /* show history indicator only if there is something to show */
    if (back || fwd) {
        str = g_strdup_printf("%s [%s]", uri, back ? (fwd ? "-+" : "-") : "+");
        gtk_label_set_text(GTK_LABEL(gui->statusbar.left), str);
        g_free(str);
    } else {
        gtk_label_set_text(GTK_LABEL(gui->statusbar.left), uri);
    }
#else
    gtk_label_set_text(GTK_LABEL(gui->statusbar.left), uri);
#endif
}

void vb_quit(gboolean force)
{
    /* if not forced quit - don't quit if there are still running downloads */
    if (!force && vb.state.downloads) {
        vb_echo_force(VB_MSG_ERROR, true, "Can't quit: there are running downloads");
        return;
    }

    /* write last URL into file for recreation */
    if (vb.state.uri) {
        g_file_set_contents(vb.files[FILES_CLOSED], vb.state.uri, -1, NULL);
    }

    completion_clean();

    webkit_web_view_stop_loading(vb.gui.webview);

    map_cleanup();
    mode_cleanup();
    setting_cleanup();
    history_cleanup();
    session_cleanup();
    register_cleanup();

    for (int i = 0; i < FILES_LAST; i++) {
        g_free(vb.files[i]);
        vb.files[i] = NULL;
    }

    gtk_main_quit();
}

static gboolean hide_message()
{
    input_print(false, VB_MSG_NORMAL, false, "");

    return false;
}

#if WEBKIT_CHECK_VERSION(1, 10, 0)
static gboolean context_menu_cb(WebKitWebView *view, GtkWidget *menu,
    WebKitHitTestResult *hitTestResult, gboolean keyboard, gpointer data)
{
    GList *items = gtk_container_get_children(GTK_CONTAINER(GTK_MENU(menu)));
    for (GList *l = items; l; l = l->next) {
        g_signal_connect(l->data, "activate", G_CALLBACK(context_menu_activate_cb), NULL);
    }
    g_list_free(items);

    return false;
}
#else
static void context_menu_cb(WebKitWebView *view, GtkMenu *menu, gpointer data)
{
    GList *items = gtk_container_get_children(GTK_CONTAINER(GTK_MENU(menu)));
    for (GList *l = items; l; l = l->next) {
        g_signal_connect(l->data, "activate", G_CALLBACK(context_menu_activate_cb), NULL);
    }
    g_list_free(items);
}
#endif

static void context_menu_activate_cb(GtkMenuItem *item, gpointer data)
{
#if WEBKIT_CHECK_VERSION(1, 10, 0)
    WebKitContextMenuAction action = webkit_context_menu_item_get_action(item);
    if (action == WEBKIT_CONTEXT_MENU_ACTION_COPY_LINK_TO_CLIPBOARD) {
        vb_set_clipboard(
            &((Arg){VB_CLIPBOARD_PRIMARY|VB_CLIPBOARD_SECONDARY, vb.state.linkhover})
        );
    }
#else
    const char *name;
    GtkAction *action = gtk_activatable_get_related_action(GTK_ACTIVATABLE(item));

    if (!action) {
        return;
    }

    name = gtk_action_get_name(action);
    /* context-menu-action-3 copy link location */
    if (!g_strcmp0(name, "context-menu-action-3")) {
        vb_set_clipboard(
            &((Arg){VB_CLIPBOARD_PRIMARY|VB_CLIPBOARD_SECONDARY,vb.state.linkhover})
        );
    }
#endif
}

static void uri_change_cb(WebKitWebView *view, GParamSpec param_spec)
{
    g_free(vb.state.uri);
    g_object_get(view, "uri", &vb.state.uri, NULL);
    vb_update_urlbar(vb.state.uri);

    g_setenv("VIMB_URI", vb.state.uri, true);
}

static void webview_progress_cb(WebKitWebView *view, GParamSpec *pspec)
{
    vb.state.progress = webkit_web_view_get_progress(view) * 100;
    vb_update_statusbar();
    update_title();
}

static void webview_download_progress_cb(WebKitWebView *view, GParamSpec *pspec)
{
    if (vb.state.downloads) {
        vb.state.progress = 0;
        GList *ptr;
        for (ptr = vb.state.downloads; ptr; ptr = g_list_next(ptr)) {
            vb.state.progress += 100 * webkit_download_get_progress(ptr->data);
        }
        vb.state.progress /= g_list_length(vb.state.downloads);
    }
    vb_update_statusbar();
    update_title();
}

static void webview_load_status_cb(WebKitWebView *view, GParamSpec *pspec)
{
    const char *uri;

    switch (webkit_web_view_get_load_status(view)) {
        case WEBKIT_LOAD_PROVISIONAL:
            /* update load progress in statusbar */
            vb.state.progress = 0;
            vb_update_statusbar();
            update_title();
            break;

        case WEBKIT_LOAD_COMMITTED:
            uri = webkit_web_view_get_uri(view);
            {
                WebKitWebFrame *frame = webkit_web_view_get_main_frame(view);
                JSContextRef ctx;
                /* set the status */
                if (g_str_has_prefix(uri, "https://")) {
                    WebKitWebDataSource *src      = webkit_web_frame_get_data_source(frame);
                    WebKitNetworkRequest *request = webkit_web_data_source_get_request(src);
                    SoupMessage *msg              = webkit_network_request_get_message(request);
                    SoupMessageFlags flags        = soup_message_get_flags(msg);
                    set_status(
                        (flags & SOUP_MESSAGE_CERTIFICATE_TRUSTED) ? VB_STATUS_SSL_VALID : VB_STATUS_SSL_INVALID
                    );
                } else {
                    set_status(VB_STATUS_NORMAL);
                }

                /* inject the hinting javascript */
                hints_init(frame);

                /* run user script file */
                ctx = webkit_web_frame_get_global_context(frame);
                js_eval_file(ctx, vb.files[FILES_SCRIPT]);
            }

            vb_update_statusbar();
            vb_update_urlbar(uri);
            /* save the current URI in register % */
            vb_register_add('%', uri);

            /* clear possible set marks */
            marks_clear();
            break;

        case WEBKIT_LOAD_FIRST_VISUALLY_NON_EMPTY_LAYOUT:
            /* if we load a page from a submitted form, leave the insert mode */
            if (vb.mode->id == 'i') {
                mode_enter('n');
            }
            break;

        case WEBKIT_LOAD_FINISHED:
            uri = webkit_web_view_get_uri(view);

            /* update load progress in statusbar */
            vb.state.progress = 100;
            vb_update_statusbar();
            update_title();

            if (strncmp(uri, "about:", 6)) {
                dom_check_auto_insert(view);
                history_add(HISTORY_URL, uri, webkit_web_view_get_title(view));
            }
            break;

        case WEBKIT_LOAD_FAILED:
            break;
    }
}

static void webview_request_starting_cb(WebKitWebView *view,
    WebKitWebFrame *frame, WebKitWebResource *res, WebKitNetworkRequest *req,
    WebKitNetworkResponse *resp, gpointer data)
{
    char *name, *value;
    GHashTableIter iter;
    SoupMessage *msg;

    /* don't try to load favicon */
    const char *uri = webkit_network_request_get_uri(req);
    if (g_str_has_suffix(uri, "/favicon.ico")) {
        webkit_network_request_set_uri(req, "about:blank");
        return;
    }

    msg = webkit_network_request_get_message(req);
    if (!msg) {
        return;
    }

#ifdef FEATURE_HSTS
    /* change uri for known and valid hsts hosts */
    hsts_prepare_message(vb.session, msg);
#endif

    if (!vb.config.headers) {
        return;
    }

    /* set/remove/change user defined headers */
    g_hash_table_iter_init(&iter, vb.config.headers);
    while (g_hash_table_iter_next(&iter, (gpointer*)&name, (gpointer*)&value)) {
        /* allow to remove header with null value */
        if (value == NULL) {
            soup_message_headers_remove(msg->request_headers, name);
        } else {
            soup_message_headers_replace(msg->request_headers, name, value);
        }
    }
}

static void destroy_window_cb(GtkWidget *widget)
{
    vb_quit(true);
}

static void scroll_cb(GtkAdjustment *adjustment)
{
    vb_update_statusbar();
}

static WebKitWebView *inspector_new(WebKitWebInspector *inspector, WebKitWebView *webview)
{
    return WEBKIT_WEB_VIEW(webkit_web_view_new());
}

static gboolean inspector_show(WebKitWebInspector *inspector)
{
    WebKitWebView *webview;
    int height;

    if (vb.state.is_inspecting) {
        return false;
    }

    webview = webkit_web_inspector_get_web_view(inspector);

    /* use about 1/3 of window height for the inspector */
    gtk_window_get_size(GTK_WINDOW(vb.gui.window), NULL, &height);
    gtk_paned_set_position(GTK_PANED(vb.gui.pane), 2 * height / 3);

    gtk_paned_pack2(GTK_PANED(vb.gui.pane), GTK_WIDGET(webview), true, true);
    gtk_widget_show(GTK_WIDGET(webview));

    vb.state.is_inspecting = true;

    return true;
}

static gboolean inspector_close(WebKitWebInspector *inspector)
{
    WebKitWebView *webview;

    if (!vb.state.is_inspecting) {
        return false;
    }
    webview = webkit_web_inspector_get_web_view(inspector);
    gtk_widget_hide(GTK_WIDGET(webview));
    gtk_widget_destroy(GTK_WIDGET(webview));

    vb.state.is_inspecting = false;

    return true;
}

static void inspector_finished(WebKitWebInspector *inspector)
{
    g_free(vb.gui.inspector);
}

static void set_status(const StatusType status)
{
    if (vb.state.status_type != status) {
        vb.state.status_type = status;
        /* update the statusbar style only if the status changed */
        vb_update_status_style();
    }
}

static void init_core(void)
{
    Gui *gui = &vb.gui;
    char *xid;

    if (vb.embed) {
        gui->window = gtk_plug_new(vb.embed);
        xid = g_strdup_printf("%u", (int)vb.embed);
    } else {

        gui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
#ifdef HAS_GTK3
        gtk_window_set_has_resize_grip(GTK_WINDOW(gui->window), false);
#endif
        gtk_window_set_wmclass(GTK_WINDOW(gui->window), PROJECT, PROJECT_UCFIRST);
        gtk_window_set_role(GTK_WINDOW(gui->window), PROJECT_UCFIRST);

        gtk_widget_realize(GTK_WIDGET(gui->window));

        /* set the x window id to env */
        xid = g_strdup_printf("%d", (int)GDK_WINDOW_XID(gtk_widget_get_window(GTK_WIDGET(gui->window))));
    }

    g_setenv("VIMB_XID", xid, true);
    g_free(xid);

    GdkGeometry hints = {10, 10};
    gtk_window_set_default_size(GTK_WINDOW(gui->window), WIN_WIDTH, WIN_HEIGHT);
    gtk_window_set_title(GTK_WINDOW(gui->window), PROJECT "/" VERSION);
    gtk_window_set_geometry_hints(GTK_WINDOW(gui->window), NULL, &hints, GDK_HINT_MIN_SIZE);
    gtk_window_set_icon(GTK_WINDOW(gui->window), NULL);
    gtk_widget_set_name(GTK_WIDGET(gui->window), PROJECT);

    /* Create a browser instance */
    gui->webview = WEBKIT_WEB_VIEW(webkit_web_view_new());
    gui->inspector = webkit_web_view_get_inspector(gui->webview);

    /* Create a scrollable area */
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gui->adjust_h = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scroll));
    gui->adjust_v = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scroll));

#ifdef FEATURE_NO_SCROLLBARS
#ifdef HAS_GTK3
    /* set the default style for the application - this can be overwritten by
     * the users style in gtk-3.0/gtk.css */
    const char *style = "GtkScrollbar{-GtkRange-slider-width:0;-GtkRange-trough-border:0;}\
                        GtkScrolledWindow{-GtkScrolledWindow-scrollbar-spacing:0;}";
    GtkCssProvider *provider = gtk_css_provider_get_default();
    gtk_css_provider_load_from_data(provider, style, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
#else /* no GTK3 */
    /* GTK_POLICY_NEVER with gtk3 disallows window resizing and scrolling */
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_NEVER);
#endif
#endif

    /* Prepare the command line */
    gui->input  = gtk_text_view_new();
    gui->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gui->input));

#ifdef HAS_GTK3
    gui->pane            = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    gui->box             = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gui->statusbar.box   = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
#else
    gui->pane            = gtk_vpaned_new();
    gui->box             = GTK_BOX(gtk_vbox_new(false, 0));
    gui->statusbar.box   = GTK_BOX(gtk_hbox_new(false, 0));
#endif
    gui->statusbar.left  = gtk_label_new(NULL);
    gui->statusbar.right = gtk_label_new(NULL);
    gui->statusbar.cmd   = gtk_label_new(NULL);

    /* Prepare the event box */
    gui->eventbox = gtk_event_box_new();

    gtk_paned_pack1(GTK_PANED(gui->pane), GTK_WIDGET(gui->box), true, true);

    /* Put all part together */
    gtk_container_add(GTK_CONTAINER(scroll), GTK_WIDGET(gui->webview));
    gtk_container_add(GTK_CONTAINER(gui->eventbox), GTK_WIDGET(gui->statusbar.box));
    gtk_container_add(GTK_CONTAINER(gui->window), GTK_WIDGET(gui->pane));
    gtk_misc_set_alignment(GTK_MISC(gui->statusbar.left), 0.0, 0.0);
    gtk_misc_set_alignment(GTK_MISC(gui->statusbar.right), 1.0, 0.0);
    gtk_misc_set_alignment(GTK_MISC(gui->statusbar.cmd), 1.0, 0.0);
    gtk_box_pack_start(gui->statusbar.box, gui->statusbar.left, true, true, 2);
    gtk_box_pack_start(gui->statusbar.box, gui->statusbar.cmd, false, false, 0);
    gtk_box_pack_start(gui->statusbar.box, gui->statusbar.right, false, false, 2);

    gtk_box_pack_start(gui->box, scroll, true, true, 0);
    gtk_box_pack_start(gui->box, gui->eventbox, false, false, 0);
    gtk_box_pack_end(gui->box, gui->input, false, false, 0);

    /* init some state variable */
    vb.state.enable_register = false;
    vb.state.enable_history  = false;
    vb.state.uri             = g_strdup("");

    /* initialize the modes */
    mode_init();
    if (!vb.config.kioskmode) {
        mode_add('n', normal_enter, normal_leave, normal_keypress, NULL);
        mode_add('c', ex_enter, ex_leave, ex_keypress, ex_input_changed);
        mode_add('i', input_enter, input_leave, input_keypress, NULL);
        mode_add('p', pass_enter, pass_leave, pass_keypress, NULL);
    } else {
        mode_add('n', NULL, NULL, NULL, NULL);
        mode_add('c', NULL, NULL, NULL, NULL);
        mode_add('i', NULL, NULL, NULL, NULL);
        mode_add('p', NULL, NULL, NULL, NULL);
    }

    /* initialize the marks with empty values */
    marks_clear();

    init_files();
    session_init();
    setting_init();
    register_init();
    read_config();

    /* initially apply input style */
    vb_update_input_style();

    setup_signals();

    /* make sure the main window and all its contents are visible */
    gtk_widget_show_all(gui->window);
    if (vb.config.kioskmode) {
        WebKitWebSettings *setting = webkit_web_view_get_settings(gui->webview);

        /* hide input box - to not create it would be better, but this needs a
         * lot of changes in the code where the input is used */
        gtk_widget_hide(vb.gui.input);

        /* disable context menu */
        g_object_set(G_OBJECT(setting), "enable-default-context-menu", false, NULL);
    }

    /* enter normal mode */
    vb.state.enable_register = true;
    vb.state.enable_history  = true;
    mode_enter('n');

    vb.config.default_zoom = 1.0;

#ifdef FEATURE_HIGH_DPI
    /* fix for high dpi displays */
    GdkScreen *screen = gdk_window_get_screen(gtk_widget_get_window(vb.gui.window));
    gdouble dpi = gdk_screen_get_resolution(screen);
    if (dpi != -1) {
        WebKitWebSettings *setting = webkit_web_view_get_settings(gui->webview);
        webkit_web_view_set_full_content_zoom(gui->webview, true);
        g_object_set(G_OBJECT(setting), "enforce-96-dpi", true, NULL);

        /* calculate the zoom level based on 96 dpi */
        vb.config.default_zoom = dpi/96;

        webkit_web_view_set_zoom_level(gui->webview, vb.config.default_zoom);
    }
#endif
}

static void marks_clear(void)
{
    int i;

    /* init empty marks array */
    for (i = 0; i < VB_MARK_SIZE; i++) {
        vb.state.marks[i] = -1;
    }
}

static void read_config(void)
{
    char *line, **lines;

    /* read config from config files */
    lines = util_get_lines(vb.files[FILES_CONFIG]);

    if (lines) {
        int length = g_strv_length(lines) - 1;
        for (int i = 0; i < length; i++) {
            line = lines[i];
            if (*line == '#') {
                continue;
            }
            if (!ex_run_string(line)) {
                g_warning("Invalid user config: '%s'", line);
            }
        }
    }
    g_strfreev(lines);
}

static void setup_signals()
{
    /* Set up callbacks so that if either the main window or the browser
     * instance is closed, the program will exit */
    g_signal_connect(vb.gui.window, "destroy", G_CALLBACK(destroy_window_cb), NULL);
    g_object_connect(
        G_OBJECT(vb.gui.webview),
#if WEBKIT_CHECK_VERSION(1, 10, 0)
        "signal::context-menu", G_CALLBACK(context_menu_cb), NULL,
#else
        "signal::populate-popup", G_CALLBACK(context_menu_cb), NULL,
#endif
        "signal::notify::uri", G_CALLBACK(uri_change_cb), NULL,
        "signal::notify::progress", G_CALLBACK(webview_progress_cb), NULL,
        "signal::notify::load-status", G_CALLBACK(webview_load_status_cb), NULL,
        "signal::button-release-event", G_CALLBACK(button_relase_cb), NULL,
        "signal::new-window-policy-decision-requested", G_CALLBACK(new_window_policy_cb), NULL,
        "signal::create-web-view", G_CALLBACK(create_web_view_cb), NULL,
        "signal::hovering-over-link", G_CALLBACK(hover_link_cb), NULL,
        "signal::title-changed", G_CALLBACK(title_changed_cb), NULL,
        "signal::mime-type-policy-decision-requested", G_CALLBACK(mimetype_decision_cb), NULL,
        "signal::download-requested", G_CALLBACK(vb_download), NULL,
        "signal::should-show-delete-interface-for-element", G_CALLBACK(gtk_false), NULL,
        "signal::resource-request-starting", G_CALLBACK(webview_request_starting_cb), NULL,
        "signal::navigation-policy-decision-requested", G_CALLBACK(navigation_decision_requested_cb), NULL,
        NULL
    );

#ifdef FEATURE_NO_SCROLLBARS
    WebKitWebFrame *frame = webkit_web_view_get_main_frame(vb.gui.webview);
    g_signal_connect(G_OBJECT(frame), "scrollbars-policy-changed", G_CALLBACK(gtk_true), NULL);
#endif

    if (!vb.config.kioskmode) {
        g_signal_connect(
            G_OBJECT(vb.gui.window), "key-press-event", G_CALLBACK(map_keypress), NULL
        );
        g_signal_connect(
            G_OBJECT(vb.gui.input), "focus-in-event", G_CALLBACK(mode_input_focusin), NULL
        );
        g_object_connect(
            G_OBJECT(vb.gui.buffer),
            "signal::changed", G_CALLBACK(mode_input_changed), NULL,
            NULL
        );

        /* inspector */
        g_object_connect(
            G_OBJECT(vb.gui.inspector),
            "signal::inspect-web-view", G_CALLBACK(inspector_new), NULL,
            "signal::show-window", G_CALLBACK(inspector_show), NULL,
            "signal::close-window", G_CALLBACK(inspector_close), NULL,
            "signal::finished", G_CALLBACK(inspector_finished), NULL,
            NULL
        );
    }

    /* webview adjustment */
    g_object_connect(G_OBJECT(vb.gui.adjust_v),
        "signal::value-changed", G_CALLBACK(scroll_cb), NULL,
        NULL
    );
}

static void init_files(void)
{
    char *path = util_get_config_dir();

    if (vb.config.file) {
        char *rp = realpath(vb.config.file, NULL);
        vb.files[FILES_CONFIG] = g_strdup(rp);
        free(rp);
    } else {
        vb.files[FILES_CONFIG] = g_build_filename(path, "config", NULL);
        util_create_file_if_not_exists(vb.files[FILES_CONFIG]);
    }

#ifdef FEATURE_COOKIE
    vb.files[FILES_COOKIE] = g_build_filename(path, "cookies", NULL);
    util_create_file_if_not_exists(vb.files[FILES_COOKIE]);
#endif

    vb.files[FILES_CLOSED] = g_build_filename(path, "closed", NULL);
    util_create_file_if_not_exists(vb.files[FILES_CLOSED]);

    vb.files[FILES_HISTORY] = g_build_filename(path, "history", NULL);
    util_create_file_if_not_exists(vb.files[FILES_HISTORY]);

    vb.files[FILES_COMMAND] = g_build_filename(path, "command", NULL);
    util_create_file_if_not_exists(vb.files[FILES_COMMAND]);

    vb.files[FILES_SEARCH] = g_build_filename(path, "search", NULL);
    util_create_file_if_not_exists(vb.files[FILES_SEARCH]);

    vb.files[FILES_BOOKMARK] = g_build_filename(path, "bookmark", NULL);
    util_create_file_if_not_exists(vb.files[FILES_BOOKMARK]);

#ifdef FEATURE_QUEUE
    vb.files[FILES_QUEUE] = g_build_filename(path, "queue", NULL);
    util_create_file_if_not_exists(vb.files[FILES_QUEUE]);
#endif
#ifdef FEATURE_HSTS
    vb.files[FILES_HSTS] = g_build_filename(path, "hsts", NULL);
    util_create_file_if_not_exists(vb.files[FILES_HSTS]);
#endif

    vb.files[FILES_SCRIPT] = g_build_filename(path, "scripts.js", NULL);

    vb.files[FILES_USER_STYLE] = g_build_filename(path, "style.css", NULL);

    g_free(path);
}

static void session_init(void)
{
    /* init soup session */
    vb.session = webkit_get_default_session();
    g_object_set(vb.session, "max-conns", SETTING_MAX_CONNS , NULL);
    g_object_set(vb.session, "max-conns-per-host", SETTING_MAX_CONNS_PER_HOST, NULL);
    g_object_set(vb.session, "accept-language-auto", true, NULL);

#ifdef FEATURE_COOKIE
    SoupCookieJar *cookie = cookiejar_new(vb.files[FILES_COOKIE], false);
    soup_session_add_feature(vb.session, SOUP_SESSION_FEATURE(cookie));
    g_object_unref(cookie);
#endif
#ifdef FEATURE_HSTS
    /* create only the session feature - the feature is added in setting.c
     * when the setting hsts=on */
    vb.config.hsts_provider = hsts_provider_new();
#endif
}

static void session_cleanup(void)
{
#ifdef FEATURE_HSTS
    /* remove feature from session and unref the feature to make sure the
     * feature is finalized */
    g_object_unref(vb.config.hsts_provider);
    soup_session_remove_feature_by_type(vb.session, HSTS_TYPE_PROVIDER);
#endif
}

static void register_init(void)
{
    memset(vb.state.reg, 0, sizeof(char*));
}

void vb_register_add(char buf, const char *value)
{
    char *mark;
    int idx;

    if (!vb.state.enable_register || !buf) {
        return;
    }

    /* make sure the mark is a valid mark char */
    if ((mark = strchr(VB_REG_CHARS, buf))) {
        /* get the index of the mark char */
        idx = mark - VB_REG_CHARS;

        OVERWRITE_STRING(vb.state.reg[idx], value);
    }
}

const char *vb_register_get(char buf)
{
    char *mark;
    int idx;

    /* make sure the mark is a valid mark char */
    if ((mark = strchr(VB_REG_CHARS, buf))) {
        /* get the index of the mark char */
        idx = mark - VB_REG_CHARS;

        return vb.state.reg[idx];
    }

    return NULL;
}

static void register_cleanup(void)
{
    int i;
    for (i = 0; i < VB_REG_SIZE; i++) {
        if (vb.state.reg[i]) {
            g_free(vb.state.reg[i]);
        }
    }
}

static gboolean button_relase_cb(WebKitWebView *webview, GdkEventButton *event)
{
    gboolean propagate = false;
    WebKitHitTestResultContext context;

    WebKitHitTestResult *result = webkit_web_view_get_hit_test_result(webview, event);

    g_object_get(result, "context", &context, NULL);
    if (context & WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK) {
        if (event->button == 2 || (event->button == 1 && event->state & GDK_CONTROL_MASK)) {
            /* ctrl click or middle mouse click onto link */
            Arg a = {VB_TARGET_NEW};
            g_object_get(result, "link-uri", &a.s, NULL);
            vb_load_uri(&a);

            propagate = true;
        }
    } else if (context & WEBKIT_HIT_TEST_RESULT_CONTEXT_EDITABLE && vb.mode->id != 'p') {
        /* don't switch back to input mode if we are currently in pass through
         * mode when the user clicks into a form field */
        mode_enter('i');
        propagate = true;
    }
    g_object_unref(result);

    return propagate;
}

static gboolean new_window_policy_cb(
    WebKitWebView *view, WebKitWebFrame *frame, WebKitNetworkRequest *request,
    WebKitWebNavigationAction *navig, WebKitWebPolicyDecision *policy)
{
    if (webkit_web_navigation_action_get_reason(navig) == WEBKIT_WEB_NAVIGATION_REASON_LINK_CLICKED) {
        webkit_web_policy_decision_ignore(policy);
        /* open in a new window */
        Arg a = {VB_TARGET_NEW, (char*)webkit_network_request_get_uri(request)};
        vb_load_uri(&a);
        return true;
    }
    return false;
}

static WebKitWebView *create_web_view_cb(WebKitWebView *view, WebKitWebFrame *frame)
{
    WebKitWebView *new = WEBKIT_WEB_VIEW(webkit_web_view_new());

    /* wait until the new webview receives its new URI */
    g_signal_connect(new, "navigation-policy-decision-requested", G_CALLBACK(create_web_view_received_uri_cb), NULL);

    return new;
}

static gboolean create_web_view_received_uri_cb(WebKitWebView *view,
    WebKitWebFrame *frame, WebKitNetworkRequest *request,
    WebKitWebNavigationAction *action, WebKitWebPolicyDecision *policy,
    gpointer data)
{
    Arg a = {VB_TARGET_NEW, (char*)webkit_network_request_get_uri(request)};
    vb_load_uri(&a);

    /* destroy temporary webview */
    gtk_widget_destroy(GTK_WIDGET(view));

    /* mark that we handled the signal */
    return true;
}

static gboolean navigation_decision_requested_cb(WebKitWebView *view,
    WebKitWebFrame *frame, WebKitNetworkRequest *request,
    WebKitWebNavigationAction *action, WebKitWebPolicyDecision *policy,
    gpointer data)
{
    /* try to find a protocol handler to open the uri */
    return handle_uri(webkit_network_request_get_uri(request));
}

static void hover_link_cb(WebKitWebView *webview, const char *title, const char *link)
{
    char *message;
    if (link) {
        /* save the uri to have this if the user want's to copy the link
         * location via context menu */
        OVERWRITE_STRING(vb.state.linkhover, link);

        message = g_strconcat("Link: ", link, NULL);
        gtk_label_set_text(GTK_LABEL(vb.gui.statusbar.left), message);
        g_free(message);
    } else {
        vb_update_urlbar(webkit_web_view_get_uri(webview));
    }
}

static void title_changed_cb(WebKitWebView *webview, WebKitWebFrame *frame, const char *title)
{
    OVERWRITE_STRING(vb.state.title, title);
    update_title();
    g_setenv("VIMB_TITLE", title ? title : "", true);
}

static void update_title(void)
{
#ifdef FEATURE_TITLE_PROGRESS
    /* show load status of page or the downloads */
    if (vb.state.progress != 100) {
        char *title = g_strdup_printf(
            "[%i%%] %s",
            vb.state.progress,
            vb.state.title ? vb.state.title : ""
        );
        gtk_window_set_title(GTK_WINDOW(vb.gui.window), title);
        g_free(title);
        return;
    }
#endif
    if (vb.state.title) {
        gtk_window_set_title(GTK_WINDOW(vb.gui.window), vb.state.title);
    }
}

static gboolean mimetype_decision_cb(WebKitWebView *webview,
    WebKitWebFrame *frame, WebKitNetworkRequest *request, char *mime_type,
    WebKitWebPolicyDecision *decision)
{
    SoupMessage *msg;
    /* don't start download if request failed or stopped by proxy or can be
     * displayed in the webview */
    if (!mime_type || *mime_type == '\0'
        || webkit_web_view_can_show_mime_type(webview, mime_type)) {

        return false;
    }

    /* don't start a download when the response has no 2xx status code */
    msg = webkit_network_request_get_message(request);
    if (SOUP_STATUS_IS_SUCCESSFUL(msg->status_code)) {
        webkit_web_policy_decision_download(decision);
        return true;
    }
    return false;
}

gboolean vb_download(WebKitWebView *view, WebKitDownload *download, const char *path)
{
    char *file, *dir;
    const char *download_cmd = GET_CHAR("download-command");
    gboolean use_external    = GET_BOOL("download-use-external");

    /* prepare the path to save the download */
    if (path) {
        file = util_build_path(path, vb.config.download_dir);

        /* if file is an directory append a file name */
        if (g_file_test(file, (G_FILE_TEST_IS_DIR))) {
            dir  = file;
            file = g_build_filename(dir, PROJECT "-download", NULL);
            g_free(dir);
        }
    } else {
        /* if there was no path given where to download the file, used
         * suggested file name or a static one */
        path = webkit_download_get_suggested_filename(download);
        if (!path || *path == '\0') {
            path = PROJECT "-download";
        }
        file = util_build_path(path, vb.config.download_dir);
    }

    if (use_external && *download_cmd) {
        /* run download with external program */
        vb_download_external(view, download, file);
        g_free(file);

        /* signalize that we handle the download ourself */
        return false;
    } else {
        /* use webkit download helpr to download the uri */
        vb_download_internal(view, download, file);
        g_free(file);

        return true;
    }
}

void vb_download_internal(WebKitWebView *view, WebKitDownload *download, const char *file)
{
    char *uri;
    guint64 size;
    WebKitDownloadStatus status;

    /* build the file uri from file path */
    uri = g_filename_to_uri(file, NULL, NULL);
    webkit_download_set_destination_uri(download, uri);
    g_free(uri);

    size = webkit_download_get_total_size(download);
    if (size > 0) {
        vb_echo(VB_MSG_NORMAL, false, "Download %s [%uB] started ...", file, size);
    } else {
        vb_echo(VB_MSG_NORMAL, false, "Download %s started ...", file);
    }

    status = webkit_download_get_status(download);
    if (status == WEBKIT_DOWNLOAD_STATUS_CREATED) {
        webkit_download_start(download);
    }

    /* prepend the download to the download list */
    vb.state.downloads = g_list_prepend(vb.state.downloads, download);

    /* connect signal handler to check if the download is done */
    g_signal_connect(download, "notify::status", G_CALLBACK(download_progress_cp), NULL);
    g_signal_connect(download, "notify::progress", G_CALLBACK(webview_download_progress_cb), NULL);

    vb_update_statusbar();
}

void vb_download_external(WebKitWebView *view, WebKitDownload *download, const char *file)
{
    const char *user_agent = NULL, *mimetype = NULL, *download_cmd;
    char **argv, **envp;
    char *cmd;
    int argc;
    guint64 size;
    SoupMessage *msg;
    WebKitNetworkRequest *request;
    GError *error = NULL;

    request = webkit_download_get_network_request(download);
    msg     = webkit_network_request_get_message(request);
    /* if the download is started by the :save command or hinting we get no
     * message here */
    if (msg) {
        user_agent = soup_message_headers_get_one(msg->request_headers, "User-Agent");
        mimetype   = soup_message_headers_get_one(msg->request_headers, "Content-Type");
    }

    /* set the required download information as environment */
    envp = g_get_environ();
    envp = g_environ_setenv(envp, "VIMB_FILE", file, true);
    envp = g_environ_setenv(envp, "VIMB_USE_PROXY", GET_BOOL("proxy") ? "1" : "0", true);
#ifdef FEATURE_COOKIE
    envp = g_environ_setenv(envp, "VIMB_COOKIES", vb.files[FILES_COOKIE], true);
#endif
    if (mimetype) {
        envp = g_environ_setenv(envp, "VIMB_MIME_TYPE", mimetype, true);
    }

    if (!user_agent) {
        WebKitWebSettings *setting = webkit_web_view_get_settings(view);
        g_object_get(G_OBJECT(setting), "user-agent", &user_agent, NULL);
    }
    envp         = g_environ_setenv(envp, "VIMB_USER_AGENT", user_agent, true);
    download_cmd = GET_CHAR("download-command");
    cmd          = g_strdup_printf(download_cmd, webkit_download_get_uri(download));

    if (!g_shell_parse_argv(cmd, &argc, &argv, &error)) {
        g_warning("Could not parse download-command '%s': %s", download_cmd, error->message);
        g_error_free(error);
        g_free(cmd);

        return;
    }
    g_free(cmd);

    if (g_spawn_async(NULL, argv, envp, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error)) {
        size = webkit_download_get_total_size(download);
        if (size > 0) {
            vb_echo(VB_MSG_NORMAL, true, "Download of %uB started", size);
        } else {
            vb_echo(VB_MSG_NORMAL, true, "Download started");
        }
    } else {
        g_warning("%s", error->message);
        g_clear_error(&error);
        vb_echo(VB_MSG_ERROR, true, "Could not start download");
    }
    g_strfreev(argv);
    g_strfreev(envp);
}

static void download_progress_cp(WebKitDownload *download, GParamSpec *pspec)
{
    WebKitDownloadStatus status = webkit_download_get_status(download);

    if (status == WEBKIT_DOWNLOAD_STATUS_STARTED || status == WEBKIT_DOWNLOAD_STATUS_CREATED) {
        return;
    }

    const char *file = webkit_download_get_destination_uri(download);
    /* skip the file protocol for the display */
    if (!strncmp(file, "file://", 7)) {
        file += 7;
    }
    if (status != WEBKIT_DOWNLOAD_STATUS_FINISHED) {
        vb_echo(VB_MSG_ERROR, false, "Error downloading %s", file);
    } else {
        vb_echo(VB_MSG_NORMAL, false, "Download %s finished", file);
    }

    /* remove the download from the list */
    vb.state.downloads = g_list_remove(vb.state.downloads, download);

    vb_update_statusbar();
}

int main(int argc, char *argv[])
{
    static char *winid = NULL;
    static gboolean ver = false;
    static GError *err;
    char *pid;

    static GOptionEntry opts[] = {
        {"cmd", 'C', 0, G_OPTION_ARG_STRING, &vb.config.autocmd, "Ex command run before first page is loaded", NULL},
        {"config", 'c', 0, G_OPTION_ARG_STRING, &vb.config.file, "Custom configuration file", NULL},
        {"embed", 'e', 0, G_OPTION_ARG_STRING, &winid, "Reparents to window specified by xid", NULL},
        {"kiosk", 'k', 0, G_OPTION_ARG_NONE, &vb.config.kioskmode, "Run in kiosk mode", NULL},
        {"version", 'v', 0, G_OPTION_ARG_NONE, &ver, "Print version", NULL},
        {NULL}
    };
    /* Initialize GTK+ */
    if (!gtk_init_with_args(&argc, &argv, "[URI]", opts, NULL, &err)) {
        g_printerr("can't init gtk: %s\n", err->message);
        g_error_free(err);

        return EXIT_FAILURE;
    }

    if (ver) {
        fprintf(stdout, "%s/%s\n", PROJECT, VERSION);
        return EXIT_SUCCESS;
    }

    /* save arguments */
    args = argv;

    if (winid) {
        vb.embed = strtol(winid, NULL, 0);
    }

    pid = g_strdup_printf("%d", getpid());
    g_setenv("VIMB_PID", pid, true);
    g_free(pid);

    init_core();

    /* process the --cmd if this was given */
    if (vb.config.autocmd) {
        ex_run_string(vb.config.autocmd);
    }

    /* command line argument: URL */
    vb_load_uri(&(Arg){VB_TARGET_CURRENT, argc > 1 ? argv[argc - 1] : NULL});

    /* Run the main GTK+ event loop */
    gtk_main();

    return EXIT_SUCCESS;
}
