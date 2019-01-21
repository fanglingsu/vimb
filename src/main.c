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

#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <libsoup/soup.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <webkit2/webkit2.h>

#include "../version.h"
#include "ascii.h"
#include "command.h"
#include "completion.h"
#include "config.h"
#include "ex.h"
#include "ext-proxy.h"
#include "handler.h"
#include "history.h"
#include "input.h"
#include "js.h"
#include "main.h"
#include "map.h"
#include "normal.h"
#include "setting.h"
#include "shortcut.h"
#include "util.h"
#include "autocmd.h"

static void client_destroy(Client *c);
static Client *client_new(WebKitWebView *webview);
static void client_show(WebKitWebView *webview, Client *c);
static GtkWidget *create_window(Client *c);
static gboolean input_clear(Client *c);
static void input_print(Client *c, MessageType type, gboolean hide,
        const char *message);
static gboolean is_plausible_uri(const char *path);
static void marks_clear(Client *c);
static void mode_free(Mode *mode);
static void on_textbuffer_changed(GtkTextBuffer *textbuffer, gpointer user_data);
static void on_webctx_download_started(WebKitWebContext *webctx,
        WebKitDownload *download, Client *c);
static void on_webctx_init_web_extension(WebKitWebContext *webctx, gpointer data);
static gboolean on_webdownload_decide_destination(WebKitDownload *download,
        gchar *suggested_filename, Client *c);
static void on_webdownload_failed(WebKitDownload *download,
        GError *error, Client *c);
static void on_webdownload_finished(WebKitDownload *download, Client *c);
static void on_webdownload_received_data(WebKitDownload *download,
        guint64 data_length, Client *c);
static void on_webview_close(WebKitWebView *webview, Client *c);
static WebKitWebView *on_webview_create(WebKitWebView *webview,
        WebKitNavigationAction *navact, Client *c);
static gboolean on_webview_decide_policy(WebKitWebView *webview,
        WebKitPolicyDecision *dec, WebKitPolicyDecisionType type, Client *c);
static void on_webview_load_changed(WebKitWebView *webview,
        WebKitLoadEvent event, Client *c);
static void on_webview_mouse_target_changed(WebKitWebView *webview,
        WebKitHitTestResult *result, guint modifiers, Client *c);
static void on_webview_notify_estimated_load_progress(WebKitWebView *webview,
        GParamSpec *spec, Client *c);
static void on_webview_notify_title(WebKitWebView *webview, GParamSpec *pspec,
        Client *c);
static void on_webview_notify_uri(WebKitWebView *webview, GParamSpec *pspec,
        Client *c);
static void on_webview_ready_to_show(WebKitWebView *webview, Client *c);
static gboolean on_webview_web_process_crashed(WebKitWebView *webview, Client *c);
static gboolean on_webview_authenticate(WebKitWebView *webview,
        WebKitAuthenticationRequest *request, Client *c);
static gboolean on_webview_enter_fullscreen(WebKitWebView *webview, Client *c);
static gboolean on_webview_leave_fullscreen(WebKitWebView *webview, Client *c);
static gboolean on_window_delete_event(GtkWidget *window, GdkEvent *event, Client *c);
static void on_window_destroy(GtkWidget *window, Client *c);
static gboolean quit(Client *c);
static void read_from_stdin(Client *c);
static void register_cleanup(Client *c);
static void update_title(Client *c);
static void update_urlbar(Client *c);
static void set_statusbar_style(Client *c, StatusType type);
static void set_title(Client *c, const char *title);
static void spawn_new_instance(const char *uri);
#ifdef FREE_ON_QUIT
static void vimb_cleanup(void);
#endif
static void vimb_setup(void);
static WebKitWebView *webview_new(Client *c, WebKitWebView *webview);
static void on_counted_matches(WebKitFindController *finder, guint count, Client *c);
static gboolean on_permission_request(WebKitWebView *webview,
        WebKitPermissionRequest *request, Client *c);
static void on_script_message_focus(WebKitUserContentManager *manager,
        WebKitJavascriptResult *res, gpointer data);
static gboolean profileOptionArgFunc(const gchar *option_name,
        const gchar *value, gpointer data, GError **error);

struct Vimb vb;

/**
 * Set the destination for a download according to suggested file name and
 * possible given path.
 */
gboolean vb_download_set_destination(Client *c, WebKitDownload *download,
    char *suggested_filename, const char *path)
{
    char *download_path, *dir, *file, *uri, *basename = NULL,
         *decoded_uri = NULL;
    const char *download_uri;
    download_path = GET_CHAR(c, "download-path");

    if (!suggested_filename || !*suggested_filename) {
        /* Try to find a matching name if there is no suggested filename. */
        download_uri = webkit_uri_request_get_uri(webkit_download_get_request(download));
        decoded_uri  = soup_uri_decode(download_uri);
        basename     = g_filename_display_basename(decoded_uri);
        g_free(decoded_uri);

        suggested_filename = basename;
    }

    /* Prepare the path to save the download. */
    if (path && *path) {
        file = util_build_path(c->state, path, download_path);

        /* if file is an directory append a file name */
        if (g_file_test(file, (G_FILE_TEST_IS_DIR))) {
            dir  = file;
            file = g_build_filename(dir, suggested_filename, NULL);
            g_free(dir);
        }
    } else {
        file = util_build_path(c->state, suggested_filename, download_path);
    }

    g_free(basename);

    if (!file) {
        return FALSE;
    }

    /* If the filepath exists already insert numerical suffix before file
     * extension. */
    if (g_file_test(file, G_FILE_TEST_EXISTS)) {
        const char *dot_pos;
        char *num = NULL;
        GString *tmp;
        gssize suffix;
        int i = 1;

        /* position on .tar. (special case, extension with two dots),
         * position on last dot (if any) otherwise */
        if (!(dot_pos = strstr(file, ".tar."))) {
            dot_pos = strrchr(file, '.');
        }

        /* the position to insert the suffix at */
        if (dot_pos) {
            suffix = dot_pos - file;
        } else {
            suffix = strlen(file);
        }

        tmp = g_string_new(NULL);

        /* Construct a new complete download filepath with suffix before the
         * file extension. */
        do {
            num = g_strdup_printf("_%d", i++);
            g_string_assign(tmp, file);
            g_string_insert(tmp, suffix, num);
            g_free(num);
        } while (g_file_test(tmp->str, G_FILE_TEST_EXISTS));

        file = g_strdup(tmp->str);
        g_string_free(tmp, TRUE);
    }

    /* Build URI from filepath. */
    uri = g_filename_to_uri(file, NULL, NULL);
    g_free(file);

    /* configure download */
    g_assert(uri);
    webkit_download_set_allow_overwrite(download, FALSE);
    webkit_download_set_destination(download, uri);
    g_free(uri);

    return TRUE;
}

/**
 * Write text to the inpubox if this isn't focused.
 */
void vb_echo(Client *c, MessageType type, gboolean hide, const char *error, ...)
{
    char *buffer;
    va_list args;
    /* Don't write to input box in case this is focused, might be the user is
     * typing in it. */
    if (gtk_widget_is_focus(GTK_WIDGET(c->input))) {
        return;
    }

    va_start(args, error);
    buffer = g_strdup_vprintf(error, args);
    va_end(args);

    input_print(c, type, hide, buffer);
    g_free(buffer);
}

/**
 * Write text to the inpubox independent if this is focused or not.
 * Note that this could disturb the user during typing into inputbox.
 */
void vb_echo_force(Client *c, MessageType type, gboolean hide, const char *error, ...)
{
    char *buffer;
    va_list args;

    va_start(args, error);
    buffer = g_strdup_vprintf(error, args);
    va_end(args);

    input_print(c, type, hide, buffer);
    g_free(buffer);
}

/**
 * Enter into the new given mode and leave possible active current mode.
 */
void vb_enter(Client *c, char id)
{
    Mode *new = g_hash_table_lookup(vb.modes, GINT_TO_POINTER(id));

    g_return_if_fail(new != NULL);

    if (c->mode) {
        /* don't do anything if the mode isn't a new one */
        if (c->mode == new) {
            return;
        }

        /* if there is a active mode, leave this first */
        if (c->mode->leave) {
            c->mode->leave(c);
        }
    }

    /* reset the flags of the new entered mode */
    new->flags = 0;

    /* set the new mode so that it is available also in enter function */
    c->mode = new;
    /* call enter only if the new mode isn't the current mode */
    if (new->enter) {
        new->enter(c);
    }

#ifndef TESTLIB
    vb_statusbar_update(c);
#endif
}

/**
 * Set the prompt chars and switch to new mode.
 *
 * @id:           Mode id.
 * @prompt:       Prompt string to set as current prompt.
 * @print_prompt: Indicates if the new set prompt should be put into inputbox
 *                after switching the mode.
 */
void vb_enter_prompt(Client *c, char id, const char *prompt, gboolean print_prompt)
{
    /* set the prompt to be accessible in vb_enter */
    strncpy(c->state.prompt, prompt, PROMPT_SIZE - 1);
    c->state.prompt[PROMPT_SIZE - 1] = '\0';

    vb_enter(c, id);

    if (print_prompt) {
        /* set it after the mode was entered so that the modes input change
         * event listener could grep the new prompt */
        vb_echo_force(c, MSG_NORMAL, FALSE, c->state.prompt);
    }
}

/**
 * Returns the client for given page id.
 */
Client *vb_get_client_for_page_id(guint64 pageid)
{
    Client *c;
    /* Search for the client with the same page id. */
    for (c = vb.clients; c && c->page_id != pageid; c = c->next);

    if (c) {
        return c;
    }
    return NULL;
}

/**
 * Retrieves the content of the command line.
 * Returned string must be freed with g_free.
 */
char *vb_input_get_text(Client *c)
{
    GtkTextIter start, end;

    gtk_text_buffer_get_bounds(c->buffer, &start, &end);
    return gtk_text_buffer_get_text(c->buffer, &start, &end, FALSE);
}

/**
 * Writes given text into the command line.
 */
void vb_input_set_text(Client *c, const char *text)
{
    gtk_text_buffer_set_text(c->buffer, text, -1);
    if (c->config.input_autohide) {
        gtk_widget_set_visible(GTK_WIDGET(c->input), *text != '\0');
    }
}

/**
 * Set the style of the inputbox according to current input type (normal or
 * error).
 */
void vb_input_update_style(Client *c)
{
    MessageType type = c->state.input_type;

    if (type == MSG_ERROR) {
        gtk_style_context_add_class(gtk_widget_get_style_context(c->input), "error");
    } else {
        gtk_style_context_remove_class(gtk_widget_get_style_context(c->input), "error");
    }
}

/**
 * Load the a uri given in Arg. This function handles also shortcuts and local
 * file paths.
 *
 * If arg.i = TARGET_CURRENT, the url is opened into the current webview.
 * TARGET_RELATED causes the generation of a new window within the current
 * instance of vimb with a own, but related webview. And TARGET_NEW spawns a
 * new instance of vimb with the given uri.
 */
gboolean vb_load_uri(Client *c, const Arg *arg)
{
    char *uri = NULL, *rp, *path = NULL;
    struct stat st;

    if (arg->s) {
        path = g_strstrip(arg->s);
    }
    if (!path || !*path) {
        path = GET_CHAR(c, "home-page");
    }

    /* If path contains :// but no space we open it direct. This is required
     * to use :// also with shortcuts */
    if ((strstr(path, "://") && !strchr(path, ' ')) || !strncmp(path, "about:", 6)) {
        uri = g_strdup(path);
    } else if (stat(path, &st) == 0) {
        /* check if the path is a file path */
        rp  = realpath(path, NULL);
        uri = g_strconcat("file://", rp, NULL);
        free(rp);
    } else if (!is_plausible_uri(path)) {
        /* use a shortcut if path contains spaces or doesn't contain typical
         * tokens ('.', [:] for IPv6 addresses, 'localhost') */
        uri = shortcut_get_uri(c->config.shortcuts, path);
    }

    if (!uri) {
        uri = g_strconcat("http://", path, NULL);
    }

    if (arg->i == TARGET_CURRENT) {
        /* Load the uri into the browser instance. */
        webkit_web_view_load_uri(c->webview, uri);
        set_title(c, uri);
    } else if (arg->i == TARGET_NEW) {
        spawn_new_instance(uri);
    } else { /* TARGET_RELATED */
        Client *newclient = client_new(c->webview);
        /* Load the uri into the new client. */
        webkit_web_view_load_uri(newclient->webview, uri);
        set_title(c, uri);
    }
    g_free(uri);

    return TRUE;
}

/**
 * Creates and add a new mode with given callback functions.
 */
void vb_mode_add(char id, ModeTransitionFunc enter, ModeTransitionFunc leave,
    ModeKeyFunc keypress, ModeInputChangedFunc input_changed)
{
    Mode *new = g_slice_new(Mode);
    new->id            = id;
    new->enter         = enter;
    new->leave         = leave;
    new->keypress      = keypress;
    new->input_changed = input_changed;
    new->flags         = 0;

    /* Initialize the hashmap if this was not done before */
    if (!vb.modes) {
        vb.modes = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)mode_free);
    }
    g_hash_table_insert(vb.modes, GINT_TO_POINTER(id), new);
}

VbResult vb_mode_handle_key(Client *c, int key)
{
    VbResult res;

    if (c->state.ctrlv) {
        c->state.processed_key = FALSE;
        c->state.ctrlv         = FALSE;

        return RESULT_COMPLETE;
    }
    if (c->mode->id != 'p' && key == CTRL('V')) {
        c->mode->flags |= FLAG_NOMAP;
        c->state.ctrlv  = TRUE;

        return RESULT_MORE;
    }

    if (c->mode && c->mode->keypress) {
#ifdef DEBUGDISABLED
        int flags = c->mode->flags;
        int id    = c->mode->id;
        res = c->mode->keypress(c, key);
        if (c->mode) {
            PRINT_DEBUG(
                "%c[%d]: %#.2x '%c' -> %c[%d]",
                id - ' ', flags, key, (key >= 0x20 && key <= 0x7e) ? key : ' ',
                c->mode->id - ' ', c->mode->flags
            );
        }
#else
        res = c->mode->keypress(c, key);
#endif
        return res;
    }
    return RESULT_ERROR;
}

/**
 * Change the label for the current mode in inputbox or on the left of
 * statusbar if inputbox is in autohide mode.
 */
void vb_modelabel_update(Client *c, const char *label)
{
    if (c->config.input_autohide) {
        /* if the inputbox is potentially not shown write mode into statusbar */
        gtk_label_set_text(GTK_LABEL(c->statusbar.mode), label);
    } else {
        vb_echo(c, MSG_NORMAL, FALSE, "%s", label);
    }
}

/**
 * Close the given client instances window.
 */
gboolean vb_quit(Client *c, gboolean force)
{
    /* if not forced quit - don't quit if there are still running downloads */
    if (!force && c->state.downloads) {
        vb_echo_force(c, MSG_ERROR, TRUE, "Can't quit: there are running downloads. Use :q! to force quit");
        return FALSE;
    }

    /* Don't run the quit synchronously, because this could lead to access of
     * no more existing widget where some command response is written. */
    g_idle_add((GSourceFunc)quit, c);

    return TRUE;
}

/**
 * Adds content to a named register.
 */
void vb_register_add(Client *c, char buf, const char *value)
{
    char *mark;
    int idx;

    if (!c->state.enable_register || !buf) {
        return;
    }

    /* make sure the mark is a valid mark char */
    if ((mark = strchr(REG_CHARS, buf))) {
        /* get the index of the mark char */
        idx = mark - REG_CHARS;

        OVERWRITE_STRING(c->state.reg[idx], value);
    }
}

/**
 * Lookup register entry by it's name.
 */
const char *vb_register_get(Client *c, char buf)
{
    char *mark;
    int idx;

    /* make sure the mark is a valid mark char */
    if ((mark = strchr(REG_CHARS, buf))) {
        /* get the index of the mark char */
        idx = mark - REG_CHARS;

        return c->state.reg[idx];
    }

    return NULL;
}

static void statusbar_update_downloads(Client *c, GString *status)
{
    GList *list;
    guint list_length, remaining_max = 0;
    gdouble progress, elapsed, total, remaining;
    WebKitDownload *download;

    g_assert(c);
    g_assert(status);

    if (c->state.downloads) {
        list_length = g_list_length(c->state.downloads);
        g_assert(list_length);

        /* get highest ETA value of all downloads based on each download's
         * current progress fraction and time elapsed */
        for (list = c->state.downloads; list != NULL; list = list->next) {
            download = (WebKitDownload *)list->data;
            g_assert(download);

            progress = webkit_download_get_estimated_progress(download);

            /* avoid dividing by zero */
            if (progress == 0.0) {
                continue;
            }

            elapsed = webkit_download_get_elapsed_time(download);
            total = (1.0 / progress) * elapsed;
            remaining = total - elapsed;

            remaining_max = MAX(remaining, remaining_max);
        }

        g_string_append_printf(status, " %d %s (ETA %us)",
                list_length, list_length == 1? "dnld" : "dnlds", remaining_max);
    }
}

void vb_statusbar_update(Client *c)
{
    GString *status;

    if (!gtk_widget_get_visible(GTK_WIDGET(c->statusbar.box))) {
        return;
    }

    status = g_string_new("");

    /* show the number of matches search results */
    if (c->state.search.matches) {
        g_string_append_printf(status, " (%d)", c->state.search.matches);
    }

    /* show load status of page or the downloads */
    if (c->state.progress != 100) {
#ifdef FEATURE_WGET_PROGRESS_BAR
        char bar[PROGRESS_BAR_LEN + 1];
        int i, state;

        state = c->state.progress * PROGRESS_BAR_LEN / 100;
        for (i = 0; i < state; i++) {
            bar[i] = PROGRESS_BAR[0];
        }
        bar[i++] = PROGRESS_BAR[1];
        for (; i < PROGRESS_BAR_LEN; i++) {
            bar[i] = PROGRESS_BAR[2];
        }
        bar[i] = '\0';
        g_string_append_printf(status, " [%s]", bar);
#else
        g_string_append_printf(status, " [%i%%]", c->state.progress);
#endif
    }

    statusbar_update_downloads(c, status);

    /* show the scroll status */
    if (c->state.scroll_max == 0) {
        g_string_append(status, " All");
    } else if (c->state.scroll_percent == 0) {
        g_string_append(status, " Top");
    } else if (c->state.scroll_percent == 100) {
        g_string_append(status, " Bot");
    } else {
        g_string_append_printf(status, " %d%%", c->state.scroll_percent);
    }

    gtk_label_set_text(GTK_LABEL(c->statusbar.right), status->str);
    g_string_free(status, TRUE);
}

/**
 * Destroys given client and removed it from client queue. If no client is
 * there in queue, quit the gtk main loop.
 */
static void client_destroy(Client *c)
{
    Client *p;
    webkit_web_view_stop_loading(c->webview);

    /* Write last URL into file for recreation.
     * The URL is only stored if the closed-max-items is not 0 and the file
     * exists. */
    if (c->state.uri && vb.config.closed_max && vb.files[FILES_CLOSED]) {
        util_file_prepend_line(vb.files[FILES_CLOSED], c->state.uri,
                vb.config.closed_max);
    }

    gtk_widget_destroy(c->window);

    /* Look for the client in the list, if we searched through the list and
     * didn't find it the client must be the first item. */
    for (p = vb.clients; p && p->next != c; p = p->next);
    if (p) {
        p->next = c->next;
    } else {
        vb.clients = c->next;
    }

    if (c->state.search.last_query) {
        g_free(c->state.search.last_query);
    }

    completion_cleanup(c);
    map_cleanup(c);
    register_cleanup(c);
    setting_cleanup(c);
#ifdef FEATURE_AUTOCMD
    autocmd_cleanup(c);
#endif
    handler_free(c->handler);
    shortcut_free(c->config.shortcuts);

    g_slice_free(Client, c);

    /* if there are no clients - quit the main loop */
    if (!vb.clients) {
        gtk_main_quit();
    }
}

/**
 * Creates a new client instance with it's own window.
 *
 * @webview:    Related webview or NULL if a client with an independent
 *              webview shoudl be created.
 */
static Client *client_new(WebKitWebView *webview)
{
    Client *c;

    /* create the client */
    /* Prepend the new client to the queue of clients. */
    c          = g_slice_new0(Client);
    c->next    = vb.clients;
    vb.clients = c;

    c->state.progress = 100;
    c->config.shortcuts = shortcut_new();

    completion_init(c);
    map_init(c);
    c->handler = handler_new();
#ifdef FEATURE_AUTOCMD
    autocmd_init(c);
#endif

    /* webview */
    c->webview   = webview_new(c, webview);
    c->finder    = webkit_web_view_get_find_controller(c->webview);
    g_signal_connect(c->finder, "counted-matches", G_CALLBACK(on_counted_matches), c);

    c->page_id   = webkit_web_view_get_page_id(c->webview);
    c->inspector = webkit_web_view_get_inspector(c->webview);

    return c;
}

static void client_show(WebKitWebView *webview, Client *c)
{
    GtkWidget *box;
    char *xid;

    c->window = create_window(c);

    /* statusbar */
    c->statusbar.box   = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
    c->statusbar.mode  = gtk_label_new(NULL);
    c->statusbar.left  = gtk_label_new(NULL);
    c->statusbar.right = gtk_label_new(NULL);
    c->statusbar.cmd   = gtk_label_new(NULL);
    gtk_widget_set_name(GTK_WIDGET(c->statusbar.box), "statusbar");
    gtk_label_set_ellipsize(GTK_LABEL(c->statusbar.left), PANGO_ELLIPSIZE_MIDDLE);
    gtk_widget_set_halign(c->statusbar.left, GTK_ALIGN_START);
    gtk_widget_set_halign(c->statusbar.mode, GTK_ALIGN_START);

    gtk_box_pack_start(c->statusbar.box, c->statusbar.mode, FALSE, TRUE, 0);
    gtk_box_pack_start(c->statusbar.box, c->statusbar.left, TRUE, TRUE, 2);
    gtk_box_pack_start(c->statusbar.box, c->statusbar.cmd, FALSE, FALSE, 0);
    gtk_box_pack_start(c->statusbar.box, c->statusbar.right, FALSE, FALSE, 2);

    /* inputbox */
    c->input  = gtk_text_view_new();
    gtk_widget_set_name(c->input, "input");
    c->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(c->input));
    g_signal_connect(c->buffer, "changed", G_CALLBACK(on_textbuffer_changed), c);
    /* Make sure the user can see the typed text. */
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(c->input), GTK_WRAP_WORD_CHAR);

    /* pack the parts together */
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(c->window), box);
    gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(c->webview), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(c->statusbar.box), FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(box), GTK_WIDGET(c->input), FALSE, FALSE, 0);

    /* Set the default style for statusbar and inputbox. */
    gtk_style_context_add_provider(gtk_widget_get_style_context(GTK_WIDGET(c->statusbar.box)),
            GTK_STYLE_PROVIDER(vb.style_provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_style_context_add_provider(gtk_widget_get_style_context(c->input),
            GTK_STYLE_PROVIDER(vb.style_provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    /* TODO separate initialization o setting from applying the values or
     * allow to se the default values for different scopes. For now we can
     * init the settings not in client_new because we need the access to some
     * widget for some settings. */
    setting_init(c);

    gtk_widget_show_all(c->window);
    if (vb.embed) {
        xid = g_strdup_printf("%d", (int)vb.embed);
    } else {
        xid = g_strdup_printf("%d", (int)GDK_WINDOW_XID(gtk_widget_get_window(c->window)));
    }

    /* set the x window id to env */
    g_setenv("VIMB_XID", xid, TRUE);
    g_free(xid);

    /* start client in normal mode */
    vb_enter(c, 'n');

    c->state.enable_register = TRUE;

    /* read the config file */
    ex_run_file(c, vb.files[FILES_CONFIG]);
}

static GtkWidget *create_window(Client *c)
{
    GtkWidget *window;

    if (vb.embed) {
        window = gtk_plug_new(vb.embed);
    } else {
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_role(GTK_WINDOW(window), PROJECT_UCFIRST);
        gtk_window_set_default_size(GTK_WINDOW(window), WIN_WIDTH, WIN_HEIGHT);
        gtk_window_maximize(GTK_WINDOW(window));
    }

    g_object_connect(
            G_OBJECT(window),
            "signal::destroy", G_CALLBACK(on_window_destroy), c,
            "signal::delete-event", G_CALLBACK(on_window_delete_event), c,
            "signal::key-press-event", G_CALLBACK(on_map_keypress), c,
            NULL);

    return window;
}

/**
 * Callback that clear the input box after a timeout if this was set on
 * input_print.
 */
static gboolean input_clear(Client *c)
{
    if (!gtk_widget_is_focus(GTK_WIDGET(c->input))) {
        return FALSE;
    }
    input_print(c, MSG_NORMAL, FALSE, "");

    return FALSE;
}

/**
 * Print a message to the input box.
 *
 * @type: Type of message normal or error
 * @hide: If TRUE the inputbox is cleared after a short timeout.
 * @message: The message to print.
 */
static void input_print(Client *c, MessageType type, gboolean hide,
        const char *message)
{
    /* apply input style only if the message type was changed */
    if (type != c->state.input_type) {
        c->state.input_type = type;
        vb_input_update_style(c);
    }

    vb_input_set_text(c, message);

    if (hide) {
        /* add timeout function */
        c->state.input_timer = g_timeout_add_seconds(MESSAGE_TIMEOUT, (GSourceFunc)input_clear, c);
    } else if (c->state.input_timer > 0) {
        /* If there is already a timeout function but the input box content is
         * changed - remove the timeout. Seems the user started another
         * command or typed into inputbox. */
        g_source_remove(c->state.input_timer);
        c->state.input_timer = 0;
    }
}

/**
 * Tests if a path is likely intended to be an URI (given that it's not a file
 * path or containing "://").
 */
static gboolean is_plausible_uri(const char *path)
{
    const char *i, *j;
    if (strchr(path, ' ')) {
        return FALSE;
    }
    if (strchr(path, '.')) {
        return TRUE;
    }
    if ((i = strstr(path, "localhost")) &&
        (i == path || i[-1] == '/' || i[-1] == '@') &&
        (i[9] == 0 || i[9]  == '/' || i[9] == ':')
    ) {
        return TRUE;
    }
    return (i = strchr(path, '[')) && (j = strchr(i, ':')) && strchr(j, ']');
}

/**
 * Reinitializes or clears the set page marks.
 */
static void marks_clear(Client *c)
{
    int i;

    /* init empty marks array */
    for (i = 0; i < MARK_SIZE; i++) {
        c->state.marks[i] = -1;
    }
}

/**
 * Free the memory of given mode. This is used as destroy function of the
 * modes hashmap.
 */
static void mode_free(Mode *mode)
{
    g_slice_free(Mode, mode);
}

/**
 * The ::changed signal is emitted when the content of a GtkTextBuffer has
 * changed. This call back function is connected to the input box' text buffer.
 */
static void on_textbuffer_changed(GtkTextBuffer *textbuffer, gpointer user_data)
{
    gchar *text;
    GtkTextIter start, end;
    Client *c = (Client *)user_data;

    g_assert(c);

    /* don't observe changes in completion mode */
    if (c->mode->flags & FLAG_COMPLETION) {
        return;
    }

    /* don't process changes not typed by the user */
    if (gtk_widget_is_focus(c->input) && c->mode && c->mode->input_changed) {

        gtk_text_buffer_get_bounds(textbuffer, &start, &end);
        text = gtk_text_buffer_get_text(textbuffer, &start, &end, FALSE);

        c->mode->input_changed(c, text);

        g_free(text);
    }
}

/**
 * Set the style of the statusbar.
 */
static void set_statusbar_style(Client *c, StatusType type)
{
    GtkStyleContext *ctx;
    /* Do nothing if the new to set style is the same as the current. */
    if (type == c->state.status_type) {
        return;
    }

    ctx = gtk_widget_get_style_context(GTK_WIDGET(c->statusbar.box));

    if (type == STATUS_SSL_VALID) {
        gtk_style_context_remove_class(ctx, "unsecure");
        gtk_style_context_add_class(ctx, "secure");
    } else if (type == STATUS_SSL_INVALID) {
        gtk_style_context_remove_class(ctx, "secure");
        gtk_style_context_add_class(ctx, "unsecure");
    } else {
        gtk_style_context_remove_class(ctx, "secure");
        gtk_style_context_remove_class(ctx, "unsecure");
    }
    c->state.status_type = type;
}

/**
 * Update the window title of the main window.
 */
static void set_title(Client *c, const char *title)
{
    OVERWRITE_STRING(c->state.title, title);
    update_title(c);
    g_setenv("VIMB_TITLE", title ? title : "", TRUE);
}

/**
 * Spawns a new browser instance for given uri.
 *
 * @uri:    URI used for the new instance.
 */
static void spawn_new_instance(const char *uri)
{
    guint i = 0;
    /* memory allocation */
    char **cmd = g_malloc_n(
        3                       /* basename + uri + ending NULL */
        + (vb.configfile ? 2 : 0)
#ifndef FEATURE_NO_XEMBED
        + (vb.embed ? 2 : 0)
#endif
        + (vb.profile ? 2 : 0),
        sizeof(char *)
    );

    cmd[i++] = vb.argv0;

    if (vb.configfile) {
        cmd[i++] = "-c";
        cmd[i++] = vb.configfile;
    }
#ifndef FEATURE_NO_XEMBED
    if (vb.embed) {
        char xid[64];
        cmd[i++] = "-e";
        snprintf(xid, LENGTH(xid), "%d", (int)vb.embed);
        cmd[i++] = xid;
    }
#endif
    if (vb.profile) {
        cmd[i++] = "-p";
        cmd[i++] = vb.profile;
    }
    cmd[i++] = (char*)uri;
    cmd[i++] = NULL;

    /* spawn a new browser instance */
    g_spawn_async(NULL, cmd, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL);

    /* free commandline */
    g_free(cmd);
}

/**
 * Callback for the web contexts download-started signal.
 */
static void on_webctx_download_started(WebKitWebContext *webctx,
        WebKitDownload *download, Client *c)
{
#ifdef FEATURE_AUTOCMD
    const char *uri = webkit_uri_request_get_uri(webkit_download_get_request(download));
    autocmd_run(c, AU_DOWNLOAD_STARTED, uri, NULL);
#endif

    g_signal_connect(download, "decide-destination", G_CALLBACK(on_webdownload_decide_destination), c);
    g_signal_connect(download, "failed", G_CALLBACK(on_webdownload_failed), c);
    g_signal_connect(download, "finished", G_CALLBACK(on_webdownload_finished), c);
    g_signal_connect(download, "received-data", G_CALLBACK(on_webdownload_received_data), c);

    c->state.downloads = g_list_append(c->state.downloads, download);

    /* to reflect the correct download count */
    vb_statusbar_update(c);
}

/**
 * Callback for the web contexts initialize-web-extensions signal.
 */
static void on_webctx_init_web_extension(WebKitWebContext *webctx, gpointer data)
{
    const char *name;
    GVariant *vdata;

#if (CHECK_WEBEXTENSION_ON_STARTUP)
    char *extension = g_build_filename(EXTENSIONDIR,  "webext_main.so", NULL);
    if (!g_file_test(extension, G_FILE_TEST_IS_REGULAR)) {
        g_error("Cannot access web extension %s", extension);
    }
    g_free(extension);
#endif

    name  = ext_proxy_init();
    vdata = g_variant_new("(ms)", name);
    webkit_web_context_set_web_extensions_initialization_user_data(webctx, vdata);

    /* Setup the extension directory. */
    webkit_web_context_set_web_extensions_directory(webctx, EXTENSIONDIR);
}

/**
 * Callback for the webkit download decide destination signal.
 * This signal is emitted after response is received to decide a destination
 * URI for the download.
 */
static gboolean on_webdownload_decide_destination(WebKitDownload *download,
        gchar *suggested_filename, Client *c)
{
    if (webkit_download_get_destination(download)) {
        return TRUE;
    }

    return vb_download_set_destination(c, download, suggested_filename, NULL);
}

/**
 * Callback for the webkit download failed signal.
 * This signal is emitted when an error occurs during the download operation.
 */
static void on_webdownload_failed(WebKitDownload *download,
               GError *error, Client *c)
{
    gchar *destination = NULL, *filename = NULL, *basename = NULL;

    g_assert(download);
    g_assert(error);
    g_assert(c);

#ifdef FEATURE_AUTOCMD
    const char *uri = webkit_uri_request_get_uri(webkit_download_get_request(download));
    autocmd_run(c, AU_DOWNLOAD_FAILED, uri, NULL);
#endif

    /* get the failed download's destination uri */
    g_object_get(download, "destination", &destination, NULL);
    g_assert(destination);

    /* filename from uri */
    if (destination) {
        filename = g_filename_from_uri(destination, NULL, NULL);
        g_free(destination);
    }

    /* basename from filename */
    if (filename) {
        basename = g_path_get_basename(filename);
        g_free(filename);
    }

    /* report the error to the user */
    if (basename) {
        vb_echo(c, MSG_ERROR, FALSE, "Download of %s failed (%s)", basename, error->message);
        g_free(basename);
    }
}

/**
 * Callback for the webkit download finished signal.
 * This signal is emitted when download finishes successfully or due to an
 * error. In case of errors “failed” signal is emitted before this one.
 */
static void on_webdownload_finished(WebKitDownload *download, Client *c)
{
    gchar *destination = NULL, *filename = NULL, *basename = NULL;

    g_assert(download);
    g_assert(c);

#ifdef FEATURE_AUTOCMD
    const char *uri = webkit_uri_request_get_uri(webkit_download_get_request(download));
    autocmd_run(c, AU_DOWNLOAD_FINISHED, uri, NULL);
#endif

    c->state.downloads = g_list_remove(c->state.downloads, download);

    /* to reflect the correct download count */
    vb_statusbar_update(c);

    /* get the finished downloads destination uri */
    g_object_get(download, "destination", &destination, NULL);
    g_assert(destination);

    /* filename from uri */
    if (destination) {
        filename = g_filename_from_uri(destination, NULL, NULL);
        g_free(destination);
    }

    if (filename) {
        /* basename from filename */
        basename = g_path_get_basename(filename);

        if (basename) {
            /* Only report to the user if the downloaded file exists, so the
             * download was successful. Otherwise, this is a failed download
             * finished signal and it was reported to the user in
             * on_webdownload_failed() already. */
            if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
                vb_echo(c, MSG_NORMAL, FALSE, "Download of %s finished", basename);
            }

            g_free(basename);
        }

        g_free(filename);
    }
}

/**
 * Callback for the webkit download received-data signal.
 * This signal is emitted after response is received, every time new data has
 * been written to the destination. It's useful to know the progress of the
 * download operation.
 */
static void on_webdownload_received_data(WebKitDownload *download,
        guint64 data_length, Client *c)
{
    /* rate limit statusbar updates */
    static gint64 statusbar_update_next = 0;

    if (g_get_monotonic_time() > statusbar_update_next) {
        statusbar_update_next = g_get_monotonic_time() + 1000000; /* 1 second */

        vb_statusbar_update(c);
    }
}

/**
 * Callback for the webview close signal.
 */
static void on_webview_close(WebKitWebView *webview, Client *c)
{
    client_destroy(c);
}

/**
 * Callback for the webview create signal.
 * This creates a new client - with it's own window with a related webview.
 */
static WebKitWebView *on_webview_create(WebKitWebView *webview,
        WebKitNavigationAction *navact, Client *c)
{
    Client *new = client_new(webview);

    return new->webview;
}

/**
 * Callback for the webview decide-policy signal.
 * Checks the reasons for some navigation actions and decides if the action is
 * allowed, or should go into a new instance of vimb.
 */
static gboolean on_webview_decide_policy(WebKitWebView *webview,
        WebKitPolicyDecision *dec, WebKitPolicyDecisionType type, Client *c)
{
    guint status, button, mod;
    WebKitNavigationAction *a;
    WebKitURIRequest *req;
    WebKitURIResponse *res;
    const char *uri;

    switch (type) {
        case WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION:
            a      = webkit_navigation_policy_decision_get_navigation_action(WEBKIT_NAVIGATION_POLICY_DECISION(dec));
            req    = webkit_navigation_action_get_request(a);
            button = webkit_navigation_action_get_mouse_button(a);
            mod    = webkit_navigation_action_get_modifiers(a);
            uri    = webkit_uri_request_get_uri(req);

            /* Try to handle with specific protocol handler. */
            if (handler_handle_uri(c->handler, uri)) {
                webkit_policy_decision_ignore(dec);
                return TRUE;
            }
            /* Spawn new instance if the new win flag is set on the mode, or
             * the navigation was triggered by CTRL-LeftMouse or MiddleMouse. */
            if ((c->mode->flags & FLAG_NEW_WIN)
                || (webkit_navigation_action_get_navigation_type(a) == WEBKIT_NAVIGATION_TYPE_LINK_CLICKED
                    && (button == 2 || (button == 1 && mod & GDK_CONTROL_MASK)))) {

                /* Remove the FLAG_NEW_WIN after the first use. */
                c->mode->flags &= ~FLAG_NEW_WIN;

                webkit_policy_decision_ignore(dec);
                spawn_new_instance(uri);
                return TRUE;
            }
            return FALSE;

        case WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION:
            a   = webkit_navigation_policy_decision_get_navigation_action(WEBKIT_NAVIGATION_POLICY_DECISION(dec));

            /* Ignore opening new window if this was started without user gesture. */
            if (!webkit_navigation_action_is_user_gesture(a)) {
                webkit_policy_decision_ignore(dec);
                return TRUE;
            }

            if (webkit_navigation_action_get_navigation_type(a) == WEBKIT_NAVIGATION_TYPE_LINK_CLICKED) {
                webkit_policy_decision_ignore(dec);
                /* This is triggered on link click for links with
                 * target="_blank". Maybe it should be configurable if the
                 * page is opened as tab or a new instance. */
                req = webkit_navigation_action_get_request(a);
                spawn_new_instance(webkit_uri_request_get_uri(req));
                return TRUE;
            }
            return FALSE;

        case WEBKIT_POLICY_DECISION_TYPE_RESPONSE:
            res    = webkit_response_policy_decision_get_response(WEBKIT_RESPONSE_POLICY_DECISION(dec));
            status = webkit_uri_response_get_status_code(res);

            if (!webkit_response_policy_decision_is_mime_type_supported(WEBKIT_RESPONSE_POLICY_DECISION(dec))
                    && (SOUP_STATUS_IS_SUCCESSFUL(status) || status == SOUP_STATUS_NONE)) {

                webkit_policy_decision_download(dec);

                return TRUE;
            }
            return FALSE;

        default:
            return FALSE;
    }
}

static void on_webview_load_changed(WebKitWebView *webview,
        WebKitLoadEvent event, Client *c)
{
    GTlsCertificateFlags tlsflags;
    char *uri = NULL;

    switch (event) {
        case WEBKIT_LOAD_STARTED:
            uri = util_sanitize_uri(webkit_web_view_get_uri(webview));
#ifdef FEATURE_AUTOCMD
            autocmd_run(c, AU_LOAD_STARTED, uri, NULL);
#endif
            /* update load progress in statusbar */
            c->state.progress = 0;
            vb_statusbar_update(c);
            set_title(c, uri);
            /* Make sure hinting is cleared before the new page is loaded.
             * Without that vimb would still be in hinting mode after hinting
             * was started and some links was clicked my mouse. Even if there
             * could not hints be shown. */
            if (c->mode->flags & FLAG_HINTING) {
                vb_enter(c, 'n');
            }
            break;

        case WEBKIT_LOAD_REDIRECTED:
            break;

        case WEBKIT_LOAD_COMMITTED:
            /* In case of HTTP authentication request we ignore the focus
             * changes so that the input mode can be set for the
             * authentication request. If the authentication dialog is filled
             * or aborted the load will be commited. So this seems to be the
             * right place to remove the flag. */
            c->mode->flags &= ~FLAG_IGNORE_FOCUS;
            uri = util_sanitize_uri(webkit_web_view_get_uri(webview));
#ifdef FEATURE_AUTOCMD
            autocmd_run(c, AU_LOAD_COMMITTED, uri, NULL);
#endif
            /* save the current URI in register % */
            vb_register_add(c, '%', uri);
            /* check if tls is on and the page is trusted */
            if (g_str_has_prefix(uri, "https://")) {
                if (webkit_web_view_get_tls_info(webview, NULL, &tlsflags) && tlsflags) {
                    set_statusbar_style(c, STATUS_SSL_INVALID);
                } else {
                    set_statusbar_style(c, STATUS_SSL_VALID);
                }
            } else {
                set_statusbar_style(c, STATUS_NORMAL);
            }

            /* clear possible set marks */
            marks_clear(c);

            /* Unset possible last search. Use commit==TRUE to clear inputbox
             * in case a link was fired from highlighted link. */
            command_search(c, &(Arg){0, NULL}, TRUE);

            break;

        case WEBKIT_LOAD_FINISHED:
            uri = util_sanitize_uri(webkit_web_view_get_uri(webview));
#ifdef FEATURE_AUTOCMD
            autocmd_run(c, AU_LOAD_FINISHED, uri, NULL);
#endif
            c->state.progress = 100;
            if (strncmp(uri, "about:", 6)) {
                history_add(c, HISTORY_URL, uri, webkit_web_view_get_title(webview));
            }
            break;
    }

    if (uri) {
        g_free(uri);
    }
}

/**
 * Callback for the webview mouse-target-changed signal.
 * This is used to print the uri too statusbar if the user hovers over links
 * or images.
 */
static void on_webview_mouse_target_changed(WebKitWebView *webview,
        WebKitHitTestResult *result, guint modifiers, Client *c)
{
    char *msg;
    char *uri;

    /* Save the hitTestResult to have this later available for events that
     * don't support this. */
    if (c->state.hit_test_result) {
        g_object_unref(c->state.hit_test_result);
    }
    c->state.hit_test_result = g_object_ref(result);

    if (webkit_hit_test_result_context_is_link(result)) {
        uri = util_sanitize_uri(webkit_hit_test_result_get_link_uri(result));
        msg = g_strconcat("Link: ", uri, NULL);
        gtk_label_set_text(GTK_LABEL(c->statusbar.left), msg);
        g_free(msg);
        g_free(uri);
    } else if (webkit_hit_test_result_context_is_image(result)) {
        uri = util_sanitize_uri(webkit_hit_test_result_get_image_uri(result));
        msg = g_strconcat("Image: ", uri, NULL);
        gtk_label_set_text(GTK_LABEL(c->statusbar.left), msg);
        g_free(msg);
        g_free(uri);
    } else {
        /* No link under cursor - show the current URI. */
        update_urlbar(c);
    }
}

/**
 * Called on webviews notify::estimated-load-progress event. This writes the
 * esitamted load progress in percent in a variable and updates the statusbar
 * to make the changes visible.
 */
static void on_webview_notify_estimated_load_progress(WebKitWebView *webview,
        GParamSpec *spec, Client *c)
{
    c->state.progress = webkit_web_view_get_estimated_load_progress(webview) * 100;
    vb_statusbar_update(c);
    update_title(c);
}

/**
 * Callback for the webview notify::title signal.
 * Changes the window title according to the title of the current page.
 */
static void on_webview_notify_title(WebKitWebView *webview, GParamSpec *pspec, Client *c)
{
    const char *title = webkit_web_view_get_title(webview);

    if (*title) {
        set_title(c, title);
    }
}

/**
 * Callback for the webview notify::uri signal.
 * Changes the current uri shown on left of statusbar.
 */
static void on_webview_notify_uri(WebKitWebView *webview, GParamSpec *pspec, Client *c)
{
    if (c->state.uri) {
        g_free(c->state.uri);
    }
    c->state.uri = util_sanitize_uri(webkit_web_view_get_uri(c->webview));

    update_urlbar(c);
    g_setenv("VIMB_URI", c->state.uri, TRUE);
}

/**
 * Callback for the webview ready-to-show signal.
 * Show the webview only if it's ready to be shown.
 */
static void on_webview_ready_to_show(WebKitWebView *webview, Client *c)
{
    client_show(webview, c);
}

/**
 * Callback for the webview web-process-crashed signal.
 */
static gboolean on_webview_web_process_crashed(WebKitWebView *webview, Client *c)
{
    vb_echo(c, MSG_ERROR, FALSE, "Webview Crashed on %s", webkit_web_view_get_uri(webview));

    return TRUE;
}

/**
 * Callback in case HTTP authentication is requested by the server.
 */
static gboolean on_webview_authenticate(WebKitWebView *webview,
        WebKitAuthenticationRequest *request, Client *c)
{
    /* Don't change the mode if we are in pass through mode. */
    if (c->mode->id == 'n') {
        vb_enter(c, 'i');
        /* Make sure we do not switch back to normal mode in case a previos
         * page is open and looses the focus. */
        c->mode->flags |= FLAG_IGNORE_FOCUS;
    }
    return FALSE;
}

/**
 * Callback in case JS calls element.webkitRequestFullScreen.
 */
static gboolean on_webview_enter_fullscreen(WebKitWebView *webview, Client *c)
{
    c->state.is_fullscreen = TRUE;
    gtk_widget_hide(GTK_WIDGET(c->statusbar.box));
    gtk_widget_set_visible(GTK_WIDGET(c->input), FALSE);
    return FALSE;
}

/**
 * Callback to restore the window state after entering fullscreen.
 */
static gboolean on_webview_leave_fullscreen(WebKitWebView *webview, Client *c)
{
    c->state.is_fullscreen = FALSE;
    gtk_widget_show(GTK_WIDGET(c->statusbar.box));
    gtk_widget_set_visible(GTK_WIDGET(c->input), TRUE);
    return FALSE;
}

/**
 * Callback for window ::delete-event signal which is emitted if a user
 * requests that a toplevel window is closed. The default handler for this
 * signal destroys the window. Returns TRUE to stop other handlers from being
 * invoked for the event. FALSE to propagate the event further.
 */
static gboolean on_window_delete_event(GtkWidget *window, GdkEvent *event, Client *c)
{
    /* if vb_quit fails, do not propagate event further, keep window open */
    return !vb_quit(c, FALSE);
}

/**
 * Callback for the window destroy signal.
 * Destroys the client that is associated to the window.
 */
static void on_window_destroy(GtkWidget *window, Client *c)
{
    client_destroy(c);
}

/**
 * Callback for to quit given client as idle event source.
 */
static gboolean quit(Client *c)
{
    /* Destroy the main window to tirgger the destruction of the client. */
    gtk_widget_destroy(c->window);

    /* Remove this from the list of event sources. */
    return FALSE;
}

/**
 * Read string from stdin and pass it to webkit for html interpretation.
 */
static void read_from_stdin(Client *c)
{
    GIOChannel *ch;
    gchar *buf = NULL;
    GError *err = NULL;
    gsize len = 0;

    g_assert(c);

    ch = g_io_channel_unix_new(fileno(stdin));
    g_io_channel_read_to_end(ch, &buf, &len, &err);
    g_io_channel_unref(ch);

    if (err) {
        g_warning("Error loading from stdin: %s", err->message);
        g_error_free(err);
    } else {
        webkit_web_view_load_html(c->webview, buf, NULL);
    }
    g_free(buf);
}

/**
 * Free the register contents memory.
 */
static void register_cleanup(Client *c)
{
    int i;
    for (i = 0; i < REG_SIZE; i++) {
        if (c->state.reg[i]) {
            g_free(c->state.reg[i]);
        }
    }
}

static void update_title(Client *c)
{
#ifdef FEATURE_TITLE_PROGRESS
    /* Show load status of page or the downloads. */
    if (c->state.progress != 100) {
        char *title = g_strdup_printf(
                "[%i%%] %s",
                c->state.progress,
                c->state.title ? c->state.title : "");
        gtk_window_set_title(GTK_WINDOW(c->window), title);
        g_free(title);

        return;
    }
#endif
    if (c->state.title) {
        gtk_window_set_title(GTK_WINDOW(c->window), c->state.title);
    }
}

/**
 * Update the contents of the url bar on the left of the statu bar according
 * to current opened url and position in back forward history.
 */
static void update_urlbar(Client *c)
{
    GString *str;
    gboolean back, fwd;

    str = g_string_new("");
    /* show profile name */
    if (vb.profile) {
        g_string_append_printf(str, "[%s] ", vb.profile);
    }

    g_string_append_printf(str, "%s", c->state.uri);

    /* show history indicator only if there is something to show */
    back = webkit_web_view_can_go_back(c->webview);
    fwd  = webkit_web_view_can_go_forward(c->webview);
    if (back || fwd) {
        g_string_append_printf(str, " [%s]", back ? (fwd ? "-+" : "-") : "+");
    }

    gtk_label_set_text(GTK_LABEL(c->statusbar.left), str->str);
    g_string_free(str, TRUE);
}

#ifdef FREE_ON_QUIT
/**
 * Free memory of the whole application.
 */
static void vimb_cleanup(void)
{
    int i;

    while (vb.clients) {
        client_destroy(vb.clients);
    }

    /* free memory of other components */
    util_cleanup();

    for (i = 0; i < FILES_LAST; i++) {
        if (vb.files[i]) {
            g_free(vb.files[i]);
        }
    }
    g_free(vb.profile);
}
#endif

/**
 * Setup resources used on application scope.
 */
static void vimb_setup(void)
{
    WebKitWebContext *ctx;
    WebKitCookieManager *cm;
    char *path;

    /* prepare the file pathes */
    path = util_get_config_dir();

    if (vb.configfile) {
        char *rp = realpath(vb.configfile, NULL);
        vb.files[FILES_CONFIG] = g_strdup(rp);
        free(rp);
    } else {
        vb.files[FILES_CONFIG] = util_get_filepath(path, "config", FALSE, 0600);
    }

    /* Setup those files that are use multiple time during runtime */
    vb.files[FILES_CLOSED]     = util_get_filepath(path, "closed", TRUE, 0600);
    vb.files[FILES_COOKIE]     = util_get_filepath(path, "cookies.db", TRUE, 0600);
    vb.files[FILES_USER_STYLE] = util_get_filepath(path, "style.css", FALSE, 0600);
    vb.files[FILES_SCRIPT]     = util_get_filepath(path, "scripts.js", FALSE, 0600);
    vb.files[FILES_HISTORY]    = util_get_filepath(path, "history", TRUE, 0600);
    vb.files[FILES_COMMAND]    = util_get_filepath(path, "command", TRUE, 0600);
    vb.files[FILES_BOOKMARK]   = util_get_filepath(path, "bookmark", TRUE, 0600);
    vb.files[FILES_QUEUE]      = util_get_filepath(path, "queue", TRUE, 0600);
    vb.files[FILES_SEARCH]     = util_get_filepath(path, "search", TRUE, 0600);
    g_free(path);

    /* Use seperate rendering processed for the webview of the clients in the
     * current instance. This must be called as soon as possible according to
     * the documentation. */
    ctx = webkit_web_context_get_default();
    webkit_web_context_set_process_model(ctx, WEBKIT_PROCESS_MODEL_MULTIPLE_SECONDARY_PROCESSES);
    webkit_web_context_set_cache_model(ctx, WEBKIT_CACHE_MODEL_WEB_BROWSER);

    g_signal_connect(ctx, "initialize-web-extensions", G_CALLBACK(on_webctx_init_web_extension), NULL);

    /* Add cookie support only if the cookie file exists. */
    if (vb.files[FILES_COOKIE]) {
        cm = webkit_web_context_get_cookie_manager(ctx);
        webkit_cookie_manager_set_persistent_storage(
                cm,
                vb.files[FILES_COOKIE],
                WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE);
    }

    /* initialize the modes */
    vb_mode_add('n', normal_enter, normal_leave, normal_keypress, NULL);
    vb_mode_add('c', ex_enter, ex_leave, ex_keypress, ex_input_changed);
    vb_mode_add('i', input_enter, input_leave, input_keypress, NULL);
    vb_mode_add('p', pass_enter, pass_leave, pass_keypress, NULL);

    /* Prepare the style provider to be used for the clients and completion. */
    vb.style_provider = gtk_css_provider_new();
}

/**
 * Update the gui style settings for client c, given a style setting name and a
 * style setting value to be updated. The complete style sheet document will be
 * regenerated and re-fed into gtk css provider.
 */
void vb_gui_style_update(Client *c, const char *setting_name_new, const char *setting_value_new)
{
    g_assert(c);
    g_assert(setting_name_new);
    g_assert(setting_value_new);

    /* The css style sheet document being composed in this function */
    GString *style_sheet = g_string_new(GUI_STYLE_CSS_BASE);
    size_t i;

    /* Mapping from vimb config setting name to css style sheet string */
    static const char *setting_style_map[][2] = {
        {"completion-css",              " #completion{%s}"},
        {"completion-hover-css",        " #completion:hover{%s}"},
        {"completion-selected-css",     " #completion:selected{%s}"},
        {"input-css",                   " #input{%s}"},
        {"input-error-css",             " #input.error{%s}"},
        {"status-css",                  " #statusbar{%s}"},
        {"status-ssl-css",              " #statusbar.secure{%s}"},
        {"status-ssl-invalid-css",      " #statusbar.unsecure{%s}"},
        {0, 0},
    };

    /* For each supported style setting name */
    for (i = 0; setting_style_map[i][0]; i++) {
        const char *setting_name = setting_style_map[i][0];
        const char *style_string = setting_style_map[i][1];

        /* If the current style setting name is the one to be updated,
         * append the given value with appropriate css wrapping to the
         * style sheet document. */
        if (strcmp(setting_name, setting_name_new) == 0) {
            if (strlen(setting_value_new)) {
                g_string_append_printf(style_sheet, style_string, setting_value_new);
            }
        }
        /* If the current style setting name is NOT the one being updated,
         * append the css string based on the current config setting. */
        else {
            Setting* setting_value = (Setting*)g_hash_table_lookup(c->config.settings, setting_name);

            /* If the current style setting name is not available via settings
             * yet - this happens during setting_init() - cleanup and return.
             * We are going to be called again. With the last setting_add(),
             * all style setting names are available. */
            if(!setting_value) {
                goto cleanup;
            }

            if (strlen(setting_value->value.s)) {
                g_string_append_printf(style_sheet, style_string, setting_value->value.s);
            }
        }
    }

    /* Feed style sheet document to gtk */
    gtk_css_provider_load_from_data(vb.style_provider, style_sheet->str, -1, NULL);

    /* WORKAROUND to always ensure correct size of input field
     *
     * The following line is required to apply the style defined font size on
     * the GtkTextView c->input. Without the call, the font size is updated on
     * first user input, leading to a sudden unpleasant widget size and layout
     * change. According to the GTK+ docs, this call should not be required as
     * style context invalidation is automatic.
     *
     * "gtk_style_context_invalidate has been deprecated since version 3.12
     * and should not be used in newly-written code. Style contexts are
     * invalidated automatically."
     * https://developer.gnome.org/gtk3/stable/GtkStyleContext.html#gtk-style-context-invalidate
     *
     * Required settings in vimb config file:
     * set input-autohide=true
     * set input-font-normal=20pt monospace
     *
     * A bug has been filed at GTK+
     * https://bugzilla.gnome.org/show_bug.cgi?id=781158
     *
     * Tested on ARCH linux with gtk3 3.22.10-1
     */
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS;
    gtk_style_context_invalidate(gtk_widget_get_style_context(c->input));
    G_GNUC_END_IGNORE_DEPRECATIONS;

cleanup:
    g_string_free(style_sheet, TRUE);
}

/**
 * Factory to create a new webview.
 *
 * @webview:    Relates webview or NULL. If given a related webview is
 *              generated.
 */
static WebKitWebView *webview_new(Client *c, WebKitWebView *webview)
{
    WebKitWebView *new;
    WebKitUserContentManager *ucm;
    WebKitWebContext *webcontext;

    /* create a new webview */
    ucm = webkit_user_content_manager_new();
    if (webview) {
        new = WEBKIT_WEB_VIEW(webkit_web_view_new_with_related_view(webview));
    } else {
        new = WEBKIT_WEB_VIEW(webkit_web_view_new_with_user_content_manager(ucm));
    }

    g_object_connect(
        G_OBJECT(new),
        "signal::close", G_CALLBACK(on_webview_close), c,
        "signal::create", G_CALLBACK(on_webview_create), c,
        "signal::decide-policy", G_CALLBACK(on_webview_decide_policy), c,
        "signal::load-changed", G_CALLBACK(on_webview_load_changed), c,
        "signal::mouse-target-changed", G_CALLBACK(on_webview_mouse_target_changed), c,
        "signal::notify::estimated-load-progress", G_CALLBACK(on_webview_notify_estimated_load_progress), c,
        "signal::notify::title", G_CALLBACK(on_webview_notify_title), c,
        "signal::notify::uri", G_CALLBACK(on_webview_notify_uri), c,
        "signal::permission-request", G_CALLBACK(on_permission_request), c,
        "signal::ready-to-show", G_CALLBACK(on_webview_ready_to_show), c,
        "signal::web-process-crashed", G_CALLBACK(on_webview_web_process_crashed), c,
        "signal::authenticate", G_CALLBACK(on_webview_authenticate), c,
        "signal::enter-fullscreen", G_CALLBACK(on_webview_enter_fullscreen), c,
        "signal::leave-fullscreen", G_CALLBACK(on_webview_leave_fullscreen), c,
        NULL
    );

    webcontext = webkit_web_view_get_context(new);
    g_signal_connect(webcontext, "download-started", G_CALLBACK(on_webctx_download_started), c);

    /* Setup script message handlers. */
    webkit_user_content_manager_register_script_message_handler(ucm, "focus");
    g_signal_connect(ucm, "script-message-received::focus", G_CALLBACK(on_script_message_focus), NULL);

    return new;
}

static void on_counted_matches(WebKitFindController *finder, guint count, Client *c)
{
    c->state.search.matches = count;
    vb_statusbar_update(c);
}

static gboolean on_permission_request(WebKitWebView *webview,
        WebKitPermissionRequest *request, Client *c)
{
    GtkWidget *dialog;
    int result;
    char *msg = NULL;

    if (WEBKIT_IS_GEOLOCATION_PERMISSION_REQUEST(request)) {
        msg = "request your location";
    } else if (WEBKIT_IS_USER_MEDIA_PERMISSION_REQUEST(request)) {
        if (webkit_user_media_permission_is_for_audio_device(WEBKIT_USER_MEDIA_PERMISSION_REQUEST(request))) {
            msg = "access the microphone";
        } else if (webkit_user_media_permission_is_for_video_device(WEBKIT_USER_MEDIA_PERMISSION_REQUEST(request))) {
            msg = "access you webcam";
        }
    } else {
        return FALSE;
    }

    dialog = gtk_message_dialog_new(GTK_WINDOW(c->window), GTK_DIALOG_MODAL,
            GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "Page wants to %s",
            msg);

    gtk_widget_show(dialog);
    result = gtk_dialog_run(GTK_DIALOG(dialog));
    if (GTK_RESPONSE_YES == result) {
        webkit_permission_request_allow(request);
    } else {
        webkit_permission_request_deny(request);
    }
    gtk_widget_destroy(dialog);

    return TRUE;
}

static void on_script_message_focus(WebKitUserContentManager *manager,
        WebKitJavascriptResult *res, gpointer data)
{
    char *message;
    GVariant *variant;
    guint64 pageid;
    gboolean is_focused;
    Client *c;

    message = util_js_result_as_string(res);
    variant = g_variant_parse(G_VARIANT_TYPE("(tb)"), message, NULL, NULL, NULL);
    g_free(message);

    g_variant_get(variant, "(tb)", &pageid, &is_focused);
    g_variant_unref(variant);

    c = vb_get_client_for_page_id(pageid);
    if (!c || c->mode->flags & FLAG_IGNORE_FOCUS) {
        return;
    }

    /* Don't change the mode if we are in pass through mode. */
    if (c->mode->id == 'n' && is_focused) {
        vb_enter(c, 'i');
    } else if (c->mode->id == 'i' && !is_focused) {
        vb_enter(c, 'n');
    }
}

static gboolean profileOptionArgFunc(const gchar *option_name,
        const gchar *value, gpointer data, GError **error)
{
    vb.profile = util_sanitize_filename(g_strdup(value));

    return TRUE;
}

int main(int argc, char* argv[])
{
    Client *c;
    GError *err = NULL;
    char *pidstr, *winid = NULL;
    gboolean ver = FALSE, buginfo = FALSE;

    GOptionEntry opts[] = {
        {"embed", 'e', 0, G_OPTION_ARG_STRING, &winid, "Reparents to window specified by xid", NULL},
        {"config", 'c', 0, G_OPTION_ARG_FILENAME, &vb.configfile, "Custom configuration file", NULL},
        {"profile", 'p', 0, G_OPTION_ARG_CALLBACK, (GOptionArgFunc*)profileOptionArgFunc, "Profile name", NULL},
        {"version", 'v', 0, G_OPTION_ARG_NONE, &ver, "Print version", NULL},
        {"bug-info", 0, 0, G_OPTION_ARG_NONE, &buginfo, "Print used library versions", NULL},
        {NULL}
    };

    /* initialize GTK+ */
    if (!gtk_init_with_args(&argc, &argv, "[URI]", opts, NULL, &err)) {
        fprintf(stderr, "can't init gtk: %s\n", err->message);
        g_error_free(err);

        return EXIT_FAILURE;
    }

    if (ver) {
        printf("%s, version %s\n", PROJECT, VERSION);
        return EXIT_SUCCESS;
    }

    if (buginfo) {
        printf("Version:         %s\n", VERSION);
        printf("WebKit compile:  %d.%d.%d\n",
                WEBKIT_MAJOR_VERSION,
                WEBKIT_MINOR_VERSION,
                WEBKIT_MICRO_VERSION);
        printf("WebKit run:      %d.%d.%d\n",
                webkit_get_major_version(),
                webkit_get_minor_version(),
                webkit_get_micro_version());
        printf("GTK compile:     %d.%d.%d\n",
                GTK_MAJOR_VERSION,
                GTK_MINOR_VERSION,
                GTK_MICRO_VERSION);
        printf("GTK run:         %d.%d.%d\n",
                gtk_major_version,
                gtk_minor_version,
                gtk_micro_version);
        printf("libsoup compile: %d.%d.%d\n",
                SOUP_MAJOR_VERSION,
                SOUP_MINOR_VERSION,
                SOUP_MICRO_VERSION);
        printf("libsoup run:     %u.%u.%u\n",
                soup_get_major_version(),
                soup_get_minor_version(),
                soup_get_micro_version());
        printf("Extension dir:   %s\n",
                EXTENSIONDIR);

        return EXIT_SUCCESS;
    }

    /* Save the base name for spawning new instances. */
    vb.argv0 = argv[0];

    /* set the current pid in env */
    pidstr = g_strdup_printf("%d", (int)getpid());
    g_setenv("VIMB_PID", pidstr, TRUE);
    g_free(pidstr);

    vimb_setup();

    if (winid) {
        vb.embed = strtol(winid, NULL, 0);
    }

    c = client_new(NULL);
    client_show(NULL, c);
    if (argc <= 1) {
        vb_load_uri(c, &(Arg){TARGET_CURRENT, NULL});
    } else if (!strcmp(argv[argc - 1], "-")) {
        /* read from stdin if uri is - */
        read_from_stdin(c);
    } else {
        vb_load_uri(c, &(Arg){TARGET_CURRENT, argv[argc - 1]});
    }

    gtk_main();
#ifdef FREE_ON_QUIT
    vimb_cleanup();
#endif

    return EXIT_SUCCESS;
}
