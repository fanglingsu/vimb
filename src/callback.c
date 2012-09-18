#include "main.h"
#include "callback.h"
#include "config.h"

void webview_load_status_cb(WebKitWebView* view, GParamSpec* pspec)
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

void destroy_window_cb(GtkWidget* widget, GtkWidget* window)
{
    vp_close_browser(0);
}

gboolean dummy_cb(void)
{
    return TRUE;
}
