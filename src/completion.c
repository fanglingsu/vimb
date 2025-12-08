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

#include "completion.h"
#include "config.h"
#include "main.h"

/* CompletionItem GObject implementation */
struct _CompletionItem {
    GObject parent_instance;
    char *first;
    char *second;
};

G_DEFINE_TYPE(CompletionItem, completion_item, G_TYPE_OBJECT)

enum {
    PROP_0,
    PROP_FIRST,
    PROP_SECOND,
    N_PROPS
};

static GParamSpec *item_props[N_PROPS] = { NULL, };

static void completion_item_finalize(GObject *object)
{
    CompletionItem *self = COMPLETION_ITEM(object);
    g_free(self->first);
    g_free(self->second);
    G_OBJECT_CLASS(completion_item_parent_class)->finalize(object);
}

static void completion_item_get_property(GObject *object, guint prop_id,
        GValue *value, GParamSpec *pspec)
{
    CompletionItem *self = COMPLETION_ITEM(object);
    switch (prop_id) {
        case PROP_FIRST:
            g_value_set_string(value, self->first);
            break;
        case PROP_SECOND:
            g_value_set_string(value, self->second);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void completion_item_set_property(GObject *object, guint prop_id,
        const GValue *value, GParamSpec *pspec)
{
    CompletionItem *self = COMPLETION_ITEM(object);
    switch (prop_id) {
        case PROP_FIRST:
            g_free(self->first);
            self->first = g_value_dup_string(value);
            break;
        case PROP_SECOND:
            g_free(self->second);
            self->second = g_value_dup_string(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void completion_item_class_init(CompletionItemClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    object_class->finalize = completion_item_finalize;
    object_class->get_property = completion_item_get_property;
    object_class->set_property = completion_item_set_property;

    item_props[PROP_FIRST] = g_param_spec_string(
        "first", "First", "First column value",
        NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
    item_props[PROP_SECOND] = g_param_spec_string(
        "second", "Second", "Second column value",
        NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

    g_object_class_install_properties(object_class, N_PROPS, item_props);
}

static void completion_item_init(CompletionItem *self)
{
    self->first = NULL;
    self->second = NULL;
}

CompletionItem *completion_item_new(const char *first, const char *second)
{
    return g_object_new(COMPLETION_TYPE_ITEM,
        "first", first,
        "second", second,
        NULL);
}

const char *completion_item_get_first(CompletionItem *item)
{
    g_return_val_if_fail(COMPLETION_IS_ITEM(item), NULL);
    return item->first;
}

const char *completion_item_get_second(CompletionItem *item)
{
    g_return_val_if_fail(COMPLETION_IS_ITEM(item), NULL);
    return item->second;
}

GListStore *completion_store_new(void)
{
    return g_list_store_new(COMPLETION_TYPE_ITEM);
}

/* Completion widget state */
typedef struct {
    GtkWidget               *win, *listview;
    GtkSingleSelection      *selection;
    int                     active;  /* number of the current active item */
    CompletionSelectFunc    selfunc;
} Completion;

static void on_selection_changed(GtkSelectionModel *model, guint position,
        guint n_items, gpointer data);
static void setup_listitem(GtkListItemFactory *factory, GtkListItem *list_item,
        gpointer user_data);
static void bind_listitem(GtkListItemFactory *factory, GtkListItem *list_item,
        gpointer user_data);

extern struct Vimb vb;


/**
 * Stop the completion and reset temporary used data.
 */
void completion_clean(Client *c)
{
    Completion *comp = (Completion*)c->comp;
    c->mode->flags  &= ~FLAG_COMPLETION;

    if (comp->win) {
        /* Disconnect signal before destroying to avoid spurious callbacks */
        if (comp->selection) {
            g_signal_handlers_disconnect_by_func(comp->selection,
                G_CALLBACK(on_selection_changed), c);
        }
        gtk_widget_unparent(comp->win);
        comp->win       = NULL;
        comp->listview  = NULL;
        comp->selection = NULL;
    }
}

/**
 * Free the memory of the completion set on the client.
 */
void completion_cleanup(Client *c)
{
    if (c->comp) {
        g_slice_free(Completion, c->comp);
        c->comp = NULL;
    }
}

/**
 * Start the completion by creating the required widgets and setting a select
 * function.
 */
gboolean completion_create(Client *c, GListStore *store,
        CompletionSelectFunc selfunc, gboolean back)
{
    GtkListItemFactory *factory;
    GtkRequisition size;
    int height, width;
    Completion *comp = (Completion*)c->comp;
    guint n_items;

    n_items = g_list_model_get_n_items(G_LIST_MODEL(store));

    /* if there is only one match - don't build the list view */
    if (n_items == 1) {
        CompletionItem *item = g_list_model_get_item(G_LIST_MODEL(store), 0);
        if (item) {
            const char *value = completion_item_get_first(item);
            /* call the select function */
            selfunc(c, (char*)value);
            g_object_unref(item);
            g_object_unref(store);
            return FALSE;
        }
    }

    comp->selfunc = selfunc;

    /* Create selection model */
    comp->selection = gtk_single_selection_new(G_LIST_MODEL(store));
    gtk_single_selection_set_autoselect(comp->selection, FALSE);
    gtk_single_selection_set_can_unselect(comp->selection, TRUE);

    /* prepare the list view */
    comp->win      = gtk_scrolled_window_new();
    factory        = gtk_signal_list_item_factory_new();

    g_signal_connect(factory, "setup", G_CALLBACK(setup_listitem), NULL);
    g_signal_connect(factory, "bind", G_CALLBACK(bind_listitem), NULL);

    comp->listview = gtk_list_view_new(GTK_SELECTION_MODEL(comp->selection), factory);

    /* GTK4: CSS providers are added to display, widgets are targeted by name/class in CSS */
    /* The provider is already added to display in vimb_setup(), widget uses CSS selector */
    gtk_widget_set_name(GTK_WIDGET(comp->listview), "completion");

    gtk_box_append(GTK_BOX(gtk_widget_get_parent(GTK_WIDGET(c->statusbar.box))), comp->win);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(comp->win), comp->listview);

    /* Connect selection changed signal */
    g_signal_connect(comp->selection, "selection-changed",
        G_CALLBACK(on_selection_changed), c);

    /* get window dimension */
    gtk_window_get_default_size(GTK_WINDOW(c->window), &width, &height);

    /* Show the list view first */
    gtk_widget_set_visible(comp->listview, TRUE);

    /* this prevents the first item to be placed out of view if the completion
     * is shown */
    while (g_main_context_pending(NULL)) {
        g_main_context_iteration(NULL, TRUE);
    }

    /* use max 1/3 of window height for the completion */
    gtk_widget_get_preferred_size(comp->listview, NULL, &size);
    height /= 3;
    gtk_scrolled_window_set_min_content_height(
        GTK_SCROLLED_WINDOW(comp->win),
        size.height > height ? height : size.height
    );

    c->mode->flags |= FLAG_COMPLETION;

    /* set to -1 to have the cursor on first or last item set in move_cursor */
    comp->active = -1;
    completion_next(c, back);

    gtk_widget_set_visible(comp->win, TRUE);

    return TRUE;
}

/**
 * Initialize the completion system for given client.
 */
void completion_init(Client *c)
{
    /* Allocate memory for the completion struct and save it on the client. */
    c->comp = g_slice_new0(Completion);
}

/**
 * Moves the selection to the next/previous item.
 * If the end/beginning is reached return false and start on the opposite end
 * on the next call.
 */
gboolean completion_next(Client *c, gboolean back)
{
    guint n_items;
    Completion *comp = (Completion*)c->comp;
    guint old_selected;

    if (!comp->selection) {
        return FALSE;
    }

    n_items = g_list_model_get_n_items(G_LIST_MODEL(
        gtk_single_selection_get_model(comp->selection)));

    old_selected = gtk_single_selection_get_selected(comp->selection);

    if (back) {
        comp->active--;
        /* Step back over the beginning. */
        if (comp->active == -1) {
            /* Deselect the current item to show the user the initial typed
             * content. */
            gtk_single_selection_set_selected(comp->selection, GTK_INVALID_LIST_POSITION);
            return FALSE;
        }
        if (comp->active < -1) {
            comp->active = n_items - 1;
        }
    } else {
        comp->active++;
        /* Step over the end. */
        if (comp->active == (int)n_items) {
            gtk_single_selection_set_selected(comp->selection, GTK_INVALID_LIST_POSITION);
            return FALSE;
        }
        if (comp->active >= (int)n_items) {
            comp->active = 0;
        }
    }

    /* Select the item */
    gtk_single_selection_set_selected(comp->selection, comp->active);

    /* Scroll to make the selected item visible */
    if (comp->listview && comp->active >= 0) {
        gtk_list_view_scroll_to(GTK_LIST_VIEW(comp->listview), comp->active,
            GTK_LIST_SCROLL_NONE, NULL);
    }

    /* If selection didn't change (same item selected), manually trigger the callback
     * since selection-changed won't fire */
    if (old_selected == (guint)comp->active && comp->active >= 0) {
        CompletionItem *item = g_list_model_get_item(
            gtk_single_selection_get_model(comp->selection), comp->active);
        if (item) {
            const char *value = completion_item_get_first(item);
            comp->selfunc(c, (char*)value);
            g_object_unref(item);
        }
    }

    return TRUE;
}

static void setup_listitem(GtkListItemFactory *factory, GtkListItem *list_item,
        gpointer user_data)
{
    GtkWidget *box, *label_first;
#ifdef FEATURE_TITLE_IN_COMPLETION
    GtkWidget *label_second;
#endif

    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);

    label_first = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(label_first), 0.0);
    gtk_label_set_ellipsize(GTK_LABEL(label_first), PANGO_ELLIPSIZE_MIDDLE);
    gtk_widget_set_hexpand(label_first, TRUE);
    gtk_box_append(GTK_BOX(box), label_first);

#ifdef FEATURE_TITLE_IN_COMPLETION
    label_second = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(label_second), 0.0);
    gtk_label_set_ellipsize(GTK_LABEL(label_second), PANGO_ELLIPSIZE_END);
    gtk_widget_set_hexpand(label_second, TRUE);
    gtk_box_append(GTK_BOX(box), label_second);
#endif

    gtk_list_item_set_child(list_item, box);
}

static void bind_listitem(GtkListItemFactory *factory, GtkListItem *list_item,
        gpointer user_data)
{
    GtkWidget *box, *label_first;
#ifdef FEATURE_TITLE_IN_COMPLETION
    GtkWidget *label_second;
#endif
    CompletionItem *item;

    box = gtk_list_item_get_child(list_item);
    item = gtk_list_item_get_item(list_item);

    label_first = gtk_widget_get_first_child(box);
    gtk_label_set_text(GTK_LABEL(label_first), completion_item_get_first(item));

#ifdef FEATURE_TITLE_IN_COMPLETION
    label_second = gtk_widget_get_next_sibling(label_first);
    gtk_label_set_text(GTK_LABEL(label_second), completion_item_get_second(item));
#endif
}

static void on_selection_changed(GtkSelectionModel *model, guint position,
        guint n_items, gpointer data)
{
    Client *c = (Client*)data;
    Completion *comp = (Completion*)c->comp;
    guint selected;
    CompletionItem *item;

    selected = gtk_single_selection_get_selected(GTK_SINGLE_SELECTION(model));
    if (selected == GTK_INVALID_LIST_POSITION) {
        return;
    }

    item = g_list_model_get_item(gtk_single_selection_get_model(GTK_SINGLE_SELECTION(model)), selected);
    if (item) {
        const char *value = completion_item_get_first(item);
        comp->selfunc(c, (char*)value);
        g_object_unref(item);
    }
}
