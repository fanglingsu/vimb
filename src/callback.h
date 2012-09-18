#ifndef CALLBACKS_H
#define CALLBACKS_H

/* callbacks */
void webview_load_status_cb(WebKitWebView* view, GParamSpec* pspec);
void destroy_window_cb(GtkWidget* widget, GtkWidget* window);
gboolean dummy_cb(void);

#endif /* end of include guard: CALLBACKS_H */
