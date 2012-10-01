#include "config.h"
#include "main.h"
#include "command.h"
#include "keybind.h"

/* variables */
VpCore vp;

/* callbacks */
void vp_webview_load_status_cb(WebKitWebView* view, GParamSpec* pspec);
void vp_destroy_window_cb(GtkWidget* widget, GtkWidget* window);
gboolean vp_frame_scrollbar_policy_changed_cb(void);

/* functions */
static void vp_print_version(void);
static void vp_init(void);
static void vp_init_gui(void);
static void vp_setup_signals(void);


void vp_webview_load_status_cb(WebKitWebView* view, GParamSpec* pspec)
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

void vp_destroy_window_cb(GtkWidget* widget, GtkWidget* window)
{
    vp_close_browser(0);
}

gboolean vp_frame_scrollbar_policy_changed_cb(void)
{
    return TRUE;
}

gboolean vp_load_uri(const Arg* arg)
{
    char* u;
    const char* uri = arg->c;

    if (strcmp(uri, "") == 0) {
        return FALSE;
    }
    u = g_strrstr(uri, "://") ? g_strdup(uri) : g_strdup_printf("http://%s", uri);

    /* Load a web page into the browser instance */
    webkit_web_view_load_uri(vp.gui.webview, u);
    g_free(u);

    /* change state to normal mode */
    vp.state.mode = VP_MODE_NORMAL;
    vp_update_statusbar();

    return TRUE;
}

gboolean vp_navigate(const Arg *arg)
{
    if (arg->i <= NAVIG_FORWARD) {
        /* TODO allow to set a count for the navigation */
        webkit_web_view_go_back_or_forward(
            vp.gui.webview, (arg->i == NAVIG_BACK ? -1 : 1)
        );
    } else if (arg->i == NAVIG_RELOAD) {
        webkit_web_view_reload(vp.gui.webview);
    } else if (arg->i == NAVIG_RELOAD_FORCE) {
        webkit_web_view_reload_bypass_cache(vp.gui.webview);
    } else {
        webkit_web_view_stop_loading(vp.gui.webview);
    }

    return TRUE;
}

void vp_close_browser()
{
    if (vp.behave.commands) {
        g_hash_table_destroy(vp.behave.commands);
        vp.behave.commands = NULL;
    }
    gtk_main_quit();
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

    /* TODO show modekeys count */
    /* at the moment we have only one modkey available */
    /* http://current_uri.tld   34% */
    g_string_append_printf(status, "%.0d", vp.state.count);
    if (vp.state.modkey) {
        g_string_append_c(status, vp.state.modkey);
    }
    g_string_append_printf(status, " %.0d%%", 46);
    markup = g_markup_printf_escaped("<span font=\"%s\">%s</span>", STATUS_BAR_FONT, status->str);
    gtk_label_set_markup(GTK_LABEL(vp.gui.statusbar.right), markup);
    g_free(markup);
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
    /* initialize the keybindings */
    keybind_init();

    /*command_parse_line("quit", NULL);*/
    keybind_add(VP_MODE_NORMAL, GDK_g, 0, GDK_s, "source");
    keybind_add(VP_MODE_NORMAL, 0,     0, GDK_d, "quit");
}

static void vp_init_gui(void)
{
    Gui* gui = &vp.gui;
    GdkColor bg, fg;

    GdkGeometry hints = {10, 10};
    gui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_wmclass(GTK_WINDOW(gui->window), PROJECT, PROJECT);
    gtk_window_set_default_size(GTK_WINDOW(gui->window), 640, 480);
    gtk_window_set_title(GTK_WINDOW(gui->window), PROJECT);
    gtk_window_set_geometry_hints(GTK_WINDOW(gui->window), NULL, &hints, GDK_HINT_MIN_SIZE);

    /* Create a browser instance */
    gui->webview = WEBKIT_WEB_VIEW(webkit_web_view_new());

    /* Create a scrollable area */
    gui->viewport = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(gui->viewport),
        GTK_POLICY_NEVER, GTK_POLICY_NEVER
    );

    gui->box = GTK_BOX(gtk_vbox_new(FALSE, 0));

    /* Prepare the imputbox */
    gui->inputbox = gtk_entry_new();
    gtk_entry_set_inner_border(GTK_ENTRY(gui->inputbox), NULL);
    g_object_set(gtk_widget_get_settings(gui->inputbox), "gtk-entry-select-on-focus", FALSE, NULL);

    PangoFontDescription* font = pango_font_description_from_string(URL_BOX_FONT);
    gtk_widget_modify_font(GTK_WIDGET(gui->inputbox), font);
    pango_font_description_free(font);

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

static void vp_setup_signals(void)
{
    Gui* gui              = &vp.gui;
    WebKitWebFrame *frame = webkit_web_view_get_main_frame(gui->webview);

    /* Set up callbacks so that if either the main window or the browser
     * instance is closed, the program will exit */
    g_signal_connect(gui->window, "destroy", G_CALLBACK(vp_destroy_window_cb), NULL);
    g_signal_connect(G_OBJECT(frame), "scrollbars-policy-changed", G_CALLBACK(vp_frame_scrollbar_policy_changed_cb), NULL);
    g_signal_connect(G_OBJECT(gui->webview), "notify::load-status", G_CALLBACK(vp_webview_load_status_cb), NULL);
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
        arg.c = g_strdup(argv[argc - 1]);
    } else {
        arg.c = g_strdup(START_PAGE);
    }
    vp_load_uri(&arg);
    g_free(arg.c);

    /* Run the main GTK+ event loop */
    gtk_main();

    return EXIT_SUCCESS;
}
