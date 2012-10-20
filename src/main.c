#include "config.h"
#include "main.h"
#include "util.h"
#include "command.h"
#include "keybind.h"

/* variables */
VpCore vp;

/* callbacks */
static void vp_webview_load_status_cb(WebKitWebView* view, GParamSpec* pspec, gpointer user_data);
static void vp_webview_load_commited_cb(WebKitWebView *webview, WebKitWebFrame* frame, gpointer user_data);
static void vp_destroy_window_cb(GtkWidget* widget, GtkWidget* window, gpointer user_data);
static gboolean vp_frame_scrollbar_policy_changed_cb(void);
static void vp_inputbox_activate_cb(GtkEntry* entry, gpointer user_data);
static gboolean vp_inputbox_keypress_cb(GtkEntry* entry, GdkEventKey* event);
static gboolean vp_inputbox_keyrelease_cb(GtkEntry* entry, GdkEventKey* event);

/* functions */
static gboolean vp_process_input(const char* input);
static void vp_print_version(void);
static void vp_init(void);
static void vp_read_config(void);
static void vp_init_gui(void);
static void vp_init_files(void);
static void vp_set_widget_font(GtkWidget* widget, const gchar* font_definition, const gchar* bg_color, const gchar* fg_color);
static void vp_setup_settings(void);
static void vp_setup_signals(void);
static gboolean vp_load_uri(const Arg* arg);

static void vp_webview_load_status_cb(WebKitWebView* view, GParamSpec* pspec, gpointer user_data)
{
    Gui* gui        = &vp.gui;
    const char* uri = webkit_web_view_get_uri(gui->webview);

    switch (webkit_web_view_get_load_status(gui->webview)) {
        case WEBKIT_LOAD_COMMITTED:
            vp_update_urlbar(uri);
            break;

        case WEBKIT_LOAD_FINISHED:
            break;

        default:
            break;
    }
}

static void vp_webview_load_commited_cb(WebKitWebView *webview, WebKitWebFrame *frame, gpointer user_data)
{
    /* unset possible set modkey and counts if loading a new page */
    vp.state.modkey = vp.state.count = 0;
    vp_update_statusbar();
}

static void vp_destroy_window_cb(GtkWidget* widget, GtkWidget* window, gpointer user_data)
{
    vp_close_browser(0);
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
        success = vp_process_input((text + 1));
        if (!success) {
            /* print error message */
            gchar* message = g_strdup_printf("Command '%s' not found", (text + 1));
            vp_echo(VP_MSG_ERROR, message);
            g_free(message);

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

static gboolean vp_inputbox_keypress_cb(GtkEntry *entry, GdkEventKey *event)
{
    return FALSE;
}

static gboolean vp_inputbox_keyrelease_cb(GtkEntry *entry, GdkEventKey *event)
{
    return FALSE;
}

static gboolean vp_process_input(const char* input)
{
    gboolean success;
    gchar* line = g_strdup(input);
    gchar* command = NULL;
    gchar** token;

    if (!input || !strlen(input)) {
        return FALSE;
    }

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

static gboolean vp_load_uri(const Arg* arg)
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

void vp_navigate(const Arg* arg)
{
    if (arg->i <= VP_NAVIG_FORWARD) {
        /* TODO allow to set a count for the navigation */
        webkit_web_view_go_back_or_forward(
            vp.gui.webview, (arg->i == VP_NAVIG_BACK ? -1 : 1)
        );
    } else if (arg->i == VP_NAVIG_RELOAD) {
        webkit_web_view_reload(vp.gui.webview);
    } else if (arg->i == VP_NAVIG_RELOAD_FORCE) {
        webkit_web_view_reload_bypass_cache(vp.gui.webview);
    } else {
        webkit_web_view_stop_loading(vp.gui.webview);
    }
}

void vp_scroll(const Arg* arg)
{
    GtkAdjustment *adjust = (arg->i & VP_SCROLL_AXIS_H) ? vp.gui.adjust_h : vp.gui.adjust_v;

    gint direction  = (arg->i & (1 << 2)) ? 1 : -1;

    /* type scroll */
    if (arg->i & VP_SCROLL_TYPE_SCROLL) {
        gdouble value;
        gint count = vp.state.count ? vp.state.count : 1;
        if (arg->i & VP_SCROLL_UNIT_LINE) {
            value = SCROLLSTEP;
        } else if (arg->i & VP_SCROLL_UNIT_HALFPAGE) {
            value = gtk_adjustment_get_page_size(adjust) / 2;
        } else {
            value = gtk_adjustment_get_page_size(adjust);
        }
        gtk_adjustment_set_value(adjust, gtk_adjustment_get_value(adjust) + direction * value * count);
    } else if (vp.state.count) {
        /* jump - if count is set to count% of page */
        gdouble max = gtk_adjustment_get_upper(adjust) - gtk_adjustment_get_page_size(adjust);
        gtk_adjustment_set_value(adjust, max * vp.state.count / 100);
    } else if (direction == 1) {
        /* jump to top */
        gtk_adjustment_set_value(adjust, gtk_adjustment_get_upper(adjust));
    } else {
        /* jump to bottom */
        gtk_adjustment_set_value(adjust, gtk_adjustment_get_lower(adjust));
    }
}

void vp_close_browser(const Arg* arg)
{
    vp_clean_up();
    gtk_main_quit();
}

void vp_clean_up(void)
{
    if (vp.behave.commands) {
        g_hash_table_destroy(vp.behave.commands);
        vp.behave.commands = NULL;
    }
    for (int i = FILES_FIRST; i < FILES_LAST; i++) {
        g_free(vp.files[i]);
    }
}

void vp_view_source(const Arg* arg)
{
    gboolean mode = webkit_web_view_get_view_source_mode(vp.gui.webview);
    webkit_web_view_set_view_source_mode(vp.gui.webview, !mode);
    webkit_web_view_reload(vp.gui.webview);
}

void vp_set_mode(const Arg* arg)
{
    vp.state.mode = arg->i;
    vp.state.modkey = vp.state.count  = 0;

    switch (arg->i) {
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
}

void vp_input(const Arg* arg)
{
    gint pos = 0;
    const gchar* url;

    /* reset the colors and fonts to defalts */
    vp_set_widget_font(vp.gui.inputbox, inputbox_font[0], inputbox_bg[0], inputbox_fg[0]);

    /* remove content from input box */
    gtk_entry_set_text(GTK_ENTRY(vp.gui.inputbox), "");

    /* insert string from arg */
    gtk_editable_insert_text(GTK_EDITABLE(vp.gui.inputbox), arg->s, -1, &pos);

    /* add current url if requested */
    if (VP_INPUT_CURRENT_URI == arg->i
            && (url = webkit_web_view_get_uri(vp.gui.webview))) {
        gtk_editable_insert_text(GTK_EDITABLE(vp.gui.inputbox), url, -1, &pos);
    }

    gtk_editable_set_position(GTK_EDITABLE(vp.gui.inputbox), -1);

    Arg a = {VP_MODE_COMMAND};
    vp_set_mode(&a);
}

void vp_open(const Arg* arg)
{
    vp_load_uri(arg);
}

void vp_update_urlbar(const gchar* uri)
{
    gchar* markup;

    markup = g_markup_printf_escaped("<span font=\"%s\">%s</span>", STATUS_BAR_FONT, uri);
    gtk_label_set_markup(GTK_LABEL(vp.gui.statusbar.left), markup);
    g_free(markup);
}

void vp_update_statusbar(void)
{
    GString* status = g_string_new("");
    gchar*   markup;

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

    markup = g_markup_printf_escaped("<span font=\"%s\">%s</span>", STATUS_BAR_FONT, status->str);
    gtk_label_set_markup(GTK_LABEL(vp.gui.statusbar.right), markup);
    g_free(markup);
}

void vp_echo(const MessageType type, const gchar *message)
{
    /* don't print message if the input is focussed */
    if (gtk_widget_is_focus(GTK_WIDGET(vp.gui.inputbox))) {
        return;
    }

    /* set the collors according to message type */
    vp_set_widget_font(vp.gui.inputbox, inputbox_font[type], inputbox_bg[type], inputbox_fg[type]);
    gtk_entry_set_text(GTK_ENTRY(vp.gui.inputbox), message);
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

    vp_read_config();
}

static void vp_read_config(void)
{
    FILE* fp;
    gchar line[255];
    gchar** string = NULL;

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
        g_strstrip(line);

        /* split into command and params */
        string = g_strsplit(line, " ", 2);
        if (g_strv_length(string) != 2) {
            g_strfreev(string);
            continue;
        }

        /* delegate the different commands */
        if (g_ascii_strcasecmp(string[0], "nmap") == 0) {
            keybind_add_from_string(string[1], VP_MODE_NORMAL);
        } else if (g_ascii_strcasecmp(string[0], "unmap") == 0) {
            keybind_remove_from_string(string[1], VP_MODE_NORMAL);
        }

        g_strfreev(string);
    }

    fclose(fp);
}

static void vp_init_gui(void)
{
    Gui* gui = &vp.gui;
    GdkColor bg, fg;

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

    /* Create a browser instance */
    gui->webview = WEBKIT_WEB_VIEW(webkit_web_view_new());

    vp_setup_settings();

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

    vp_set_widget_font(gui->inputbox, inputbox_font[0], inputbox_bg[0], inputbox_fg[0]);

    /* Prepare the statusbar */
    gui->statusbar.box   = GTK_BOX(gtk_hbox_new(FALSE, 0));
    gui->statusbar.left  = gtk_label_new(NULL);
    gui->statusbar.right = gtk_label_new(NULL);

    /* Prepare the event box */
    gui->eventbox = gtk_event_box_new();
    gdk_color_parse(STATUS_BG_COLOR, &bg);
    gdk_color_parse(STATUS_FG_COLOR, &fg);
    gtk_widget_modify_bg(gui->eventbox, GTK_STATE_NORMAL, &bg);
    gtk_widget_modify_fg(gui->eventbox, GTK_STATE_NORMAL, &fg);

    vp_setup_signals();

    /* Put all part together */
    gtk_container_add(GTK_CONTAINER(gui->viewport), GTK_WIDGET(gui->webview));
    gtk_container_add(GTK_CONTAINER(gui->eventbox), GTK_WIDGET(gui->statusbar.box));
    gtk_container_add(GTK_CONTAINER(gui->window), GTK_WIDGET(gui->box));
    gtk_misc_set_alignment(GTK_MISC(gui->statusbar.left), 0.0, 0.0);
    gtk_misc_set_alignment(GTK_MISC(gui->statusbar.right), 1.0, 0.0);
    gtk_box_pack_start(gui->statusbar.box, gui->statusbar.left, TRUE, TRUE, 2);
    gtk_box_pack_start(gui->statusbar.box, gui->statusbar.right, FALSE, FALSE, 2);
    gtk_widget_modify_bg(GTK_WIDGET(gui->statusbar.left), GTK_STATE_NORMAL, &bg);
    gtk_widget_modify_fg(GTK_WIDGET(gui->statusbar.left), GTK_STATE_NORMAL, &fg);
    gtk_widget_modify_bg(GTK_WIDGET(gui->statusbar.right), GTK_STATE_NORMAL, &bg);
    gtk_widget_modify_fg(GTK_WIDGET(gui->statusbar.right), GTK_STATE_NORMAL, &fg);

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

    g_free(path);
}

static void vp_set_widget_font(GtkWidget* widget, const gchar* font_definition, const gchar* bg_color, const gchar* fg_color)
{
    GdkColor fg, bg;
    PangoFontDescription *font;

    font = pango_font_description_from_string(font_definition);
    gtk_widget_modify_font(widget, font);
    pango_font_description_free(font);

    if (fg_color) {
        gdk_color_parse(fg_color, &fg);
    }
    if (bg_color) {
        gdk_color_parse(bg_color, &bg);
    }

    gtk_widget_modify_text(widget, GTK_STATE_NORMAL, fg_color ? &fg : NULL);
    gtk_widget_modify_base(widget, GTK_STATE_NORMAL, bg_color ? &bg : NULL);
}

static void vp_setup_settings(void)
{
    WebKitWebSettings *settings = webkit_web_view_get_settings(vp.gui.webview);

    g_object_set(G_OBJECT(settings), "user-agent", SETTING_USER_AGENT, NULL);
    webkit_web_view_set_settings(vp.gui.webview, settings);
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
    g_signal_connect(G_OBJECT(gui->webview), "load-committed", G_CALLBACK(vp_webview_load_commited_cb), NULL);

    g_object_connect(
        G_OBJECT(gui->inputbox),
        "signal::activate",          G_CALLBACK(vp_inputbox_activate_cb),   NULL,
        "signal::key-press-event",   G_CALLBACK(vp_inputbox_keypress_cb),   NULL,
        "signal::key-release-event", G_CALLBACK(vp_inputbox_keyrelease_cb), NULL,
        NULL
    );

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
