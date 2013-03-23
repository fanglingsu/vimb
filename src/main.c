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

#include <sys/stat.h>
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
VbCore      vb;

/* callbacks */
static void vb_webview_progress_cb(WebKitWebView* view, GParamSpec* pspec);
static void vb_webview_download_progress_cb(WebKitWebView* view, GParamSpec* pspec);
static void vb_webview_load_status_cb(WebKitWebView* view, GParamSpec* pspec);
static void vb_destroy_window_cb(GtkWidget* widget);
static void vb_inputbox_activate_cb(GtkEntry* entry);
static gboolean vb_inputbox_keyrelease_cb(GtkEntry* entry, GdkEventKey* event);
static void vb_scroll_cb(GtkAdjustment* adjustment);
static void vb_new_request_cb(SoupSession* session, SoupMessage *message);
static void vb_gotheaders_cb(SoupMessage* message);
static WebKitWebView* vb_inspector_new(WebKitWebInspector* inspector, WebKitWebView* webview);
static gboolean vb_inspector_show(WebKitWebInspector* inspector);
static gboolean vb_inspector_close(WebKitWebInspector* inspector);
static void vb_inspector_finished(WebKitWebInspector* inspector);
static gboolean vb_button_relase_cb(WebKitWebView *webview, GdkEventButton* event);
static gboolean vb_new_window_policy_cb(
    WebKitWebView* view, WebKitWebFrame* frame, WebKitNetworkRequest* request,
    WebKitWebNavigationAction* navig, WebKitWebPolicyDecision* policy);
static void vb_hover_link_cb(WebKitWebView* webview, const char* title, const char* link);
static void vb_title_changed_cb(WebKitWebView* webview, WebKitWebFrame* frame, const char* title);
static gboolean vb_mimetype_decision_cb(WebKitWebView* webview,
    WebKitWebFrame* frame, WebKitNetworkRequest* request, char*
    mime_type, WebKitWebPolicyDecision* decision);
static gboolean vb_download_requested_cb(WebKitWebView* view, WebKitDownload* download);
static void vb_download_progress_cp(WebKitDownload* download, GParamSpec* pspec);
static void vb_request_start_cb(WebKitWebView* webview, WebKitWebFrame* frame,
    WebKitWebResource* resource, WebKitNetworkRequest* request,
    WebKitNetworkResponse* response);

/* functions */
static gboolean vb_process_input(const char* input);
static void vb_run_user_script(WebKitWebFrame* frame);
static char* vb_jsref_to_string(JSContextRef context, JSValueRef ref);
static void vb_init_core(void);
static void vb_read_config(void);
static void vb_setup_signals();
static void vb_init_files(void);
static void vb_set_cookie(SoupCookie* cookie);
static const char* vb_get_cookies(SoupURI *uri);
static gboolean vb_hide_message();
static void vb_set_status(const StatusType status);
static void vb_destroy_client();

void vb_clean_input()
{
    /* move focus from input box to clean it */
    gtk_widget_grab_focus(GTK_WIDGET(vb.gui.webview));
    vb_echo(VB_MSG_NORMAL, FALSE, "");
}

void vb_echo(const MessageType type, gboolean hide, const char *error, ...)
{
    va_list arg_list;

    va_start(arg_list, error);
    char message[255];
    vsnprintf(message, 255, error, arg_list);
    va_end(arg_list);

    /* don't print message if the input is focussed */
    if (gtk_widget_is_focus(GTK_WIDGET(vb.gui.inputbox))) {
        return;
    }

    vb_update_input_style(type);
    gtk_entry_set_text(GTK_ENTRY(vb.gui.inputbox), message);
    if (hide) {
        g_timeout_add_seconds(MESSAGE_TIMEOUT, (GSourceFunc)vb_hide_message, NULL);
    }
}

gboolean vb_eval_script(WebKitWebFrame* frame, char* script, char* file, char** value)
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
        *value = vb_jsref_to_string(js, result);
        return TRUE;
    }

    *value = vb_jsref_to_string(js, exception);
    return FALSE;
}

gboolean vb_load_uri(const Arg* arg)
{
    char* uri;
    char* path = arg->s;
    struct stat st;

    if (!path) {
        return FALSE;
    }

    g_strstrip(path);
    if (!strlen(path)) {
        return FALSE;
    }

    /* check if the path is a file path */
    if (stat(path, &st) == 0) {
        char* rp = realpath(path, NULL);
        uri = g_strdup_printf("file://%s", rp);
    } else {
        uri = g_strrstr(path, "://") ? g_strdup(path) : g_strdup_printf("http://%s", path);
    }

    /* change state to normal mode */
    vb_set_mode(VB_MODE_NORMAL, FALSE);

    if (arg->i == VB_TARGET_NEW) {
        guint i = 0;
        char* cmd[5];
        char xid[64];

        cmd[i++] = *args;
        if (vb.embed) {
            cmd[i++] = "-e";
            snprintf(xid, LENGTH(xid), "%u", (int)vb.embed);
            cmd[i++] = xid;
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

    return TRUE;
}

gboolean vb_set_clipboard(const Arg* arg)
{
    gboolean result = FALSE;
    if (!arg->s) {
        return result;
    }

    if (arg->i & VB_CLIPBOARD_PRIMARY) {
        gtk_clipboard_set_text(PRIMARY_CLIPBOARD(), arg->s, -1);
        result = TRUE;
    }
    if (arg->i & VB_CLIPBOARD_SECONDARY) {
        gtk_clipboard_set_text(SECONDARY_CLIPBOARD(), arg->s, -1);
        result = TRUE;
    }

    return result;
}

/**
 * Set the base modes. All other mode flags like completion can be set directly
 * to vb.state.mode.
 */
gboolean vb_set_mode(Mode mode, gboolean clean)
{
    if ((vb.state.mode & VB_MODE_COMPLETE)
        && !(mode & VB_MODE_COMPLETE)
    ) {
        completion_clean();
    }
    int clean_mode = CLEAN_MODE(vb.state.mode);
    switch (CLEAN_MODE(mode)) {
        case VB_MODE_NORMAL:
            /* do this only if the mode is really switched */
            if (clean_mode != VB_MODE_NORMAL) {
                history_rewind();
            }
            if (clean_mode == VB_MODE_HINTING) {
                /* if previous mode was hinting clear the hints */
                hints_clear();
            } else if (clean_mode == VB_MODE_INSERT) {
                /* clean the input if current mode is insert to remove -- INPUT -- */
                clean = TRUE;
            } else if (clean_mode == VB_MODE_SEARCH) {
                /* cleaup previous search */
                command_search(&((Arg){VB_SEARCH_OFF}));
            }
            gtk_widget_grab_focus(GTK_WIDGET(vb.gui.webview));
            break;

        case VB_MODE_COMMAND:
        case VB_MODE_HINTING:
            gtk_widget_grab_focus(GTK_WIDGET(vb.gui.inputbox));
            break;

        case VB_MODE_INSERT:
            gtk_widget_grab_focus(GTK_WIDGET(vb.gui.webview));
            vb_echo(VB_MSG_NORMAL, FALSE, "-- INPUT --");
            break;

        case VB_MODE_PATH_THROUGH:
            gtk_widget_grab_focus(GTK_WIDGET(vb.gui.webview));
            break;
    }

    vb.state.mode = mode;
    vb.state.modkey = vb.state.count  = 0;

    /* echo message if given */
    if (clean) {
        vb_echo(VB_MSG_NORMAL, FALSE, "");
    }

    vb_update_statusbar();

    return TRUE;
}

void vb_set_widget_font(GtkWidget* widget, const VpColor* fg, const VpColor* bg, PangoFontDescription* font)
{
    VB_WIDGET_OVERRIDE_FONT(widget, font);
    VB_WIDGET_OVERRIDE_TEXT(widget, GTK_STATE_NORMAL, fg);
    VB_WIDGET_OVERRIDE_COLOR(widget, GTK_STATE_NORMAL, fg);
    VB_WIDGET_OVERRIDE_BASE(widget, GTK_STATE_NORMAL, bg);
    VB_WIDGET_OVERRIDE_BACKGROUND(widget, GTK_STATE_NORMAL, bg);
}

void vb_update_statusbar()
{
    GString* status = g_string_new("");

    /* show current count */
    g_string_append_printf(status, "%.0d", vb.state.count);
    /* show current modkey */
    if (vb.state.modkey) {
        g_string_append_c(status, vb.state.modkey);
    }

    /* show the active downloads */
    if (vb.state.downloads) {
        int num = g_list_length(vb.state.downloads);
        g_string_append_printf(status, " %d %s", num, num == 1 ? "download" : "downloads");
    }

    /* show load status of page or the downloads */
    if (vb.state.progress != 100) {
        g_string_append_printf(status, " [%i%%]", vb.state.progress);
    }

    /* show the scroll status */
    int max = gtk_adjustment_get_upper(vb.gui.adjust_v) - gtk_adjustment_get_page_size(vb.gui.adjust_v);
    int val = (int)(gtk_adjustment_get_value(vb.gui.adjust_v) / max * 100);

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
    g_string_free(status, TRUE);
}

void vb_update_status_style()
{
    StatusType type = vb.state.status;
    vb_set_widget_font(
        vb.gui.eventbox, &vb.style.status_fg[type], &vb.style.status_bg[type], vb.style.status_font[type]
    );
    vb_set_widget_font(
        vb.gui.statusbar.left, &vb.style.status_fg[type], &vb.style.status_bg[type], vb.style.status_font[type]
    );
    vb_set_widget_font(
        vb.gui.statusbar.right, &vb.style.status_fg[type], &vb.style.status_bg[type], vb.style.status_font[type]
    );
}

void vb_update_input_style(MessageType type)
{
    vb_set_widget_font(
        vb.gui.inputbox, &vb.style.input_fg[type], &vb.style.input_bg[type], vb.style.input_font[type]
    );
}

void vb_update_urlbar(const char* uri)
{
    gtk_label_set_text(GTK_LABEL(vb.gui.statusbar.left), uri);
}

static gboolean vb_hide_message()
{
    vb_echo(VB_MSG_NORMAL, FALSE, "");

    return FALSE;
}

static void vb_webview_progress_cb(WebKitWebView* view, GParamSpec* pspec)
{
    vb.state.progress = webkit_web_view_get_progress(view) * 100;
    vb_update_statusbar();
}

static void vb_webview_download_progress_cb(WebKitWebView* view, GParamSpec* pspec)
{
    if (vb.state.downloads) {
        vb.state.progress = 0;
        GList* ptr;
        for (ptr = vb.state.downloads; ptr; ptr = g_list_next(ptr)) {
            vb.state.progress += 100 * webkit_download_get_progress(ptr->data);
        }
        vb.state.progress /= g_list_length(vb.state.downloads);
    }
    vb_update_statusbar();
}

static void vb_webview_load_status_cb(WebKitWebView* view, GParamSpec* pspec)
{
    const char* uri = webkit_web_view_get_uri(vb.gui.webview);

    switch (webkit_web_view_get_load_status(vb.gui.webview)) {
        case WEBKIT_LOAD_PROVISIONAL:
            /* update load progress in statusbar */
            vb.state.progress = 0;
            vb_update_statusbar();
            break;

        case WEBKIT_LOAD_COMMITTED:
            {
                WebKitWebFrame* frame = webkit_web_view_get_main_frame(vb.gui.webview);
                /* set the status */
                if (g_str_has_prefix(uri, "https://")) {
                    WebKitWebDataSource* src      = webkit_web_frame_get_data_source(frame);
                    WebKitNetworkRequest* request = webkit_web_data_source_get_request(src);
                    SoupMessage* msg              = webkit_network_request_get_message(request);
                    SoupMessageFlags flags        = soup_message_get_flags(msg);
                    vb_set_status(
                        (flags & SOUP_MESSAGE_CERTIFICATE_TRUSTED) ? VB_STATUS_SSL_VALID : VB_STATUS_SSL_INVALID
                    );
                } else {
                    vb_set_status(VB_STATUS_NORMAL);
                }

                /* inject the hinting javascript */
                hints_init(frame);

                /* run user script file */
                vb_run_user_script(frame);
            }

            /* status bar is updated by vb_set_mode */
            vb_set_mode(VB_MODE_NORMAL , FALSE);
            vb_update_urlbar(uri);

            break;

        case WEBKIT_LOAD_FIRST_VISUALLY_NON_EMPTY_LAYOUT:
            break;

        case WEBKIT_LOAD_FINISHED:
            /* update load progress in statusbar */
            vb.state.progress = 100;
            vb_update_statusbar();

            dom_check_auto_insert();

            url_history_add(uri, webkit_web_view_get_title(vb.gui.webview));
            break;

        case WEBKIT_LOAD_FAILED:
            break;
    }
}

static void vb_destroy_window_cb(GtkWidget* widget)
{
    vb_destroy_client();
}

static void vb_inputbox_activate_cb(GtkEntry *entry)
{
    const char* text;
    gboolean hist_save = FALSE;
    char* command  = NULL;
    guint16 length = gtk_entry_get_text_length(entry);

    if (0 == length) {
        return;
    }

    gtk_widget_grab_focus(GTK_WIDGET(vb.gui.webview));

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
            a.i = *text == '/' ? VB_SEARCH_FORWARD : VB_SEARCH_BACKWARD;
            a.s = (command + 1);
            command_search(&a);
            hist_save = TRUE;
            break;

        case ':':
            completion_clean();
            vb_process_input((command + 1));
            hist_save = TRUE;
            break;
    }

    if (hist_save) {
        /* save the command in history */
        history_append(command);
    }
    g_free(command);
}

static gboolean vb_inputbox_keyrelease_cb(GtkEntry* entry, GdkEventKey* event)
{
    return FALSE;
}

static void vb_scroll_cb(GtkAdjustment* adjustment)
{
    vb_update_statusbar();
}

static void vb_new_request_cb(SoupSession* session, SoupMessage *message)
{
    SoupMessageHeaders* header = message->request_headers;
    SoupURI* uri;
    const char* cookie;

    soup_message_headers_remove(header, "Cookie");
    uri = soup_message_get_uri(message);
    if ((cookie = vb_get_cookies(uri))) {
        soup_message_headers_append(header, "Cookie", cookie);
    }
    g_signal_connect_after(G_OBJECT(message), "got-headers", G_CALLBACK(vb_gotheaders_cb), NULL);
}

static void vb_gotheaders_cb(SoupMessage* message)
{
    GSList* list = NULL;
    GSList* p = NULL;

    for(p = list = soup_cookies_from_response(message); p; p = g_slist_next(p)) {
        vb_set_cookie((SoupCookie *)p->data);
    }
    soup_cookies_free(list);
}

static WebKitWebView* vb_inspector_new(WebKitWebInspector* inspector, WebKitWebView* webview)
{
    return WEBKIT_WEB_VIEW(webkit_web_view_new());
}

static gboolean vb_inspector_show(WebKitWebInspector* inspector)
{
    WebKitWebView* webview;
    int height;

    if (vb.state.is_inspecting) {
        return FALSE;
    }

    webview = webkit_web_inspector_get_web_view(inspector);

    /* use about 1/3 of window height for the inspector */
    gtk_window_get_size(GTK_WINDOW(vb.gui.window), NULL, &height);
    gtk_paned_set_position(GTK_PANED(vb.gui.pane), 2 * height / 3);

    gtk_paned_pack2(GTK_PANED(vb.gui.pane), GTK_WIDGET(webview), TRUE, TRUE);
    gtk_widget_show(GTK_WIDGET(webview));

    vb.state.is_inspecting = TRUE;

    return TRUE;
}

static gboolean vb_inspector_close(WebKitWebInspector* inspector)
{
    WebKitWebView* webview;

    if (!vb.state.is_inspecting) {
        return FALSE;
    }
    webview = webkit_web_inspector_get_web_view(inspector);
    gtk_widget_hide(GTK_WIDGET(webview));
    gtk_widget_destroy(GTK_WIDGET(webview));

    vb.state.is_inspecting = FALSE;

    return TRUE;
}

static void vb_inspector_finished(WebKitWebInspector* inspector)
{
    g_free(vb.gui.inspector);
}

/**
 * Processed input from input box without trailing : or ? /, input from config
 * file and default config.
 */
static gboolean vb_process_input(const char* input)
{
    gboolean success;
    char* command = NULL;
    char** token;

    if (!input || !strlen(input)) {
        return FALSE;
    }

    /* get a possible command count */
    vb.state.count = g_ascii_strtoll(input, &command, 10);

    /* split the input string into command and parameter part */
    token = g_strsplit(command, " ", 2);

    if (!token[0]) {
        g_strfreev(token);
        return FALSE;
    }
    success = command_run(token[0], token[1] ? token[1] : NULL);
    g_strfreev(token);

    return success;
}

#ifdef FEATURE_COOKIE
static void vb_set_cookie(SoupCookie* cookie)
{
    SoupDate* date;

    SoupCookieJar* jar = soup_cookie_jar_text_new(vb.files[FILES_COOKIE], FALSE);
    cookie = soup_cookie_copy(cookie);
    if (cookie->expires == NULL && vb.config.cookie_timeout) {
        date = soup_date_new_from_time_t(time(NULL) + vb.config.cookie_timeout);
        soup_cookie_set_expires(cookie, date);
    }
    soup_cookie_jar_add_cookie(jar, cookie);
    g_object_unref(jar);
}

static const char* vb_get_cookies(SoupURI *uri)
{
    const char* cookie;

    SoupCookieJar* jar = soup_cookie_jar_text_new(vb.files[FILES_COOKIE], TRUE);
    cookie = soup_cookie_jar_get_cookies(jar, uri, TRUE);
    g_object_unref(jar);

    return cookie;
}
#endif

static void vb_set_status(const StatusType status)
{
    if (vb.state.status != status) {
        vb.state.status = status;
        /* update the statusbar style only if the status changed */
        vb_update_status_style();
    }
}

static void vb_run_user_script(WebKitWebFrame* frame)
{
    char* js      = NULL;
    GError* error = NULL;

    if (g_file_test(vb.files[FILES_SCRIPT], G_FILE_TEST_IS_REGULAR)
        && g_file_get_contents(vb.files[FILES_SCRIPT], &js, NULL, &error)
    ) {
        char* value = NULL;
        gboolean success = vb_eval_script(frame, js, vb.files[FILES_SCRIPT], &value);
        if (!success) {
            fprintf(stderr, "%s", value);
        }
        g_free(value);
        g_free(js);
    }
}

static char* vb_jsref_to_string(JSContextRef context, JSValueRef ref)
{
    char* string;
    JSStringRef str_ref = JSValueToStringCopy(context, ref, NULL);
    size_t len          = JSStringGetMaximumUTF8CStringSize(str_ref);

    string = g_new0(char, len);
    JSStringGetUTF8CString(str_ref, string, len);
    JSStringRelease(str_ref);

    return string;
}

static void vb_init_core(void)
{
    Gui* gui = &vb.gui;

    if (vb.embed) {
        gui->window = gtk_plug_new(vb.embed);
    } else {
        gui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_wmclass(GTK_WINDOW(gui->window), "vimb", "Vimb");
        gtk_window_set_role(GTK_WINDOW(gui->window), "Vimb");
    }

    GdkGeometry hints = {10, 10};
    gtk_window_set_default_size(GTK_WINDOW(gui->window), 640, 480);
    gtk_window_set_title(GTK_WINDOW(gui->window), "vimb");
    gtk_window_set_geometry_hints(GTK_WINDOW(gui->window), NULL, &hints, GDK_HINT_MIN_SIZE);
    gtk_window_set_icon(GTK_WINDOW(gui->window), NULL);
    gtk_widget_set_name(GTK_WIDGET(gui->window), "vimb");

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

    /* init soup session */
    vb.soup_session = webkit_get_default_session();
    soup_session_remove_feature_by_type(vb.soup_session, soup_cookie_jar_get_type());
    g_object_set(vb.soup_session, "max-conns", SETTING_MAX_CONNS , NULL);
    g_object_set(vb.soup_session, "max-conns-per-host", SETTING_MAX_CONNS_PER_HOST, NULL);

    vb_setup_signals();

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

    vb_init_files();
    setting_init();
    command_init();
    keybind_init();
    vb_read_config();
    url_history_init();

    vb_update_status_style();
    vb_update_input_style(VB_MSG_NORMAL);

    /* make sure the main window and all its contents are visible */
    gtk_widget_show_all(gui->window);
}

static void vb_read_config(void)
{
    /* load default config */
    for (guint i = 0; default_config[i].command != NULL; i++) {
        if (!vb_process_input(default_config[i].command)) {
            fprintf(stderr, "Invalid default config: %s\n", default_config[i].command);
        }
    }

    /* read config from config files */
    char **lines = util_get_lines(vb.files[FILES_CONFIG]);
    char *line;

    if (lines) {
        int length = g_strv_length(lines) - 1;
        for (int i = 0; i < length; i++) {
            line = lines[i];
            g_strstrip(line);

            if (!g_ascii_isalpha(line[0])) {
                continue;
            }
            if (!vb_process_input(line)) {
                fprintf(stderr, "Invalid config: %s\n", line);
            }
        }
    }
    g_strfreev(lines);
}

static void vb_setup_signals()
{
    /* Set up callbacks so that if either the main window or the browser
     * instance is closed, the program will exit */
    g_signal_connect(vb.gui.window, "destroy", G_CALLBACK(vb_destroy_window_cb), NULL);
    g_object_connect(
        G_OBJECT(vb.gui.webview),
        "signal::notify::progress", G_CALLBACK(vb_webview_progress_cb), NULL,
        "signal::notify::load-status", G_CALLBACK(vb_webview_load_status_cb), NULL,
        "signal::button-release-event", G_CALLBACK(vb_button_relase_cb), NULL,
        "signal::new-window-policy-decision-requested", G_CALLBACK(vb_new_window_policy_cb), NULL,
        "signal::hovering-over-link", G_CALLBACK(vb_hover_link_cb), NULL,
        "signal::title-changed", G_CALLBACK(vb_title_changed_cb), NULL,
        "signal::mime-type-policy-decision-requested", G_CALLBACK(vb_mimetype_decision_cb), NULL,
        "signal::download-requested", G_CALLBACK(vb_download_requested_cb), NULL,
        "signal::resource-request-starting", G_CALLBACK(vb_request_start_cb), NULL,
        NULL
    );

    g_object_connect(
        G_OBJECT(vb.gui.inputbox),
        "signal::activate",          G_CALLBACK(vb_inputbox_activate_cb),   NULL,
        "signal::key-release-event", G_CALLBACK(vb_inputbox_keyrelease_cb), NULL,
        NULL
    );
    /* webview adjustment */
    g_object_connect(G_OBJECT(vb.gui.adjust_v),
        "signal::value-changed", G_CALLBACK(vb_scroll_cb), NULL,
        NULL
    );

    g_signal_connect_after(G_OBJECT(vb.soup_session), "request-started", G_CALLBACK(vb_new_request_cb), NULL);

    /* inspector */
    /* TODO use g_object_connect instead */
    g_signal_connect(G_OBJECT(vb.gui.inspector), "inspect-web-view", G_CALLBACK(vb_inspector_new), NULL);
    g_signal_connect(G_OBJECT(vb.gui.inspector), "show-window", G_CALLBACK(vb_inspector_show), NULL);
    g_signal_connect(G_OBJECT(vb.gui.inspector), "close-window", G_CALLBACK(vb_inspector_close), NULL);
    g_signal_connect(G_OBJECT(vb.gui.inspector), "finished", G_CALLBACK(vb_inspector_finished), NULL);
}

static void vb_init_files(void)
{
    char* path = util_get_config_dir();

    vb.files[FILES_CONFIG] = g_build_filename(path, "config", NULL);
    util_create_file_if_not_exists(vb.files[FILES_CONFIG]);

    vb.files[FILES_COOKIE] = g_build_filename(path, "cookies", NULL);
    util_create_file_if_not_exists(vb.files[FILES_COOKIE]);

    vb.files[FILES_CLOSED] = g_build_filename(path, "closed", NULL);
    util_create_file_if_not_exists(vb.files[FILES_CLOSED]);

    vb.files[FILES_HISTORY] = g_build_filename(path, "history", NULL);
    util_create_file_if_not_exists(vb.files[FILES_HISTORY]);

    vb.files[FILES_SCRIPT] = g_build_filename(path, "scripts.js", NULL);

    vb.files[FILES_USER_STYLE] = g_build_filename(path, "style.css", NULL);

    g_free(path);
}

static gboolean vb_button_relase_cb(WebKitWebView* webview, GdkEventButton* event)
{
    gboolean propagate = FALSE;
    WebKitHitTestResultContext context;
    Mode mode = CLEAN_MODE(vb.state.mode);

    WebKitHitTestResult *result = webkit_web_view_get_hit_test_result(webview, event);

    g_object_get(result, "context", &context, NULL);
    if (mode == VB_MODE_NORMAL && context & WEBKIT_HIT_TEST_RESULT_CONTEXT_EDITABLE) {
        vb_set_mode(VB_MODE_INSERT, FALSE);

        propagate = TRUE;
    }
    /* middle mouse click onto link */
    if (context & WEBKIT_HIT_TEST_RESULT_CONTEXT_LINK && event->button == 2) {
        Arg a = {VB_TARGET_NEW};
        g_object_get(result, "link-uri", &a.s, NULL);
        vb_load_uri(&a);

        propagate = TRUE;
    }
    g_object_unref(result);

    return propagate;
}

static gboolean vb_new_window_policy_cb(
    WebKitWebView* view, WebKitWebFrame* frame, WebKitNetworkRequest* request,
    WebKitWebNavigationAction* navig, WebKitWebPolicyDecision* policy)
{
    if (webkit_web_navigation_action_get_reason(navig) == WEBKIT_WEB_NAVIGATION_REASON_LINK_CLICKED) {
        webkit_web_policy_decision_ignore(policy);
        /* open in a new window */
        Arg a = {VB_TARGET_NEW, (char*)webkit_network_request_get_uri(request)};
        vb_load_uri(&a);
        return TRUE;
    }
    return FALSE;
}

static void vb_hover_link_cb(WebKitWebView* webview, const char* title, const char* link)
{
    if (link) {
        char* message = g_strdup_printf("Link: %s", link);
        gtk_label_set_text(GTK_LABEL(vb.gui.statusbar.left), message);
        g_free(message);
    } else {
        vb_update_urlbar(webkit_web_view_get_uri(webview));
    }
}

static void vb_title_changed_cb(WebKitWebView* webview, WebKitWebFrame* frame, const char* title)
{
    gtk_window_set_title(GTK_WINDOW(vb.gui.window), title);
}

static gboolean vb_mimetype_decision_cb(WebKitWebView* webview,
    WebKitWebFrame* frame, WebKitNetworkRequest* request, char*
    mime_type, WebKitWebPolicyDecision* decision)
{
    if (webkit_web_view_can_show_mime_type(webview, mime_type) == FALSE) {
        webkit_web_policy_decision_download(decision);

        return TRUE;
    }
    return FALSE;
}

static gboolean vb_download_requested_cb(WebKitWebView* view, WebKitDownload* download)
{
    WebKitDownloadStatus status;
    char* uri = NULL;

    const char* filename = webkit_download_get_suggested_filename(download);
    if (!filename) {
        filename = "vimb_donwload";
    }

    /* prepare the download target path */
    uri = g_strdup_printf("file://%s%c%s", vb.config.download_dir, G_DIR_SEPARATOR, filename);
    webkit_download_set_destination_uri(download, uri);
    g_free(uri);

    guint64 size = webkit_download_get_total_size(download);
    if (size > 0) {
        vb_echo(VB_MSG_NORMAL, FALSE, "Download %s [~%uB] started ...", filename, size);
    } else {
        vb_echo(VB_MSG_NORMAL, FALSE, "Download %s started ...", filename);
    }

    status = webkit_download_get_status(download);
    if (status == WEBKIT_DOWNLOAD_STATUS_CREATED) {
        webkit_download_start(download);
    }

    /* prepend the download to the download list */
    vb.state.downloads = g_list_prepend(vb.state.downloads, download);

    /* connect signal handler to check if the download is done */
    g_signal_connect(download, "notify::status", G_CALLBACK(vb_download_progress_cp), NULL);
    g_signal_connect(download, "notify::progress", G_CALLBACK(vb_webview_download_progress_cb), NULL);

    vb_update_statusbar();

    return TRUE;
}

/**
 * Callback to filter started resource request.
 */
static void vb_request_start_cb(WebKitWebView* webview, WebKitWebFrame* frame,
    WebKitWebResource* resource, WebKitNetworkRequest* request,
    WebKitNetworkResponse* response)
{
    const char* uri = webkit_network_request_get_uri(request);
    if (g_str_has_suffix(uri, "/favicon.ico")) {
        webkit_network_request_set_uri(request, "about:blank");
    }
}

static void vb_download_progress_cp(WebKitDownload* download, GParamSpec* pspec)
{
    WebKitDownloadStatus status = webkit_download_get_status(download);

    if (status == WEBKIT_DOWNLOAD_STATUS_STARTED || status == WEBKIT_DOWNLOAD_STATUS_CREATED) {
        return;
    }

    char* file = g_path_get_basename(webkit_download_get_destination_uri(download));
    if (status != WEBKIT_DOWNLOAD_STATUS_FINISHED) {
        vb_echo(VB_MSG_ERROR, FALSE, "Error downloading %s", file);
    } else {
        vb_echo(VB_MSG_NORMAL, FALSE, "Download %s finished", file);
    }
    g_free(file);

    /* remove the donwload from the list */
    vb.state.downloads = g_list_remove(vb.state.downloads, download);

    vb_update_statusbar();
}

static void vb_destroy_client()
{
    const char* uri = webkit_web_view_get_uri(vb.gui.webview);
    /* write last URL into file for recreation */
    if (uri) {
        g_file_set_contents(vb.files[FILES_CLOSED], uri, -1, NULL);
    }

    completion_clean();

    webkit_web_view_stop_loading(vb.gui.webview);
    gtk_widget_destroy(GTK_WIDGET(vb.gui.webview));
    gtk_widget_destroy(GTK_WIDGET(vb.gui.scroll));
    gtk_widget_destroy(GTK_WIDGET(vb.gui.box));
    gtk_widget_destroy(GTK_WIDGET(vb.gui.window));

    command_cleanup();
    setting_cleanup();
    keybind_cleanup();
    searchengine_cleanup();
    url_history_cleanup();

    for (int i = 0; i < FILES_LAST; i++) {
        g_free(vb.files[i]);
    }

    gtk_main_quit();
}

int main(int argc, char* argv[])
{
    static char* winid = NULL;
    static gboolean ver = false;
    static gboolean print_config = false;
    static GError* err;
    static GOptionEntry opts[] = {
        {"version", 'v', 0, G_OPTION_ARG_NONE, &ver, "Print version", NULL},
        {"embed", 'e', 0, G_OPTION_ARG_STRING, &winid, "Reparents to window specified by xid", NULL},
        {"print-config", 'D', 0, G_OPTION_ARG_NONE, &print_config, "Print the value of all configuration options to stdout", NULL},
        {NULL}
    };
    /* Initialize GTK+ */
    if (!gtk_init_with_args(&argc, &argv, "[URI]", opts, NULL, &err)) {
        g_printerr("can't init gtk: %s\n", err->message);
        g_error_free(err);

        return EXIT_FAILURE;
    }

    if (ver) {
        fprintf(stdout, "%s/%s (build %s %s)\n", PROJECT, VERSION, __DATE__, __TIME__);
        return EXIT_SUCCESS;
    }
    if (print_config) {
        /* load default config */
        for (guint i = 0; default_config[i].command != NULL; i++) {
            fprintf(stdout, "%s\n", default_config[i].command);
        }
        return EXIT_SUCCESS;
    }

    /* save arguments */
    args = argv;

    if (winid) {
        vb.embed = strtol(winid, NULL, 0);
    }

    vb_init_core();

    /* command line argument: URL */
    Arg arg = {VB_TARGET_CURRENT};
    if (argc > 1) {
        arg.s = g_strdup(argv[argc - 1]);
    } else {
        arg.s = g_strdup(vb.config.home_page);
    }
    vb_load_uri(&arg);
    g_free(arg.s);

    /* Run the main GTK+ event loop */
    gtk_main();

    return EXIT_SUCCESS;
}
