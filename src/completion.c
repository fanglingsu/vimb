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

typedef struct {
    GtkWidget               *win, *tree;
    int                     active;  /* number of the current active tree item */
    CompletionSelectFunc    selfunc;
} Completion;

static gboolean tree_selection_func(GtkTreeSelection *selection,
    GtkTreeModel *model, GtkTreePath *path, gboolean selected, gpointer data);

extern struct Vimb vb;


/**
 * Stop the completion and reset temporary used data.
 */
void completion_clean(Client *c)
{
    Completion *comp = (Completion*)c->comp;
    c->mode->flags  &= ~FLAG_COMPLETION;

    if (comp->win) {
        gtk_widget_destroy(comp->win);
        comp->win  = NULL;
        comp->tree = NULL;
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
gboolean completion_create(Client *c, GtkTreeModel *model,
        CompletionSelectFunc selfunc, gboolean back)
{
    GtkCellRenderer *renderer;
    GtkTreeSelection *selection;
    GtkTreeViewColumn *column;
    GtkRequisition size;
    GtkTreePath *path;
    GtkTreeIter iter;
    int height, width;
    Completion *comp = (Completion*)c->comp;

    /* if there is only one match - don't build the tree view */
    if (gtk_tree_model_iter_n_children(model, NULL) == 1) {
        char *value;
        path = gtk_tree_path_new_from_indices(0, -1);
        if (gtk_tree_model_get_iter(model, &iter, path)) {
            gtk_tree_model_get(model, &iter, COMPLETION_STORE_FIRST, &value, -1);

            /* call the select function */
            selfunc(c, value);

            g_free(value);
            g_object_unref(model);

            return FALSE;
        }
    }

    comp->selfunc = selfunc;

    /* prepare the tree view */
    comp->win  = gtk_scrolled_window_new(NULL, NULL);
    comp->tree = gtk_tree_view_new_with_model(model);

    gtk_style_context_add_provider(gtk_widget_get_style_context(comp->tree),
            GTK_STYLE_PROVIDER(vb.style_provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_widget_set_name(GTK_WIDGET(comp->tree), "completion");

    gtk_box_pack_end(GTK_BOX(gtk_widget_get_parent(GTK_WIDGET(c->statusbar.box))), comp->win, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(comp->win), comp->tree);

    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(comp->tree), FALSE);
    /* we have only on line per item so we can use the faster fixed heigh mode */
    gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(comp->tree), TRUE);
    g_object_unref(model);

    /* prepare the selection */
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(comp->tree));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
    gtk_tree_selection_set_select_function(selection, tree_selection_func, c, NULL);

    /* get window dimension */
    gtk_window_get_size(GTK_WINDOW(c->window), &width, &height);

    /* prepare first column */
    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_append_column(GTK_TREE_VIEW(comp->tree), column);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer,
        "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
        NULL
    );
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_add_attribute(column, renderer, "text", COMPLETION_STORE_FIRST);
    gtk_tree_view_column_set_min_width(column, 2 * width/3);

    /* prepare second column */
#ifdef FEATURE_TITLE_IN_COMPLETION
    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_append_column(GTK_TREE_VIEW(comp->tree), column);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer,
        "ellipsize", PANGO_ELLIPSIZE_END,
        NULL
    );
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_add_attribute(column, renderer, "text", COMPLETION_STORE_SECOND);
#endif

    /* to set the height for the treeview the tree must be realized first */
    gtk_widget_show(comp->tree);

    /* this prevents the first item to be placed out of view if the completion
     * is shown */
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }

    /* use max 1/3 of window height for the completion */
    gtk_widget_get_preferred_size(comp->tree, NULL, &size);
    height /= 3;
    gtk_scrolled_window_set_min_content_height(
        GTK_SCROLLED_WINDOW(comp->win),
        size.height > height ? height : size.height
    );

    c->mode->flags |= FLAG_COMPLETION;

    /* set to -1 to have the cursor on first or last item set in move_cursor */
    comp->active = -1;
    completion_next(c, back);

    gtk_widget_show(comp->win);

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
 * Moves the selection to the next/previous tree item.
 * If the end/beginning is reached return false and start on the opposite end
 * on the next call.
 */
gboolean completion_next(Client *c, gboolean back)
{
    int rows;
    GtkTreePath *path;
    GtkTreeView *tree = GTK_TREE_VIEW(((Completion*)c->comp)->tree);
    Completion *comp  = (Completion*)c->comp;

    rows = gtk_tree_model_iter_n_children(gtk_tree_view_get_model(tree), NULL);
    if (back) {
        comp->active--;
        /* Step back over the beginning. */
        if (comp->active == -1) {
            /* Deselect the current item to show the user the initial typed
             * content. */
            gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(comp->tree)));

            return FALSE;
        }
        if (comp->active < -1) {
            comp->active = rows - 1;
        }
    } else {
        comp->active++;
        /* Step over the end. */
        if (comp->active == rows) {
            gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(comp->tree)));

            return FALSE;
        }
        if (comp->active >= rows) {
            comp->active = 0;
        }
    }

    /* get new path and move cursor to it */
    path = gtk_tree_path_new_from_indices(comp->active, -1);
    gtk_tree_view_set_cursor(tree, path, NULL, FALSE);
    gtk_tree_path_free(path);

    return TRUE;
}

static gboolean tree_selection_func(GtkTreeSelection *selection,
    GtkTreeModel *model, GtkTreePath *path, gboolean selected, gpointer data)
{
    char *value;
    GtkTreeIter iter;
    Completion *comp = (Completion*)((Client*)data)->comp;

    /* if not selected means the item is going to be selected which we are
     * interested in */
    if (!selected && gtk_tree_model_get_iter(model, &iter, path)) {
        gtk_tree_model_get(model, &iter, COMPLETION_STORE_FIRST, &value, -1);

        comp->selfunc((Client*)data, value);

        g_free(value);
        /* TODO update comp->active on select by mouse to continue with <Tab>
         * or <S-Tab> from selected item on. */
    }

    return TRUE;
}
