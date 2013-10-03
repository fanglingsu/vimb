/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2013 Daniel Carl
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
#include <sys/stat.h>
#include <math.h>
#include "main.h"
#include "util.h"
#include "command.h"
#include "setting.h"
#include "completion.h"
#include "dom.h"
#include "hints.h"
#include "shortcut.h"
#include "history.h"
#include "session.h"
#include "mode.h"
#include "normal.h"
#include "ex.h"
#include "input.h"
#include "map.h"
#include "default.h"
#include "pass.h"
#include "bookmark.h"

/* variables */
static char **args;
VbCore      vb;

/* callbacks */
static void webview_progress_cb(WebKitWebView *view, GParamSpec *pspec);
static void webview_download_progress_cb(WebKitWebView *view, GParamSpec *pspec);
static void webview_load_status_cb(WebKitWebView *view, GParamSpec *pspec);
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
static void create_web_view_received_uri_cb(WebKitWebView *view);
static void hover_link_cb(WebKitWebView *webview, const char *title, const char *link);
static void title_changed_cb(WebKitWebView *webview, WebKitWebFrame *frame, const char *title);
static gboolean mimetype_decision_cb(WebKitWebView *webview,
    WebKitWebFrame *frame, WebKitNetworkRequest *request, char*
    mime_type, WebKitWebPolicyDecision *decision);
static void download_progress_cp(WebKitDownload *download, GParamSpec *pspec);
static void request_start_cb(WebKitWebView *webview, WebKitWebFrame *frame,
    WebKitWebResource *resource, WebKitNetworkRequest *request,
    WebKitNetworkResponse *response);

/* functions */
static void run_user_script(WebKitWebFrame *frame);
static char *jsref_to_string(JSContextRef context, JSValueRef ref);
static void init_core(void);
static void read_config(void);
static void setup_signals();
static void init_files(void);
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
 * Retruned string must be freed with g_free.
 */
char *vb_get_input_text(void)
{
    GtkTextIter start, end;

    gtk_text_buffer_get_bounds(vb.gui.buffer, &start, &end);
    return gtk_text_buffer_get_text(vb.gui.buffer, &start, &end, false);
}

gboolean vb_eval_script(WebKitWebFrame *frame, char *script, char *file, char **value)
{
    JSStringRef str, file_name;
    JSValueRef exception = NULL, result = NULL;
    JSContextRef js;

    js        = webkit_web_frame_get_global_context(frame);
    str       = JSStringCreateWithUTF8CString(script);
    file_name = JSStringCreateWithUTF8CString(file);

    result = JSEvaluateScript(js, str, JSContextGetGlobalObject(js), file_name, 0, &exception);
    JSStringRelease(file_name);
    JSStringRelease(str);

    if (result) {
        *value = jsref_to_string(js, result);
        return true;
    }

    *value = jsref_to_string(js, exception);
    return false;
}

gboolean vb_load_uri(const Arg *arg)
{
    char *uri = NULL, *rp, *path = NULL;
    struct stat st;

    if (arg->s) {
        path = g_strstrip(arg->s);
    }
    if (!path || !*path) {
        path = vb.config.home_page;
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
        char *cmd[7], xid[64];

        cmd[i++] = *args;
        if (vb.embed) {
            cmd[i++] = "-e";
            snprintf(xid, LENGTH(xid), "%u", (int)vb.embed);
            cmd[i++] = xid;
        }
        if (vb.custom_config) {
            cmd[i++] = "-c";
            cmd[i++] = vb.custom_config;
        }
        cmd[i++] = uri;
        cmd[i++] = NULL;

        /* spawn a new browser instance */
        g_spawn_async(NULL, cmd, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL);
    } else {
        /* Load a web page into the browser instance */
        webkit_web_view_load_uri(vb.gui.webview, uri);
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
        g_string_append_printf(status, " [%i%%]", vb.state.progress);
    }

    /* show the scroll status */
    max = gtk_adjustment_get_upper(vb.gui.adjust_v) - gtk_adjustment_get_page_size(vb.gui.adjust_v);
    val = (int)floor(0.5 + (gtk_adjustment_get_value(vb.gui.adjust_v) / max * 100));

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
    gtk_label_set_text(GTK_LABEL(vb.gui.statusbar.left), uri);
}

/**
 * Analyzes the given input string for known prefixes (':set ', ':open ', '/',
 * ...) and set the given prefix pointer to the found prefix and the given
 * suffix pointer to the suffix.
 */
VbInputType vb_get_input_parts(const char* input, unsigned int use,
    const char **prefix, const char **clean)
{
    static const struct {
        VbInputType type;
        const char *prefix;
        unsigned int len;
    } types[] = {
        {VB_INPUT_OPEN, ":o ", 3},
        {VB_INPUT_TABOPEN, ":t ", 3},
        {VB_INPUT_OPEN, ":open ", 6},
        {VB_INPUT_TABOPEN, ":tabopen ", 9},
        {VB_INPUT_SET, ":set ", 5},
        {VB_INPUT_BOOKMARK_ADD, ":bma ", 5},
        {VB_INPUT_BOOKMARK_ADD, ":bookmark-add ", 14},
        {VB_INPUT_COMMAND, ":", 1},
        {VB_INPUT_SEARCH_FORWARD, "/", 1},
        {VB_INPUT_SEARCH_BACKWARD, "?", 1},
    };
    for (unsigned int i = 0; i < LENGTH(types); i++) {
        /* process only those types given with use */
        if (!(types[i].type & use)) {
            continue;
        }
        if (!strncmp(input, types[i].prefix, types[i].len)) {
            *prefix = types[i].prefix;
            *clean  = input + types[i].len;
            return types[i].type;
        }
    }

    *prefix = NULL;
    *clean  = input;
    return VB_INPUT_UNKNOWN;
}

void vb_quit(void)
{
    const char *uri = GET_URI();
    /* write last URL into file for recreation */
    if (uri) {
        g_file_set_contents(vb.files[FILES_CLOSED], uri, -1, NULL);
    }

    completion_clean();

    webkit_web_view_stop_loading(vb.gui.webview);

    map_cleanup();
    mode_cleanup();
    setting_cleanup();
    shortcut_cleanup();
    history_cleanup();

    for (int i = 0; i < FILES_LAST; i++) {
        g_free(vb.files[i]);
    }

    gtk_main_quit();
}

static gboolean hide_message()
{
    input_print(false, VB_MSG_NORMAL, false, "");

    return false;
}

static void webview_progress_cb(WebKitWebView *view, GParamSpec *pspec)
{
    vb.state.progress = webkit_web_view_get_progress(view) * 100;
    vb_update_statusbar();
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
}

static void webview_load_status_cb(WebKitWebView *view, GParamSpec *pspec)
{
    const char *uri = GET_URI();

    switch (webkit_web_view_get_load_status(vb.gui.webview)) {
        case WEBKIT_LOAD_PROVISIONAL:
            /* update load progress in statusbar */
            vb.state.progress = 0;
            vb_update_statusbar();
            break;

        case WEBKIT_LOAD_COMMITTED:
            {
                WebKitWebFrame *frame = webkit_web_view_get_main_frame(vb.gui.webview);
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
                run_user_script(frame);
            }

            /* if we load a page from a submitted form, leafe the insert mode */
            if (vb.mode->id == 'i') {
                mode_enter('n');
            }

            vb_update_statusbar();
            vb_update_urlbar(uri);

            break;

        case WEBKIT_LOAD_FIRST_VISUALLY_NON_EMPTY_LAYOUT:
            break;

        case WEBKIT_LOAD_FINISHED:
            /* update load progress in statusbar */
            vb.state.progress = 100;
            vb_update_statusbar();

            if (strncmp(uri, "about:", 6)) {
                dom_check_auto_insert(view);
                history_add(HISTORY_URL, uri, webkit_web_view_get_title(view));
            }
            break;

        case WEBKIT_LOAD_FAILED:
            break;
    }
}

static void destroy_window_cb(GtkWidget *widget)
{
    vb_quit();
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

static void run_user_script(WebKitWebFrame *frame)
{
    char *js = NULL, *value = NULL;
    GError *error = NULL;

    if (g_file_test(vb.files[FILES_SCRIPT], G_FILE_TEST_IS_REGULAR)
        && g_file_get_contents(vb.files[FILES_SCRIPT], &js, NULL, &error)
    ) {
        gboolean success = vb_eval_script(frame, js, vb.files[FILES_SCRIPT], &value);
        if (!success) {
            fprintf(stderr, "%s", value);
        }
        g_free(value);
        g_free(js);
    }
}

static char *jsref_to_string(JSContextRef context, JSValueRef ref)
{
    char *string;
    JSStringRef str_ref = JSValueToStringCopy(context, ref, NULL);
    size_t len          = JSStringGetMaximumUTF8CStringSize(str_ref);

    string = g_new0(char, len);
    JSStringGetUTF8CString(str_ref, string, len);
    JSStringRelease(str_ref);

    return string;
}

static void init_core(void)
{
    Gui *gui = &vb.gui;

    if (vb.embed) {
        gui->window = gtk_plug_new(vb.embed);
    } else {
        gui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
#ifdef HAS_GTK3
        gtk_window_set_has_resize_grip(GTK_WINDOW(gui->window), false);
#endif
        gtk_window_set_wmclass(GTK_WINDOW(gui->window), PROJECT, PROJECT_UCFIRST);
        gtk_window_set_role(GTK_WINDOW(gui->window), PROJECT_UCFIRST);
    }

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
    gui->scroll = gtk_scrolled_window_new(NULL, NULL);
    gui->adjust_h = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(gui->scroll));
    gui->adjust_v = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(gui->scroll));

#ifdef FEATURE_NO_SCROLLBARS
#ifdef HAS_GTK3
    /* set the default style for the application - this can be overwritten by
     * the users style in gtk-3.0/gtk.css */
    char *style = "GtkScrollbar{-GtkRange-slider-width:0;-GtkRange-trough-border:0;}";
    GtkCssProvider *provider = gtk_css_provider_get_default();
    gtk_css_provider_load_from_data(provider, style, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
#else /* no GTK3 */
    /* GTK_POLICY_NEVER with gtk3 disallows window resizing and scrolling */
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gui->scroll), GTK_POLICY_NEVER, GTK_POLICY_NEVER);
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
    gtk_widget_show_all(gui->window);

    /* Put all part together */
    gtk_container_add(GTK_CONTAINER(gui->scroll), GTK_WIDGET(gui->webview));
    gtk_container_add(GTK_CONTAINER(gui->eventbox), GTK_WIDGET(gui->statusbar.box));
    gtk_container_add(GTK_CONTAINER(gui->window), GTK_WIDGET(gui->pane));
    gtk_misc_set_alignment(GTK_MISC(gui->statusbar.left), 0.0, 0.0);
    gtk_misc_set_alignment(GTK_MISC(gui->statusbar.right), 1.0, 0.0);
    gtk_misc_set_alignment(GTK_MISC(gui->statusbar.cmd), 1.0, 0.0);
    gtk_box_pack_start(gui->statusbar.box, gui->statusbar.left, true, true, 2);
    gtk_box_pack_start(gui->statusbar.box, gui->statusbar.cmd, false, false, 0);
    gtk_box_pack_start(gui->statusbar.box, gui->statusbar.right, false, false, 2);

    gtk_box_pack_start(gui->box, gui->scroll, true, true, 0);
    gtk_box_pack_start(gui->box, gui->eventbox, false, false, 0);
    gtk_box_pack_end(gui->box, gui->input, false, false, 0);

    setup_signals();

    /* Make sure that when the browser area becomes visible, it will get mouse
     * and keyboard events */
    gtk_widget_grab_focus(GTK_WIDGET(gui->webview));

    /* initialize the modes */
    mode_init();
    mode_add('n', normal_enter, normal_leave, normal_keypress, NULL);
    mode_add('c', ex_enter, ex_leave, ex_keypress, ex_input_changed);
    mode_add('i', input_enter, input_leave, input_keypress, NULL);
    mode_add('p', pass_enter, pass_leave, pass_keypress, NULL);
    mode_enter('n');

    init_files();
    session_init();
    setting_init();
    shortcut_init();
    read_config();

    /* initially apply input style */
    vb_update_input_style();
    /* make sure the main window and all its contents are visible */
    gtk_widget_show_all(gui->window);
}

static void read_config(void)
{
    char *line, **lines;

    /* load default config */
    for (guint i = 0; default_config[i] != NULL; i++) {
        if (!ex_run_string(default_config[i])) {
            fprintf(stderr, "Invalid default config: %s\n", default_config[i]);
        }
    }

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
                fprintf(stderr, "Invalid config: %s\n", line);
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
        "signal::notify::progress", G_CALLBACK(webview_progress_cb), NULL,
        "signal::notify::load-status", G_CALLBACK(webview_load_status_cb), NULL,
        "signal::button-release-event", G_CALLBACK(button_relase_cb), NULL,
        "signal::new-window-policy-decision-requested", G_CALLBACK(new_window_policy_cb), NULL,
        "signal::create-web-view", G_CALLBACK(create_web_view_cb), NULL,
        "signal::hovering-over-link", G_CALLBACK(hover_link_cb), NULL,
        "signal::title-changed", G_CALLBACK(title_changed_cb), NULL,
        "signal::mime-type-policy-decision-requested", G_CALLBACK(mimetype_decision_cb), NULL,
        "signal::download-requested", G_CALLBACK(vb_download), NULL,
        "signal::resource-request-starting", G_CALLBACK(request_start_cb), NULL,
        "signal::should-show-delete-interface-for-element", G_CALLBACK(gtk_false), NULL,
        NULL
    );

#ifdef FEATURE_NO_SCROLLBARS
    WebKitWebFrame *frame = webkit_web_view_get_main_frame(vb.gui.webview);
    g_signal_connect(G_OBJECT(frame), "scrollbars-policy-changed", G_CALLBACK(gtk_true), NULL);
#endif

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

    /* webview adjustment */
    g_object_connect(G_OBJECT(vb.gui.adjust_v),
        "signal::value-changed", G_CALLBACK(scroll_cb), NULL,
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

static void init_files(void)
{
    char *path = util_get_config_dir();

    if (vb.custom_config) {
        char *rp = realpath(vb.custom_config, NULL);
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

    vb.files[FILES_SCRIPT] = g_build_filename(path, "scripts.js", NULL);

    vb.files[FILES_USER_STYLE] = g_build_filename(path, "style.css", NULL);

    g_free(path);
}

static gboolean button_relase_cb(WebKitWebView *webview, GdkEventButton *event)
{
    gboolean propagate = false;
    WebKitHitTestResultContext context;

    WebKitHitTestResult *result = webkit_web_view_get_hit_test_result(webview, event);

    g_object_get(result, "context", &context, NULL);
    /* TODO move this to normal.c */
    if (vb.mode->id == 'n' && context & WEBKIT_HIT_TEST_RESULT_CONTEXT_EDITABLE) {
        mode_enter('i');
        propagate = true;
    } else if (context & WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK && event->button == 2) {
        /* middle mouse click onto link */
        Arg a = {VB_TARGET_NEW};
        g_object_get(result, "link-uri", &a.s, NULL);
        vb_load_uri(&a);

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
    g_signal_connect(new, "notify::uri", G_CALLBACK(create_web_view_received_uri_cb), NULL);

    return new;
}

static void create_web_view_received_uri_cb(WebKitWebView *view)
{
    Arg a = {VB_TARGET_NEW, (char*)webkit_web_view_get_uri(view)};
    /* destroy temporary webview */
    webkit_web_view_stop_loading(view);
    gtk_widget_destroy(GTK_WIDGET(view));
    vb_load_uri(&a);
}

static void hover_link_cb(WebKitWebView *webview, const char *title, const char *link)
{
    char *message;
    if (link) {
        message = g_strconcat("Link: ", link, NULL);
        gtk_label_set_text(GTK_LABEL(vb.gui.statusbar.left), message);
        g_free(message);
    } else {
        vb_update_urlbar(webkit_web_view_get_uri(webview));
    }
}

static void title_changed_cb(WebKitWebView *webview, WebKitWebFrame *frame, const char *title)
{
    gtk_window_set_title(GTK_WINDOW(vb.gui.window), title);
}

static gboolean mimetype_decision_cb(WebKitWebView *webview,
    WebKitWebFrame *frame, WebKitNetworkRequest *request, char *mime_type,
    WebKitWebPolicyDecision *decision)
{
    if (webkit_web_view_can_show_mime_type(webview, mime_type) == false) {
        webkit_web_policy_decision_download(decision);

        return true;
    }
    return false;
}

gboolean vb_download(WebKitWebView *view, WebKitDownload *download, const char *path)
{
    WebKitDownloadStatus status;
    char *uri, *file;

    /* prepare the path to save the donwload */
    if (path) {
        file = util_build_path(path, vb.config.download_dir);
    } else {
        path = webkit_download_get_suggested_filename(download);
        if (!path) {
            path = PROJECT "-donwload";
        }
        file = util_build_path(path, vb.config.download_dir);
    }

    /* build the file uri from file path */
    uri = g_filename_to_uri(file, NULL, NULL);
    webkit_download_set_destination_uri(download, uri);
    g_free(file);
    g_free(uri);

    guint64 size = webkit_download_get_total_size(download);
    if (size > 0) {
        vb_echo(VB_MSG_NORMAL, false, "Download %s [~%uB] started ...", path, size);
    } else {
        vb_echo(VB_MSG_NORMAL, false, "Download %s started ...", path);
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

    return true;
}

/**
 * Callback to filter started resource request.
 */
static void request_start_cb(WebKitWebView *webview, WebKitWebFrame *frame,
    WebKitWebResource *resource, WebKitNetworkRequest *request,
    WebKitNetworkResponse *response)
{
    const char *uri = webkit_network_request_get_uri(request);
    if (g_str_has_suffix(uri, "/favicon.ico")) {
        webkit_network_request_set_uri(request, "about:blank");
    }
}

static void download_progress_cp(WebKitDownload *download, GParamSpec *pspec)
{
    WebKitDownloadStatus status = webkit_download_get_status(download);

    if (status == WEBKIT_DOWNLOAD_STATUS_STARTED || status == WEBKIT_DOWNLOAD_STATUS_CREATED) {
        return;
    }

    const char *file = webkit_download_get_destination_uri(download);
    /* skip the file protocoll for the display */
    if (!strncmp(file, "file://", 7)) {
        file += 7;
    }
    if (status != WEBKIT_DOWNLOAD_STATUS_FINISHED) {
        vb_echo(VB_MSG_ERROR, false, "Error downloading %s", file);
    } else {
        vb_echo(VB_MSG_NORMAL, false, "Download %s finished", file);
    }

    /* remove the donwload from the list */
    vb.state.downloads = g_list_remove(vb.state.downloads, download);

    vb_update_statusbar();
}

int main(int argc, char *argv[])
{
    static char *winid = NULL;
    static gboolean ver = false, dump = false;
    static GError *err;

    vb.custom_config = NULL;
    static GOptionEntry opts[] = {
        {"version", 'v', 0, G_OPTION_ARG_NONE, &ver, "Print version", NULL},
        {"config", 'c', 0, G_OPTION_ARG_STRING, &vb.custom_config, "Custom cufiguration file", NULL},
        {"embed", 'e', 0, G_OPTION_ARG_STRING, &winid, "Reparents to window specified by xid", NULL},
        {"dump-config", 'd', 0, G_OPTION_ARG_NONE, &dump, "Dump out the default configuration to stdout", NULL},
        {NULL}
    };
    /* Initialize GTK+ */
    if (!gtk_init_with_args(&argc, &argv, "[URI]", opts, NULL, &err)) {
        g_printerr("can't init gtk: %s\n", err->message);
        g_error_free(err);

        return EXIT_FAILURE;
    }

    if (ver) {
        fprintf(stdout, "%s/%s\n", PROJECT, FULL_VERSION);
        return EXIT_SUCCESS;
    }
    if (dump) {
        /* load default config */
        for (guint i = 0; default_config[i] != NULL; i++) {
            fprintf(stdout, "%s\n", default_config[i]);
        }
        return EXIT_SUCCESS;
    }

    /* save arguments */
    args = argv;

    if (winid) {
        vb.embed = strtol(winid, NULL, 0);
    }

    init_core();

    /* command line argument: URL */
    vb_load_uri(&(Arg){VB_TARGET_CURRENT, argc > 1 ? argv[argc - 1] : vb.config.home_page});

    /* Run the main GTK+ event loop */
    gtk_main();

    return EXIT_SUCCESS;
}
