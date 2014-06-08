/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2014 Daniel Carl
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

#include "config.h"
#include "main.h"
#include "completion.h"

extern VbCore vb;

static struct {
    GtkWidget *win;
    GtkWidget *tree;
    int       active;  /* number of the current active tree item */
    CompletionSelectFunc selfunc;
} comp;

static gboolean tree_selection_func(GtkTreeSelection *selection,
    GtkTreeModel *model, GtkTreePath *path, gboolean selected, gpointer data);


gboolean completion_create(GtkTreeModel *model, CompletionSelectFunc selfunc,
    gboolean back)
{
    GtkCellRenderer *renderer;
    GtkTreeSelection *selection;
    GtkTreeViewColumn *column;
    GtkRequisition size;
    GtkTreePath *path;
    GtkTreeIter iter;
    int height, width;

    /* if there is only one match - don't build the tree view */
    if (gtk_tree_model_iter_n_children(model, NULL) == 1) {
        char *value;
        path = gtk_tree_path_new_from_indices(0, -1);
        if (gtk_tree_model_get_iter(model, &iter, path)) {
            gtk_tree_model_get(model, &iter, COMPLETION_STORE_FIRST, &value, -1);

            /* call the select function */
            selfunc(value);

            g_free(value);
            g_object_unref(model);

            return false;
        }
    }

    comp.selfunc = selfunc;

    /* prepare the tree view */
    comp.win  = gtk_scrolled_window_new(NULL, NULL);
    comp.tree = gtk_tree_view_new();
#ifndef HAS_GTK3
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(comp.win), GTK_POLICY_NEVER, GTK_POLICY_NEVER);
#endif
    gtk_box_pack_end(GTK_BOX(vb.gui.box), comp.win, false, false, 0);
    gtk_container_add(GTK_CONTAINER(comp.win), comp.tree);

    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(comp.tree), false);
    /* we have only on line per item so we can use the faster fixed heigh mode */
    gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(comp.tree), true);
    gtk_tree_view_set_model(GTK_TREE_VIEW(comp.tree), model);
    g_object_unref(model);

    VB_WIDGET_OVERRIDE_TEXT(comp.tree, VB_GTK_STATE_NORMAL, &vb.style.comp_fg[VB_COMP_NORMAL]);
    VB_WIDGET_OVERRIDE_BASE(comp.tree, VB_GTK_STATE_NORMAL, &vb.style.comp_bg[VB_COMP_NORMAL]);
    VB_WIDGET_OVERRIDE_TEXT(comp.tree, VB_GTK_STATE_SELECTED, &vb.style.comp_fg[VB_COMP_ACTIVE]);
    VB_WIDGET_OVERRIDE_BASE(comp.tree, VB_GTK_STATE_SELECTED, &vb.style.comp_bg[VB_COMP_ACTIVE]);
    VB_WIDGET_OVERRIDE_TEXT(comp.tree, VB_GTK_STATE_ACTIVE, &vb.style.comp_fg[VB_COMP_ACTIVE]);
    VB_WIDGET_OVERRIDE_BASE(comp.tree, VB_GTK_STATE_ACTIVE, &vb.style.comp_bg[VB_COMP_ACTIVE]);

    /* prepare the selection */
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(comp.tree));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
    gtk_tree_selection_set_select_function(selection, tree_selection_func, NULL, NULL);

    /* get window dimension */
    gtk_window_get_size(GTK_WINDOW(vb.gui.window), &width, &height);

    /* prepare first column */
    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_append_column(GTK_TREE_VIEW(comp.tree), column);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer,
        "font-desc", vb.style.comp_font,
        "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
        NULL
    );
    gtk_tree_view_column_pack_start(column, renderer, true);
    gtk_tree_view_column_add_attribute(column, renderer, "text", COMPLETION_STORE_FIRST);
    gtk_tree_view_column_set_min_width(column, 2 * width/3);

    /* prepare second column */
#ifdef FEATURE_TITLE_IN_COMPLETION
    column = gtk_tree_view_column_new();
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_append_column(GTK_TREE_VIEW(comp.tree), column);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer,
        "font-desc", vb.style.comp_font,
        "ellipsize", PANGO_ELLIPSIZE_END,
        NULL
    );
    gtk_tree_view_column_pack_start(column, renderer, true);
    gtk_tree_view_column_add_attribute(column, renderer, "text", COMPLETION_STORE_SECOND);
#endif

    /* to set the height for the treeview the tree must be realized first */
    gtk_widget_show(comp.tree);

    /* this prevents the first item to be placed out of view if the completion
     * is shown */
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }

    /* use max 1/3 of window height for the completion */
#ifdef HAS_GTK3
    gtk_widget_get_preferred_size(comp.tree, NULL, &size);
    height /= 3;
    gtk_scrolled_window_set_min_content_height(
        GTK_SCROLLED_WINDOW(comp.win),
        size.height > height ? height : size.height
    );
#else
    gtk_widget_size_request(comp.tree, &size);
    height /= 3;
    if (size.height > height) {
        gtk_widget_set_size_request(comp.win, -1, height);
    }
#endif

    vb.mode->flags |= FLAG_COMPLETION;

    /* set to -1 to have the cursor on first or last item set in move_cursor */
    comp.active = -1;
    completion_next(back);

    gtk_widget_show(comp.win);

    return true;
}

void completion_next(gboolean back)
{
    int rows;
    GtkTreePath *path;
    GtkTreeView *tree = GTK_TREE_VIEW(comp.tree);

    rows = gtk_tree_model_iter_n_children(gtk_tree_view_get_model(tree), NULL);
    if (back) {
        /* step back */
        if (--comp.active < 0) {
            comp.active = rows - 1;
        }
    } else {
        /* step forward */
        if (++comp.active >= rows) {
            comp.active = 0;
        }
    }

    /* get new path and move cursor to it */
    path = gtk_tree_path_new_from_indices(comp.active, -1);
    gtk_tree_view_set_cursor(tree, path, NULL, false);
    gtk_tree_path_free(path);
}

void completion_clean(void)
{
    vb.mode->flags &= ~FLAG_COMPLETION;
    if (comp.win) {
        gtk_widget_destroy(comp.win);
        comp.win = comp.tree = NULL;
    }
}

static gboolean tree_selection_func(GtkTreeSelection *selection,
    GtkTreeModel *model, GtkTreePath *path, gboolean selected, gpointer data)
{
    char *value;
    GtkTreeIter iter;

    /* if not selected means the item is going to be selected which we are
     * interested in */
    if (!selected && gtk_tree_model_get_iter(model, &iter, path)) {
        gtk_tree_model_get(model, &iter, COMPLETION_STORE_FIRST, &value, -1);

        comp.selfunc(value);

        g_free(value);
    }

    return true;
}
