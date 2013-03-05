/**
 * vimp - a webkit based vim like browser.
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

#include "main.h"
#include "util.h"
#include "command.h"
#include "keybind.h"
#include "setting.h"
#include "config.h"
#include "completion.h"
#include "dom.h"
#include "hints.h"
#include "searchengine.h"
#include "history.h"
#include "url_history.h"

/* variables */
static char **args;
VpCore      core;
Client* clients = NULL;

/* callbacks */
static void vp_webview_progress_cb(WebKitWebView* view, GParamSpec* pspec, Client* c);
static void vp_webview_download_progress_cb(WebKitWebView* view, GParamSpec* pspec, Client* c);
static void vp_webview_load_status_cb(WebKitWebView* view, GParamSpec* pspec, Client* c);
static void vp_destroy_window_cb(GtkWidget* widget, Client* c);
static void vp_inputbox_activate_cb(GtkEntry* entry, Client* c);
static gboolean vp_inputbox_keyrelease_cb(GtkEntry* entry, GdkEventKey* event, Client* c);
static void vp_scroll_cb(GtkAdjustment* adjustment, Client* c);
static void vp_new_request_cb(SoupSession* session, SoupMessage *message, Client* c);
static void vp_gotheaders_cb(SoupMessage* message, Client* c);
static WebKitWebView* vp_inspector_new(WebKitWebInspector* inspector, WebKitWebView* webview, Client* c);
static gboolean vp_inspector_show(WebKitWebInspector* inspector, Client* c);
static gboolean vp_inspector_close(WebKitWebInspector* inspector, Client* c);
static void vp_inspector_finished(WebKitWebInspector* inspector, Client* c);
static gboolean vp_button_relase_cb(WebKitWebView *webview, GdkEventButton* event, Client* c);
static gboolean vp_new_window_policy_cb(
    WebKitWebView* view, WebKitWebFrame* frame, WebKitNetworkRequest* request,
    WebKitWebNavigationAction* navig, WebKitWebPolicyDecision* policy, Client* c);
static WebKitWebView* vp_create_new_webview_cb(WebKitWebView* webview, WebKitWebFrame* frame, Client* c);
static void vp_hover_link_cb(WebKitWebView* webview, const char* title, const char* link, Client* c);
static void vp_title_changed_cb(WebKitWebView* webview, WebKitWebFrame* frame, const char* title, Client* c);
static gboolean vp_mimetype_decision_cb(WebKitWebView* webview,
    WebKitWebFrame* frame, WebKitNetworkRequest* request, char*
    mime_type, WebKitWebPolicyDecision* decision, Client* c);
static gboolean vp_download_requested_cb(WebKitWebView* view, WebKitDownload* download, Client* c);
static void vp_download_progress_cp(WebKitDownload* download, GParamSpec* pspec, Client* c);
static void vp_request_start_cb(WebKitWebView* webview, WebKitWebFrame* frame,
    WebKitWebResource* resource, WebKitNetworkRequest* request,
    WebKitNetworkResponse* response, Client* c);

/* functions */
static gboolean vp_process_input(Client* c, const char* input);
static void vp_run_user_script(WebKitWebFrame* frame);
static char* vp_jsref_to_string(JSContextRef context, JSValueRef ref);
static void vp_init_core(void);
static void vp_read_global_config(void);
static void vp_process_config_file(Client* c, VpFile file);
static Client* vp_client_new(void);
static void vp_setup_signals(Client* c);
static void vp_init_files(void);
static void vp_set_cookie(SoupCookie* cookie);
static const char* vp_get_cookies(SoupURI *uri);
static gboolean vp_hide_message(Client* c);
static void vp_set_status(Client* c, const StatusType status);
static void vp_destroy_client(Client* c);
static void vp_clean_up(void);

void vp_clean_input(Client* c)
{
    /* move focus from input box to clean it */
    gtk_widget_grab_focus(GTK_WIDGET(c->gui.webview));
    vp_echo(c, VP_MSG_NORMAL, FALSE, "");
}

void vp_echo(Client* c, const MessageType type, gboolean hide, const char *error, ...)
{
    va_list arg_list;

    va_start(arg_list, error);
    char message[255];
    vsnprintf(message, 255, error, arg_list);
    va_end(arg_list);

    /* don't print message if the input is focussed */
    if (gtk_widget_is_focus(GTK_WIDGET(c->gui.inputbox))) {
        return;
    }

    vp_update_input_style(c, type);
    gtk_entry_set_text(GTK_ENTRY(c->gui.inputbox), message);
    if (hide) {
        g_timeout_add_seconds(MESSAGE_TIMEOUT, (GSourceFunc)vp_hide_message, c);
    }
}

void vp_eval_script(WebKitWebFrame* frame, char* script, char* file, char** value, char** error)
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
        *value = vp_jsref_to_string(js, result);
    } else {
        *error = vp_jsref_to_string(js, exception);
    }
}

gboolean vp_load_uri(Client* c, const Arg* arg)
{
    char* uri;
    char* path = arg->s;

    if (!path) {
        return FALSE;
    }

    g_strstrip(path);
    if (!strlen(path)) {
        return FALSE;
    }

    /* check if the path is a file path */
    if (path[0] == '/' || !strcspn(path, "./")) {
        char* rp = realpath(path, NULL);
        uri = g_strdup_printf("file://%s", rp);
    } else {
        uri = g_strrstr(path, "://") ? g_strdup(path) : g_strdup_printf("http://%s", path);
    }

    /* change state to normal mode */
    vp_set_mode(c, VP_MODE_NORMAL, FALSE);

    if (arg->i == VP_TARGET_NEW) {
        Client* new = vp_client_new();
        webkit_web_view_load_uri(new->gui.webview, uri);
    } else {
        /* Load a web page into the browser instance */
        webkit_web_view_load_uri(c->gui.webview, uri);
    }
    g_free(uri);

    return TRUE;
}

gboolean vp_set_clipboard(const Arg* arg)
{
    gboolean result = FALSE;
    if (!arg->s) {
        return result;
    }

    if (arg->i & VP_CLIPBOARD_PRIMARY) {
        gtk_clipboard_set_text(PRIMARY_CLIPBOARD(), arg->s, -1);
        result = TRUE;
    }
    if (arg->i & VP_CLIPBOARD_SECONDARY) {
        gtk_clipboard_set_text(SECONDARY_CLIPBOARD(), arg->s, -1);
        result = TRUE;
    }

    return result;
}

/**
 * Set the base modes. All other mode flags like completion can be set directly
 * to c->state.mode.
 */
gboolean vp_set_mode(Client* c, Mode mode, gboolean clean)
{
    if (!c) {
        return FALSE;
    }
    if ((c->state.mode & VP_MODE_COMPLETE)
        && !(mode & VP_MODE_COMPLETE)
    ) {
        completion_clean(c);
    }
    int clean_mode = CLEAN_MODE(c->state.mode);
    switch (CLEAN_MODE(mode)) {
        case VP_MODE_NORMAL:
            /* do this only if the mode is really switched */
            if (clean_mode != VP_MODE_NORMAL) {
                history_rewind(c);
            }
            if (clean_mode == VP_MODE_HINTING) {
                /* if previous mode was hinting clear the hints */
                hints_clear(c);
            } else if (clean_mode == VP_MODE_INSERT) {
                /* clean the input if current mode is insert to remove -- INPUT -- */
                clean = TRUE;
            } else if (clean_mode == VP_MODE_SEARCH) {
                /* cleaup previous search */
                command_search(c, &((Arg){VP_SEARCH_OFF}));
            }
            gtk_widget_grab_focus(GTK_WIDGET(c->gui.webview));
            break;

        case VP_MODE_COMMAND:
        case VP_MODE_HINTING:
            gtk_widget_grab_focus(GTK_WIDGET(c->gui.inputbox));
            break;

        case VP_MODE_INSERT:
            gtk_widget_grab_focus(GTK_WIDGET(c->gui.webview));
            vp_echo(c, VP_MSG_NORMAL, FALSE, "-- INPUT --");
            break;

        case VP_MODE_PATH_THROUGH:
            gtk_widget_grab_focus(GTK_WIDGET(c->gui.webview));
            break;
    }

    c->state.mode = mode;
    c->state.modkey = c->state.count  = 0;

    /* echo message if given */
    if (clean) {
        vp_echo(c, VP_MSG_NORMAL, FALSE, "");
    }

    vp_update_statusbar(c);

    return TRUE;
}

void vp_set_widget_font(GtkWidget* widget, const VpColor* fg, const VpColor* bg, PangoFontDescription* font)
{
    VP_WIDGET_OVERRIDE_FONT(widget, font);
    VP_WIDGET_OVERRIDE_TEXT(widget, GTK_STATE_NORMAL, fg);
    VP_WIDGET_OVERRIDE_COLOR(widget, GTK_STATE_NORMAL, fg);
    VP_WIDGET_OVERRIDE_BASE(widget, GTK_STATE_NORMAL, bg);
    VP_WIDGET_OVERRIDE_BACKGROUND(widget, GTK_STATE_NORMAL, bg);
}

void vp_update_statusbar(Client* c)
{
    GString* status = g_string_new("");

    /* show current count */
    g_string_append_printf(status, "%.0d", c->state.count);
    /* show current modkey */
    if (c->state.modkey) {
        g_string_append_c(status, c->state.modkey);
    }

    /* show the active downloads */
    if (c->state.downloads) {
        int num = g_list_length(c->state.downloads);
        g_string_append_printf(status, " %d %s", num, num == 1 ? "download" : "downloads");
    }

    /* show load status of page or the downloads */
    if (c->state.progress != 100) {
        g_string_append_printf(status, " [%i%%]", c->state.progress);
    }

    /* show the scroll status */
    int max = gtk_adjustment_get_upper(c->gui.adjust_v) - gtk_adjustment_get_page_size(c->gui.adjust_v);
    int val = (int)(gtk_adjustment_get_value(c->gui.adjust_v) / max * 100);

    if (max == 0) {
        g_string_append(status, " All");
    } else if (val == 0) {
        g_string_append(status, " Top");
    } else if (val >= 100) {
        g_string_append(status, " Bot");
    } else {
        g_string_append_printf(status, " %d%%", val);
    }

    gtk_label_set_text(GTK_LABEL(c->gui.statusbar.right), status->str);
    g_string_free(status, TRUE);
}

void vp_update_status_style(Client* c)
{
    StatusType type = c->state.status;
    vp_set_widget_font(
        c->gui.eventbox, &core.style.status_fg[type], &core.style.status_bg[type], core.style.status_font[type]
    );
    vp_set_widget_font(
        c->gui.statusbar.left, &core.style.status_fg[type], &core.style.status_bg[type], core.style.status_font[type]
    );
    vp_set_widget_font(
        c->gui.statusbar.right, &core.style.status_fg[type], &core.style.status_bg[type], core.style.status_font[type]
    );
}

void vp_update_input_style(Client* c, MessageType type)
{
    vp_set_widget_font(
        c->gui.inputbox, &core.style.input_fg[type], &core.style.input_bg[type], core.style.input_font[type]
    );
}

void vp_update_urlbar(Client* c, const char* uri)
{
    gtk_label_set_text(GTK_LABEL(c->gui.statusbar.left), uri);
}

static gboolean vp_hide_message(Client* c)
{
    vp_echo(c, VP_MSG_NORMAL, FALSE, "");

    return FALSE;
}

static void vp_webview_progress_cb(WebKitWebView* view, GParamSpec* pspec, Client* c)
{
    c->state.progress = webkit_web_view_get_progress(view) * 100;
    vp_update_statusbar(c);
}

static void vp_webview_download_progress_cb(WebKitWebView* view, GParamSpec* pspec, Client* c)
{
    if (c->state.downloads) {
        c->state.progress = 0;
        GList* ptr;
        for (ptr = c->state.downloads; ptr; ptr = g_list_next(ptr)) {
            c->state.progress += 100 * webkit_download_get_progress(ptr->data);
        }
        c->state.progress /= g_list_length(c->state.downloads);
    }
    vp_update_statusbar(c);
}

static void vp_webview_load_status_cb(WebKitWebView* view, GParamSpec* pspec, Client* c)
{
    const char* uri = webkit_web_view_get_uri(c->gui.webview);

    switch (webkit_web_view_get_load_status(c->gui.webview)) {
        case WEBKIT_LOAD_PROVISIONAL:
            /* update load progress in statusbar */
            c->state.progress = 0;
            vp_update_statusbar(c);
            break;

        case WEBKIT_LOAD_COMMITTED:
            {
                WebKitWebFrame* frame = webkit_web_view_get_main_frame(c->gui.webview);
                /* set the status */
                if (g_str_has_prefix(uri, "https://")) {
                    WebKitWebDataSource* src      = webkit_web_frame_get_data_source(frame);
                    WebKitNetworkRequest* request = webkit_web_data_source_get_request(src);
                    SoupMessage* msg              = webkit_network_request_get_message(request);
                    SoupMessageFlags flags        = soup_message_get_flags(msg);
                    vp_set_status(
                        c,
                        (flags & SOUP_MESSAGE_CERTIFICATE_TRUSTED) ? VP_STATUS_SSL_VALID : VP_STATUS_SSL_INVALID
                    );
                } else {
                    vp_set_status(c, VP_STATUS_NORMAL);
                }

                /* inject the hinting javascript */
                hints_init(frame);

                /* run user script file */
                vp_run_user_script(frame);
            }

            /* status bar is updated by vp_set_mode */
            vp_set_mode(c, VP_MODE_NORMAL , FALSE);
            vp_update_urlbar(c, uri);

            break;

        case WEBKIT_LOAD_FIRST_VISUALLY_NON_EMPTY_LAYOUT:
            break;

        case WEBKIT_LOAD_FINISHED:
            /* update load progress in statusbar */
            c->state.progress = 100;
            vp_update_statusbar(c);

            dom_check_auto_insert(c);

            url_history_add(uri, webkit_web_view_get_title(c->gui.webview));
            break;

        case WEBKIT_LOAD_FAILED:
            break;
    }
}

static void vp_destroy_window_cb(GtkWidget* widget, Client* c)
{
    vp_destroy_client(c);
}

static void vp_inputbox_activate_cb(GtkEntry *entry, Client* c)
{
    const char* text;
    gboolean hist_save = FALSE;
    char* command  = NULL;
    guint16 length = gtk_entry_get_text_length(entry);

    if (0 == length) {
        return;
    }

    gtk_widget_grab_focus(GTK_WIDGET(c->gui.webview));

    if (length <= 1) {
        return;
    }

    /* do not free or modify text */
    text = gtk_entry_get_text(entry);

    /* duplicate the content because this may change for example if
     * :set varName? is used the text is changed to the new printed
     * content of inputbox */
    command = g_strdup((text));

    Arg a;
    switch (*text) {
        case '/':
        case '?':
            a.i = *text == '/' ? VP_SEARCH_FORWARD : VP_SEARCH_BACKWARD;
            a.s = (command + 1);
            command_search(c, &a);
            hist_save = TRUE;
            break;

        case ':':
            completion_clean(c);
            vp_process_input(c, (command + 1));
            hist_save = TRUE;
            break;
    }

    if (hist_save) {
        /* save the command in history */
        history_append(command);
    }
    g_free(command);
}

static gboolean vp_inputbox_keyrelease_cb(GtkEntry* entry, GdkEventKey* event, Client* c)
{
    return FALSE;
}

static void vp_scroll_cb(GtkAdjustment* adjustment, Client* c)
{
    vp_update_statusbar(c);
}

static void vp_new_request_cb(SoupSession* session, SoupMessage *message, Client* c)
{
    SoupMessageHeaders* header = message->request_headers;
    SoupURI* uri;
    const char* cookie;

    soup_message_headers_remove(header, "Cookie");
    uri = soup_message_get_uri(message);
    if ((cookie = vp_get_cookies(uri))) {
        soup_message_headers_append(header, "Cookie", cookie);
    }
    g_signal_connect_after(G_OBJECT(message), "got-headers", G_CALLBACK(vp_gotheaders_cb), c);
}

static void vp_gotheaders_cb(SoupMessage* message, Client* c)
{
    GSList* list = NULL;
    GSList* p = NULL;

    for(p = list = soup_cookies_from_response(message); p; p = g_slist_next(p)) {
        vp_set_cookie((SoupCookie *)p->data);
    }
    soup_cookies_free(list);
}

static WebKitWebView* vp_inspector_new(WebKitWebInspector* inspector, WebKitWebView* webview, Client* c)
{
    return WEBKIT_WEB_VIEW(webkit_web_view_new());
}

static gboolean vp_inspector_show(WebKitWebInspector* inspector, Client* c)
{
    WebKitWebView* webview;
    int height;

    if (c->state.is_inspecting) {
        return FALSE;
    }

    webview = webkit_web_inspector_get_web_view(inspector);

    /* use about 1/3 of window height for the inspector */
    gtk_window_get_size(GTK_WINDOW(c->gui.window), NULL, &height);
    gtk_paned_set_position(GTK_PANED(c->gui.pane), 2 * height / 3);

    gtk_paned_pack2(GTK_PANED(c->gui.pane), GTK_WIDGET(webview), TRUE, TRUE);
    gtk_widget_show(GTK_WIDGET(webview));

    c->state.is_inspecting = TRUE;

    return TRUE;
}

static gboolean vp_inspector_close(WebKitWebInspector* inspector, Client* c)
{
    WebKitWebView* webview;

    if (!c->state.is_inspecting) {
        return FALSE;
    }
    webview = webkit_web_inspector_get_web_view(inspector);
    gtk_widget_hide(GTK_WIDGET(webview));
    gtk_widget_destroy(GTK_WIDGET(webview));

    c->state.is_inspecting = FALSE;

    return TRUE;
}

static void vp_inspector_finished(WebKitWebInspector* inspector, Client* c)
{
    g_free(c->gui.inspector);
}

/**
 * Processed input from input box without trailing : or ? /, input from config
 * file and default config.
 */
static gboolean vp_process_input(Client* c, const char* input)
{
    gboolean success;
    char* command = NULL;
    char** token;

    if (!input || !strlen(input)) {
        return FALSE;
    }

    /* get a possible command count */
    if (c) {
        c->state.count = g_ascii_strtoll(input, &command, 10);
    } else {
        command = (char*)input;
    }

    /* split the input string into command and parameter part */
    token = g_strsplit(command, " ", 2);

    if (!token[0]) {
        g_strfreev(token);
        return FALSE;
    }
    success = command_run(c, token[0], token[1] ? token[1] : NULL);
    g_strfreev(token);

    return success;
}

#ifdef FEATURE_COOKIE
static void vp_set_cookie(SoupCookie* cookie)
{
    SoupDate* date;

    SoupCookieJar* jar = soup_cookie_jar_text_new(core.files[FILES_COOKIE], FALSE);
    cookie = soup_cookie_copy(cookie);
    if (cookie->expires == NULL && core.config.cookie_timeout) {
        date = soup_date_new_from_time_t(time(NULL) + core.config.cookie_timeout);
        soup_cookie_set_expires(cookie, date);
    }
    soup_cookie_jar_add_cookie(jar, cookie);
    g_object_unref(jar);
}

static const char* vp_get_cookies(SoupURI *uri)
{
    const char* cookie;

    SoupCookieJar* jar = soup_cookie_jar_text_new(core.files[FILES_COOKIE], TRUE);
    cookie = soup_cookie_jar_get_cookies(jar, uri, TRUE);
    g_object_unref(jar);

    return cookie;
}
#endif

static void vp_set_status(Client* c, const StatusType status)
{
    if (c->state.status != status) {
        c->state.status = status;
        /* update the statusbar style only if the status changed */
        vp_update_status_style(c);
    }
}

static void vp_run_user_script(WebKitWebFrame* frame)
{
    char* js      = NULL;
    GError* error = NULL;

    if (g_file_test(core.files[FILES_SCRIPT], G_FILE_TEST_IS_REGULAR)
        && g_file_get_contents(core.files[FILES_SCRIPT], &js, NULL, &error)
    ) {
        char* value = NULL;
        char* error = NULL;

        vp_eval_script(frame, js, core.files[FILES_SCRIPT], &value, &error);
        if (error) {
            fprintf(stderr, "%s", error);
            g_free(error);
        } else {
            g_free(value);
        }
        g_free(js);
    }
}

static char* vp_jsref_to_string(JSContextRef context, JSValueRef ref)
{
    char* string;
    JSStringRef str_ref = JSValueToStringCopy(context, ref, NULL);
    size_t len          = JSStringGetMaximumUTF8CStringSize(str_ref);

    string = g_new0(char, len);
    JSStringGetUTF8CString(str_ref, string, len);
    JSStringRelease(str_ref);

    return string;
}

static void vp_init_core(void)
{
    /* TODO */
    /* initialize the commands hash map */
    command_init();

    /* initialize the config files */
    vp_init_files();

    /* initialize the keybindings */
    keybind_init();

    /* init soup session */
    core.soup_session = webkit_get_default_session();
    soup_session_remove_feature_by_type(core.soup_session, soup_cookie_jar_get_type());
    g_object_set(core.soup_session, "max-conns", SETTING_MAX_CONNS , NULL);
    g_object_set(core.soup_session, "max-conns-per-host", SETTING_MAX_CONNS_PER_HOST, NULL);

    /* initialize settings */
    setting_init();

    /* read additional configuration from config files */
    vp_read_global_config();

    url_history_init();
}

static void vp_read_global_config(void)
{
    /* load default config */
    for (guint i = 0; default_config[i].command != NULL; i++) {
        if (!vp_process_input(NULL, default_config[i].command)) {
            fprintf(stderr, "Invalid default config: %s\n", default_config[i].command);
        }
    }

    vp_process_config_file(NULL, FILES_GLOBAL_CONFIG);
}

static void vp_process_config_file(Client* c, VpFile file)
{
    /* read config from config files */
    char **lines = util_get_lines(core.files[file]);
    char *line;

    if (lines) {
        int length = g_strv_length(lines) - 1;
        for (int i = 0; i < length; i++) {
            line = lines[i];
            g_strstrip(line);

            if (!g_ascii_isalpha(line[0])) {
                continue;
            }
            if (!vp_process_input(c, line)) {
                fprintf(stderr, "Invalid config: %s\n", line);
            }
        }
    }
    g_strfreev(lines);
}

static Client* vp_client_new(void)
{
    Client* c = g_new0(Client, 1);
    Gui* gui  = &c->gui;

    if (core.embed) {
        gui->window = gtk_plug_new(core.embed);
    } else {
        gui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_wmclass(GTK_WINDOW(gui->window), "vimp", "Vimp");
        gtk_window_set_role(GTK_WINDOW(gui->window), "Vimp");
    }

    GdkGeometry hints = {10, 10};
    gtk_window_set_default_size(GTK_WINDOW(gui->window), 640, 480);
    gtk_window_set_title(GTK_WINDOW(gui->window), "vimp");
    gtk_window_set_geometry_hints(GTK_WINDOW(gui->window), NULL, &hints, GDK_HINT_MIN_SIZE);
    gtk_window_set_icon(GTK_WINDOW(gui->window), NULL);
    gtk_widget_set_name(GTK_WIDGET(gui->window), "vimp");

    /* Create a browser instance */
    gui->webview = WEBKIT_WEB_VIEW(webkit_web_view_new());
    gui->inspector = webkit_web_view_get_inspector(gui->webview);

    /* Create a scrollable area */
    gui->scroll = gtk_scrolled_window_new(NULL, NULL);
    gui->adjust_h = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(gui->scroll));
    gui->adjust_v = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(gui->scroll));

    /* Prepare the inputbox */
    gui->inputbox = gtk_entry_new();
    gtk_entry_set_inner_border(GTK_ENTRY(gui->inputbox), NULL);
    g_object_set(gtk_widget_get_settings(gui->inputbox), "gtk-entry-select-on-focus", FALSE, NULL);

#ifdef HAS_GTK3
    gui->pane            = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
    gui->box             = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gui->statusbar.box   = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
#else
    gui->pane            = gtk_vpaned_new();
    gui->box             = GTK_BOX(gtk_vbox_new(FALSE, 0));
    gui->statusbar.box   = GTK_BOX(gtk_hbox_new(FALSE, 0));
#endif
    gui->statusbar.left  = gtk_label_new(NULL);
    gui->statusbar.right = gtk_label_new(NULL);

    /* Prepare the event box */
    gui->eventbox = gtk_event_box_new();

    gtk_paned_pack1(GTK_PANED(gui->pane), GTK_WIDGET(gui->box), TRUE, TRUE);

    vp_setup_signals(c);

    /* Put all part together */
    gtk_container_add(GTK_CONTAINER(gui->scroll), GTK_WIDGET(gui->webview));
    gtk_container_add(GTK_CONTAINER(gui->eventbox), GTK_WIDGET(gui->statusbar.box));
    gtk_container_add(GTK_CONTAINER(gui->window), GTK_WIDGET(gui->pane));
    gtk_misc_set_alignment(GTK_MISC(gui->statusbar.left), 0.0, 0.0);
    gtk_misc_set_alignment(GTK_MISC(gui->statusbar.right), 1.0, 0.0);
    gtk_box_pack_start(gui->statusbar.box, gui->statusbar.left, TRUE, TRUE, 2);
    gtk_box_pack_start(gui->statusbar.box, gui->statusbar.right, FALSE, FALSE, 2);

    gtk_box_pack_start(gui->box, gui->scroll, TRUE, TRUE, 0);
    gtk_box_pack_start(gui->box, gui->eventbox, FALSE, FALSE, 0);
    gtk_entry_set_has_frame(GTK_ENTRY(gui->inputbox), FALSE);
    gtk_box_pack_end(gui->box, gui->inputbox, FALSE, FALSE, 0);

    /* Make sure that when the browser area becomes visible, it will get mouse
     * and keyboard events */
    gtk_widget_grab_focus(GTK_WIDGET(gui->webview));

    /* Make sure the main window and all its contents are visible */
    gtk_widget_show_all(gui->window);

    keybind_init_client(c);
    setting_init_client(c);

    vp_process_config_file(c, FILES_LOCAL_CONFIG);

    /* apply global settings to the status bar and input box */
    vp_update_status_style(c);
    vp_update_input_style(c, VP_MSG_NORMAL);

    c->next = clients;
    clients = c;

    return c;
}

static void vp_setup_signals(Client* c)
{
    /* Set up callbacks so that if either the main window or the browser
     * instance is closed, the program will exit */
    g_signal_connect(c->gui.window, "destroy", G_CALLBACK(vp_destroy_window_cb), c);
    g_object_connect(
        G_OBJECT(c->gui.webview),
        "signal::notify::progress", G_CALLBACK(vp_webview_progress_cb), c,
        "signal::notify::load-status", G_CALLBACK(vp_webview_load_status_cb), c,
        "signal::button-release-event", G_CALLBACK(vp_button_relase_cb), c,
        "signal::new-window-policy-decision-requested", G_CALLBACK(vp_new_window_policy_cb), c,
        "signal::create-web-view", G_CALLBACK(vp_create_new_webview_cb), c,
        "signal::hovering-over-link", G_CALLBACK(vp_hover_link_cb), c,
        "signal::title-changed", G_CALLBACK(vp_title_changed_cb), c,
        "signal::mime-type-policy-decision-requested", G_CALLBACK(vp_mimetype_decision_cb), c,
        "signal::download-requested", G_CALLBACK(vp_download_requested_cb), c,
        "signal::resource-request-starting", G_CALLBACK(vp_request_start_cb), c,
        NULL
    );

    g_object_connect(
        G_OBJECT(c->gui.inputbox),
        "signal::activate",          G_CALLBACK(vp_inputbox_activate_cb),   c,
        "signal::key-release-event", G_CALLBACK(vp_inputbox_keyrelease_cb), c,
        NULL
    );
    /* webview adjustment */
    g_object_connect(G_OBJECT(c->gui.adjust_v),
        "signal::value-changed",     G_CALLBACK(vp_scroll_cb),              c,
        NULL
    );

    g_signal_connect_after(G_OBJECT(core.soup_session), "request-started", G_CALLBACK(vp_new_request_cb), c);

    /* inspector */
    /* TODO use g_object_connect instead */
    g_signal_connect(G_OBJECT(c->gui.inspector), "inspect-web-view", G_CALLBACK(vp_inspector_new), c);
    g_signal_connect(G_OBJECT(c->gui.inspector), "show-window", G_CALLBACK(vp_inspector_show), c);
    g_signal_connect(G_OBJECT(c->gui.inspector), "close-window", G_CALLBACK(vp_inspector_close), c);
    g_signal_connect(G_OBJECT(c->gui.inspector), "finished", G_CALLBACK(vp_inspector_finished), c);
}

static void vp_init_files(void)
{
    char* path = util_get_config_dir();

    core.files[FILES_GLOBAL_CONFIG] = g_build_filename(path, "global", NULL);
    util_create_file_if_not_exists(core.files[FILES_GLOBAL_CONFIG]);

    core.files[FILES_LOCAL_CONFIG] = g_build_filename(path, "local", NULL);
    util_create_file_if_not_exists(core.files[FILES_LOCAL_CONFIG]);

    core.files[FILES_COOKIE] = g_build_filename(path, "cookies", NULL);
    util_create_file_if_not_exists(core.files[FILES_COOKIE]);

    core.files[FILES_CLOSED] = g_build_filename(path, "closed", NULL);
    util_create_file_if_not_exists(core.files[FILES_CLOSED]);

    core.files[FILES_SCRIPT] = g_build_filename(path, "scripts.js", NULL);

    core.files[FILES_HISTORY] = g_build_filename(path, "history", NULL);
    util_create_file_if_not_exists(core.files[FILES_HISTORY]);

    core.files[FILES_USER_STYLE] = g_build_filename(path, "style.css", NULL);
    util_create_file_if_not_exists(core.files[FILES_USER_STYLE]);

    g_free(path);
}

static gboolean vp_button_relase_cb(WebKitWebView* webview, GdkEventButton* event, Client* c)
{
    gboolean propagate = FALSE;
    WebKitHitTestResultContext context;
    Mode mode = CLEAN_MODE(c->state.mode);

    WebKitHitTestResult *result = webkit_web_view_get_hit_test_result(webview, event);

    g_object_get(result, "context", &context, NULL);
    if (mode == VP_MODE_NORMAL && context & WEBKIT_HIT_TEST_RESULT_CONTEXT_EDITABLE) {
        vp_set_mode(c, VP_MODE_INSERT, FALSE);

        propagate = TRUE;
    }
    /* middle mouse click onto link */
    if (context & WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK && event->button == 2) {
        Arg a = {VP_TARGET_NEW};
        g_object_get(result, "link-uri", &a.s, NULL);
        vp_load_uri(c, &a);

        propagate = TRUE;
    }
    g_object_unref(result);

    return propagate;
}

static gboolean vp_new_window_policy_cb(
    WebKitWebView* view, WebKitWebFrame* frame, WebKitNetworkRequest* request,
    WebKitWebNavigationAction* navig, WebKitWebPolicyDecision* policy, Client* c)
{
    if (webkit_web_navigation_action_get_reason(navig) == WEBKIT_WEB_NAVIGATION_REASON_LINK_CLICKED) {
        /* open in a new window */
        Arg a = {VP_TARGET_NEW, (char*)webkit_network_request_get_uri(request)};
        vp_load_uri(c, &a);
        webkit_web_policy_decision_ignore(policy);
        return TRUE;
    }
    return FALSE;
}

static WebKitWebView* vp_create_new_webview_cb(WebKitWebView* webview, WebKitWebFrame* frame, Client* c)
{
    Client* new = vp_client_new();
    return new->gui.webview;
}

static void vp_hover_link_cb(WebKitWebView* webview, const char* title, const char* link, Client* c)
{
    if (link) {
        char* message = g_strdup_printf("Link: %s", link);
        gtk_label_set_text(GTK_LABEL(c->gui.statusbar.left), message);
        g_free(message);
    } else {
        vp_update_urlbar(c, webkit_web_view_get_uri(webview));
    }
}

static void vp_title_changed_cb(WebKitWebView* webview, WebKitWebFrame* frame, const char* title, Client* c)
{
    gtk_window_set_title(GTK_WINDOW(c->gui.window), title);
}

static gboolean vp_mimetype_decision_cb(WebKitWebView* webview,
    WebKitWebFrame* frame, WebKitNetworkRequest* request, char*
    mime_type, WebKitWebPolicyDecision* decision, Client* c)
{
    if (webkit_web_view_can_show_mime_type(webview, mime_type) == FALSE) {
        webkit_web_policy_decision_download(decision);

        return TRUE;
    }
    return FALSE;
}

static gboolean vp_download_requested_cb(WebKitWebView* view, WebKitDownload* download, Client* c)
{
    WebKitDownloadStatus status;
    char* uri = NULL;

    const char* filename = webkit_download_get_suggested_filename(download);
    if (!filename) {
        filename = "vimp_donwload";
    }

    /* prepare the download target path */
    uri = g_strdup_printf("file://%s%c%s", core.config.download_dir, G_DIR_SEPARATOR, filename);
    webkit_download_set_destination_uri(download, uri);
    g_free(uri);

    guint64 size = webkit_download_get_total_size(download);
    if (size > 0) {
        vp_echo(c, VP_MSG_NORMAL, FALSE, "Download %s [~%uB] started ...", filename, size);
    } else {
        vp_echo(c, VP_MSG_NORMAL, FALSE, "Download %s started ...", filename);
    }

    status = webkit_download_get_status(download);
    if (status == WEBKIT_DOWNLOAD_STATUS_CREATED) {
        webkit_download_start(download);
    }

    /* prepend the download to the download list */
    c->state.downloads = g_list_prepend(c->state.downloads, download);

    /* connect signal handler to check if the download is done */
    g_signal_connect(download, "notify::status", G_CALLBACK(vp_download_progress_cp), c);
    g_signal_connect(download, "notify::progress", G_CALLBACK(vp_webview_download_progress_cb), c);

    vp_update_statusbar(c);

    return TRUE;
}

/**
 * Callback to filter started resource request.
 */
static void vp_request_start_cb(WebKitWebView* webview, WebKitWebFrame* frame,
    WebKitWebResource* resource, WebKitNetworkRequest* request,
    WebKitNetworkResponse* response, Client* c)
{
    const char* uri = webkit_network_request_get_uri(request);
    if (g_str_has_suffix(uri, "/favicon.ico")) {
        webkit_network_request_set_uri(request, "about:blank");
    }
}

static void vp_download_progress_cp(WebKitDownload* download, GParamSpec* pspec, Client* c)
{
    WebKitDownloadStatus status = webkit_download_get_status(download);

    if (status == WEBKIT_DOWNLOAD_STATUS_STARTED || status == WEBKIT_DOWNLOAD_STATUS_CREATED) {
        return;
    }

    char* file = g_path_get_basename(webkit_download_get_destination_uri(download));
    if (status != WEBKIT_DOWNLOAD_STATUS_FINISHED) {
        vp_echo(c, VP_MSG_ERROR, FALSE, "Error downloading %s", file);
    } else {
        vp_echo(c, VP_MSG_NORMAL, FALSE, "Download %s finished", file);
    }
    g_free(file);

    /* remove the donwload from the list */
    c->state.downloads = g_list_remove(c->state.downloads, download);

    vp_update_statusbar(c);
}

static void vp_destroy_client(Client* c)
{
    const char* uri = webkit_web_view_get_uri(c->gui.webview);
    /* write last URL into file for recreation */
    if (uri) {
        g_file_set_contents(core.files[FILES_CLOSED], uri, -1, NULL);
    }

    Client* p;

    completion_clean(c);

    webkit_web_view_stop_loading(c->gui.webview);
    gtk_widget_destroy(GTK_WIDGET(c->gui.webview));
    gtk_widget_destroy(GTK_WIDGET(c->gui.scroll));
    gtk_widget_destroy(GTK_WIDGET(c->gui.box));
    gtk_widget_destroy(GTK_WIDGET(c->gui.window));

    for(p = clients; p && p->next != c; p = p->next);
    if (p) {
        p->next = c->next;
    } else {
        clients = c->next;
    }
    g_free(c);
    if (clients == NULL) {
        gtk_main_quit();
    }
}

static void vp_clean_up(void)
{
    while (clients) {
        vp_destroy_client(clients);
    }

    command_cleanup();
    setting_cleanup();
    keybind_cleanup();
    searchengine_cleanup();
    url_history_cleanup();

    for (int i = 0; i < FILES_LAST; i++) {
        g_free(core.files[i]);
    }
}

int main(int argc, char* argv[])
{
    static char* winid = NULL;
    static gboolean ver = false;
    static GError* err;
    static GOptionEntry opts[] = {
        {"version", 'v', 0, G_OPTION_ARG_NONE, &ver, "Print version", NULL},
        {"embed", 'e', 0, G_OPTION_ARG_STRING, &winid, "Reparents to window specified by xid", NULL},
        {NULL}
    };
    /* Initialize GTK+ */
    if (!gtk_init_with_args(&argc, &argv, "[URI]", opts, NULL, &err)) {
        g_printerr("can't init gtk: %s\n", err->message);
        g_error_free(err);

        return EXIT_FAILURE;
    }

    if (ver) {
        fprintf(stderr, "%s/%s (build %s %s)\n", PROJECT, VERSION, __DATE__, __TIME__);
        return EXIT_SUCCESS;
    }

    /* save arguments */
    args = argv;

    if (winid) {
        core.embed = strtol(winid, NULL, 0);
    }

    vp_init_core();

    vp_client_new();

    /* command line argument: URL */
    Arg arg = {VP_TARGET_CURRENT};
    if (argc > 1) {
        arg.s = g_strdup(argv[argc - 1]);
    } else {
        arg.s = g_strdup(core.config.home_page);
    }
    vp_load_uri(clients, &arg);
    g_free(arg.s);

    /* Run the main GTK+ event loop */
    gtk_main();
    vp_clean_up();

    return EXIT_SUCCESS;
}
