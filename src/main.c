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

#include "main.h"
#include "util.h"
#include "command.h"
#include "keybind.h"
#include "setting.h"
#include "config.h"
#include "completion.h"

/* variables */
VpCore vp;

/* callbacks */
static void vp_webview_load_status_cb(WebKitWebView* view, GParamSpec* pspec, gpointer user_data);
static void vp_destroy_window_cb(GtkWidget* widget, GtkWidget* window, gpointer user_data);
static gboolean vp_frame_scrollbar_policy_changed_cb(void);
static void vp_inputbox_activate_cb(GtkEntry* entry, gpointer user_data);
static gboolean vp_inputbox_keyrelease_cb(GtkEntry* entry, GdkEventKey* event);
static void vp_scroll_cb(GtkAdjustment* adjustment, gpointer data);
#ifdef FEATURE_COOKIE
static void vp_new_request_cb(SoupSession* session, SoupMessage *message, gpointer data);
static void vp_gotheaders_cb(SoupMessage* message, gpointer data);
#endif

/* functions */
static gboolean vp_process_input(const char* input);
static void vp_print_version(void);
static void vp_init(void);
static void vp_read_config(void);
static void vp_init_gui(void);
static void vp_init_files(void);
static void vp_setup_signals(void);
#ifdef FEATURE_COOKIE
static void vp_set_cookie(SoupCookie* cookie);
static const gchar* vp_get_cookies(SoupURI *uri);
#endif
static gboolean vp_hide_message(void);

static void vp_webview_load_status_cb(WebKitWebView* view, GParamSpec* pspec, gpointer user_data)
{
    Gui* gui        = &vp.gui;
    const char* uri = webkit_web_view_get_uri(gui->webview);

    switch (webkit_web_view_get_load_status(gui->webview)) {
        case WEBKIT_LOAD_PROVISIONAL:
            break;

        case WEBKIT_LOAD_COMMITTED:
            {
                Arg a = {VP_MODE_NORMAL};
                vp_set_mode(&a);
            }
            /* status bar is updated by vp_set_mode */
            vp_update_urlbar(uri);
            break;

        case WEBKIT_LOAD_FIRST_VISUALLY_NON_EMPTY_LAYOUT:
            break;

        case WEBKIT_LOAD_FINISHED:
            break;

        case WEBKIT_LOAD_FAILED:
            break;
    }
}

static void vp_destroy_window_cb(GtkWidget* widget, GtkWidget* window, gpointer user_data)
{
    command_close(0);
}

static gboolean vp_frame_scrollbar_policy_changed_cb(void)
{
    return TRUE;
}

static void vp_inputbox_activate_cb(GtkEntry *entry, gpointer user_data)
{
    gboolean success = FALSE;
    const gchar *text;
    guint16 length = gtk_entry_get_text_length(entry);
    Gui* gui = &vp.gui;

    if (0 == length) {
        return;
    }

    gtk_widget_grab_focus(GTK_WIDGET(gui->webview));

    /* do not free or modify text */
    text = gtk_entry_get_text(entry);

    if (1 < length && ':' == text[0]) {
        completion_clean();
        success = vp_process_input((text + 1));
        if (!success) {
            /* switch to normal mode after running command */
            Arg a = {VP_MODE_NORMAL};
            vp_set_mode(&a);
        } else {
            /* switch to normal mode after running command and clean input */
            Arg a = {VP_MODE_NORMAL, ""};
            vp_set_mode(&a);
        }
    }
}

static gboolean vp_inputbox_keyrelease_cb(GtkEntry* entry, GdkEventKey* event)
{
    return FALSE;
}

static void vp_scroll_cb(GtkAdjustment* adjustment, gpointer data)
{       
    vp_update_statusbar();
} 

#ifdef FEATURE_COOKIE
static void vp_new_request_cb(SoupSession* session, SoupMessage *message, gpointer data)
{
    SoupMessageHeaders* header = message->request_headers;
    SoupURI* uri;
    const gchar* cookie;

    soup_message_headers_remove(header, "Cookie");
    uri = soup_message_get_uri(message);
    if ((cookie = vp_get_cookies(uri))) {
        soup_message_headers_append(header, "Cookie", cookie);
    }
    g_signal_connect_after(G_OBJECT(message), "got-headers", G_CALLBACK(vp_gotheaders_cb), NULL);
}

static void vp_gotheaders_cb(SoupMessage* message, gpointer data)
{
    GSList* list = NULL;
    GSList* p = NULL;

    for(p = list = soup_cookies_from_response(message); p; p = g_slist_next(p)) {
        vp_set_cookie((SoupCookie *)p->data);
    }
    soup_cookies_free(list);
}
#endif

/**
 * Processed input from input box without trailing : or ? /, input from config
 * file and default config.
 */
static gboolean vp_process_input(const char* input)
{
    gboolean success;
    gchar* line = NULL;
    gchar* command = NULL;
    gchar** token;

    if (!input || !strlen(input)) {
        return FALSE;
    }

    line = g_strdup(input);
    g_strstrip(line);

    /* get a possible command count */
    vp.state.count = g_ascii_strtoll(line, &command, 10);

    /* split the input string into command and parameter part */
    token = g_strsplit(command, " ", 2);
    g_free(line);

    if (!token[0]) {
        g_strfreev(token);
        return FALSE;
    }
    success = command_run(token[0], token[1] ? token[1] : NULL);
    g_strfreev(token);

    return success;
}

gboolean vp_load_uri(const Arg* arg)
{
    char* uri;
    char* line = arg->s;

    if (!line) {
        return FALSE;
    }

    g_strstrip(line);
    if (!strlen(line)) {
        return FALSE;
    }

    uri = g_strrstr(line, "://") ? g_strdup(line) : g_strdup_printf("http://%s", line);

    /* Load a web page into the browser instance */
    webkit_web_view_load_uri(vp.gui.webview, uri);
    g_free(uri);

    /* change state to normal mode */
    Arg a = {VP_MODE_NORMAL};
    vp_set_mode(&a);

    return TRUE;
}

#ifdef FEATURE_COOKIE
static void vp_set_cookie(SoupCookie* cookie)
{
    SoupDate* date;

    SoupCookieJar* jar = soup_cookie_jar_text_new(vp.files[FILES_COOKIE], FALSE);
    cookie = soup_cookie_copy(cookie);
    if (cookie->expires == NULL && vp.config.cookie_timeout) {
        date = soup_date_new_from_time_t(time(NULL) + vp.config.cookie_timeout);
        soup_cookie_set_expires(cookie, date);
    }
    soup_cookie_jar_add_cookie(jar, cookie);
    g_object_unref(jar);
}

static const gchar* vp_get_cookies(SoupURI *uri)
{
    const gchar* cookie;

    SoupCookieJar* jar = soup_cookie_jar_text_new(vp.files[FILES_COOKIE], TRUE);
    cookie = soup_cookie_jar_get_cookies(jar, uri, TRUE);
    g_object_unref(jar);

    return cookie;
}
#endif

void vp_clean_up(void)
{
    if (vp.behave.commands) {
        g_hash_table_destroy(vp.behave.commands);
        vp.behave.commands = NULL;
    }
    for (int i = FILES_FIRST; i < FILES_LAST; i++) {
        g_free(vp.files[i]);
    }
    command_cleanup();
    setting_cleanup();
    keybind_cleanup();
    completion_clean();
}

static gboolean vp_hide_message(void)
{
    /* do not clean in command mode */
    if (CLEAN_MODE(vp.state.mode) == VP_MODE_COMMAND) {
        return FALSE;
    }

    vp_set_widget_font(
        vp.gui.inputbox,
        &vp.style.input_fg[VP_MSG_NORMAL],
        &vp.style.input_bg[VP_MSG_NORMAL],
        vp.style.input_font[VP_MSG_NORMAL]
    );

    gtk_entry_set_text(GTK_ENTRY(vp.gui.inputbox), "");

    return FALSE;
}

/**
 * Set the base modes. All other mode flags like completion can be set directly
 * to vp.state.mode.
 */
gboolean vp_set_mode(const Arg* arg)
{
    vp.state.mode = arg->i;
    vp.state.modkey = vp.state.count  = 0;

    switch (CLEAN_MODE(arg->i)) {
        case VP_MODE_NORMAL:
            gtk_widget_grab_focus(GTK_WIDGET(vp.gui.webview));
            break;

        case VP_MODE_COMMAND:
            gtk_widget_grab_focus(GTK_WIDGET(vp.gui.inputbox));
            break;

        case VP_MODE_INSERT:
            gtk_widget_grab_focus(GTK_WIDGET(vp.gui.webview));
            break;

        case VP_MODE_PATH_THROUGH:
            gtk_widget_grab_focus(GTK_WIDGET(vp.gui.webview));
            break;
    }

    /* echo message if given */
    if (arg->s) {
        vp_echo(VP_MSG_NORMAL, arg->s);
    }

    vp_update_statusbar();

    return TRUE;
}

void vp_update_urlbar(const gchar* uri)
{
    gtk_label_set_text(GTK_LABEL(vp.gui.statusbar.left), uri);
}

void vp_update_statusbar(void)
{
    GString* status = g_string_new("");

    /* show current count */
    g_string_append_printf(status, "%.0d", vp.state.count);
    /* show current modkey */
    if (vp.state.modkey) {
        g_string_append_c(status, vp.state.modkey);
    }

    /* show the scroll status */
    gint max = gtk_adjustment_get_upper(vp.gui.adjust_v) - gtk_adjustment_get_page_size(vp.gui.adjust_v);
    gint val = (int)(gtk_adjustment_get_value(vp.gui.adjust_v) / max * 100);

    if (max == 0) {
        g_string_append(status, " All");
    } else if (val == 0) {
        g_string_append(status, " Top");
    } else if (val >= 100) {
        g_string_append(status, " Bot");
    } else {
        g_string_append_printf(status, " %d%%", val);
    }

    gtk_label_set_text(GTK_LABEL(vp.gui.statusbar.right), status->str);
    g_string_free(status, TRUE);
}

void vp_echo(const MessageType type, const char *error, ...)
{
    va_list arg_list;

    va_start(arg_list, error);
    char message[255];
    vsnprintf(message, 255, error, arg_list);
    va_end(arg_list);

    /* don't print message if the input is focussed */
    if (gtk_widget_is_focus(GTK_WIDGET(vp.gui.inputbox))) {
        return;
    }

    /* set the collors according to message type */
    vp_set_widget_font(
        vp.gui.inputbox,
        &vp.style.input_fg[type],
        &vp.style.input_bg[type],
        vp.style.input_font[type]
    );
    gtk_entry_set_text(GTK_ENTRY(vp.gui.inputbox), message);
    g_timeout_add_seconds(2, (GSourceFunc)vp_hide_message, NULL);
}

static void vp_print_version(void)
{
    fprintf(stderr, "%s/%s (build %s %s)\n", VERSION, PROJECT, __DATE__, __TIME__);
}

static void vp_init(void)
{
    /* initialize the gui elements and event callbacks */
    vp_init_gui();

    /* initialize the commands hash map */
    command_init();

    /* initialize the config files */
    vp_init_files();

    /* initialize the keybindings */
    keybind_init();

    /* initialize settings */
    setting_init();

    vp_read_config();
}

static void vp_read_config(void)
{
    FILE* fp;
    gchar line[255];

    /* load default config */
    for (guint i = 0; default_config[i].command != NULL; i++) {
        vp_process_input(default_config[i].command);
    }

    /* read config from config files */
    if (access(vp.files[FILES_CONFIG], F_OK) != 0) {
        fprintf(stderr, "Could not find config file");
        return;
    }
    fp = fopen(vp.files[FILES_CONFIG], "r");
    if (fp == NULL) {
        fprintf(stderr, "Could not read config file");
        return;
    }
    while (fgets(line, 254, fp)) {
        if (!g_ascii_isalpha(line[0])) {
            continue;
        }
        if (!vp_process_input(line)) {
            fprintf(stderr, "Invalid config: %s", line);
        }
    }

    fclose(fp);
}

static void vp_init_gui(void)
{
    Gui* gui = &vp.gui;

    gui->sb_h = GTK_SCROLLBAR(gtk_hscrollbar_new(NULL));
    gui->sb_v = GTK_SCROLLBAR(gtk_vscrollbar_new(NULL));
    gui->adjust_h = gtk_range_get_adjustment(GTK_RANGE(gui->sb_h));
    gui->adjust_v = gtk_range_get_adjustment(GTK_RANGE(gui->sb_v));

    GdkGeometry hints = {10, 10};
    gui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_wmclass(GTK_WINDOW(gui->window), PROJECT, PROJECT);
    gtk_window_set_default_size(GTK_WINDOW(gui->window), 640, 480);
    gtk_window_set_title(GTK_WINDOW(gui->window), PROJECT);
    gtk_window_set_geometry_hints(GTK_WINDOW(gui->window), NULL, &hints, GDK_HINT_MIN_SIZE);
    gtk_window_set_icon(GTK_WINDOW(gui->window), NULL);

    /* Create a browser instance */
    gui->webview = WEBKIT_WEB_VIEW(webkit_web_view_new());

    /* init soup session */
#ifdef FEATURE_COOKIE
    vp.net.soup_session = webkit_get_default_session();
    soup_session_remove_feature_by_type(vp.net.soup_session, soup_cookie_jar_get_type());
#endif

    /* Create a scrollable area */
    gui->viewport = gtk_scrolled_window_new(gui->adjust_h, gui->adjust_v);
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(gui->viewport),
        GTK_POLICY_NEVER, GTK_POLICY_NEVER
    );

    gui->box = GTK_BOX(gtk_vbox_new(FALSE, 0));

    /* Prepare the imputbox */
    gui->inputbox = gtk_entry_new();
    gtk_entry_set_inner_border(GTK_ENTRY(gui->inputbox), NULL);
    g_object_set(gtk_widget_get_settings(gui->inputbox), "gtk-entry-select-on-focus", FALSE, NULL);

    /* Prepare the statusbar */
    gui->statusbar.box   = GTK_BOX(gtk_hbox_new(FALSE, 0));
    gui->statusbar.left  = gtk_label_new(NULL);
    gui->statusbar.right = gtk_label_new(NULL);

    /* Prepare the event box */
    gui->eventbox = gtk_event_box_new();

    vp_setup_signals();

    /* Put all part together */
    gtk_container_add(GTK_CONTAINER(gui->viewport), GTK_WIDGET(gui->webview));
    gtk_container_add(GTK_CONTAINER(gui->eventbox), GTK_WIDGET(gui->statusbar.box));
    gtk_container_add(GTK_CONTAINER(gui->window), GTK_WIDGET(gui->box));
    gtk_misc_set_alignment(GTK_MISC(gui->statusbar.left), 0.0, 0.0);
    gtk_misc_set_alignment(GTK_MISC(gui->statusbar.right), 1.0, 0.0);
    gtk_box_pack_start(gui->statusbar.box, gui->statusbar.left, TRUE, TRUE, 2);
    gtk_box_pack_start(gui->statusbar.box, gui->statusbar.right, FALSE, FALSE, 2);

    gtk_box_pack_start(gui->box, gui->viewport, TRUE, TRUE, 0);
    gtk_box_pack_start(gui->box, gui->eventbox, FALSE, FALSE, 0);
    gtk_entry_set_has_frame(GTK_ENTRY(gui->inputbox), FALSE);
    gtk_box_pack_end(gui->box, gui->inputbox, FALSE, FALSE, 0);

    /* Make sure that when the browser area becomes visible, it will get mouse
     * and keyboard events */
    gtk_widget_grab_focus(GTK_WIDGET(gui->webview));

    /* Make sure the main window and all its contents are visible */
    gtk_widget_show_all(gui->window);
}

static void vp_init_files(void)
{
    gchar* path = util_get_config_dir();

    vp.files[FILES_CONFIG] = g_build_filename(path, "config", NULL);
    util_create_file_if_not_exists(vp.files[FILES_CONFIG]);

    vp.files[FILES_COOKIE] = g_build_filename(path, "cookies", NULL);
    util_create_file_if_not_exists(vp.files[FILES_COOKIE]);

    g_free(path);
}

void vp_set_widget_font(GtkWidget* widget, const GdkColor* fg, const GdkColor* bg, PangoFontDescription* font)
{
    gtk_widget_modify_font(widget, font);
    gtk_widget_modify_text(widget, GTK_STATE_NORMAL, fg);
    gtk_widget_modify_base(widget, GTK_STATE_NORMAL, bg);
}

static void vp_setup_signals(void)
{
    Gui* gui              = &vp.gui;
    WebKitWebFrame *frame = webkit_web_view_get_main_frame(gui->webview);

    /* Set up callbacks so that if either the main window or the browser
     * instance is closed, the program will exit */
    g_signal_connect(gui->window, "destroy", G_CALLBACK(vp_destroy_window_cb), NULL);
    g_signal_connect(G_OBJECT(frame), "scrollbars-policy-changed", G_CALLBACK(vp_frame_scrollbar_policy_changed_cb), NULL);
    g_signal_connect(G_OBJECT(gui->webview), "notify::load-status", G_CALLBACK(vp_webview_load_status_cb), NULL);

    g_object_connect(
        G_OBJECT(gui->inputbox),
        "signal::activate",          G_CALLBACK(vp_inputbox_activate_cb),   NULL,
        "signal::key-release-event", G_CALLBACK(vp_inputbox_keyrelease_cb), NULL,
        NULL
    );
    /* webview adjustment */
    g_object_connect(G_OBJECT(vp.gui.adjust_v),
        "signal::value-changed",     G_CALLBACK(vp_scroll_cb),              NULL,
        NULL
    );

#ifdef FEATURE_COOKIE
    g_object_set(vp.net.soup_session, "max-conns", SETTING_MAX_CONNS , NULL);
    g_object_set(vp.net.soup_session, "max-conns-per-host", SETTING_MAX_CONNS_PER_HOST, NULL);
    g_signal_connect_after(G_OBJECT(vp.net.soup_session), "request-started", G_CALLBACK(vp_new_request_cb), NULL);
#endif
}

int main(int argc, char* argv[])
{
    static gboolean ver = false;
    static GError* err;
    static GOptionEntry opts[] = {
        { "version", 'v', 0, G_OPTION_ARG_NONE, &ver, "print version", NULL },
        { NULL }
    };
    /* Initialize GTK+ */
    if (!gtk_init_with_args(&argc, &argv, "[<uri>]", opts, NULL, &err)) {
        g_printerr("can't init gtk: %s\n", err->message);
        g_error_free(err);

        return EXIT_FAILURE;
    }

    if (ver) {
        vp_print_version();
        return EXIT_SUCCESS;
    }
    vp_init();

    /* command line argument: URL */
    Arg arg;
    if (argc > 1) {
        arg.s = g_strdup(argv[argc - 1]);
    } else {
        arg.s = g_strdup(START_PAGE);
    }
    vp_load_uri(&arg);
    g_free(arg.s);

    /* Run the main GTK+ event loop */
    gtk_main();

    return EXIT_SUCCESS;
}
