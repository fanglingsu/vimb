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

#include "main.h"
#include "completion.h"
#include "util.h"
#include "history.h"
#include "bookmark.h"
#include "command.h"
#include "setting.h"

#define TAG_INDICATOR '!'
#define COMP_ITEM 0

extern VbCore vb;

static struct {
    GtkWidget *win;
    GtkWidget *tree;
    int       count;   /* command count before the completed content */
    char      *prefix; /* prefix that marks the completion ':', '/', ':open', ... */
    int       active;  /* number of the current active tree item */
    char      *text;   /* text of the current active tree item */
} comp;

static GtkTreeModel *get_tree_model(GList *source);
static void init_completion(GtkTreeModel *model);
static void show(gboolean back);
static void move_cursor(gboolean back);
static gboolean tree_selection_func(GtkTreeSelection *selection,
    GtkTreeModel *model, GtkTreePath *path, gboolean selected, gpointer data);

gboolean completion_complete(gboolean back)
{
    VbInputType type;
    const char *input, *prefix, *suffix;
    GList *source = NULL;
    GtkTreeModel *model = NULL;

    input = GET_TEXT();
    type  = vb_get_input_parts(input, &prefix, &suffix);

    if (vb.state.mode & VB_MODE_COMPLETE) {
        if (comp.text && !strcmp(input, comp.text)) {
            /* step through the next/prev completion item */
            move_cursor(back);
            return true;
        }
        /* if current input isn't the content of the completion item, stop
         * completion and start it after that again */
        completion_clean();
    }

    /* don't disturb other command sub modes - complete only if no sub mode
     * is set before */
    if (vb.state.mode != VB_MODE_COMMAND) {
        return false;
    }

    if (type == VB_INPUT_SET) {
        source = g_list_sort(setting_get_by_prefix(suffix), (GCompareFunc)g_strcmp0);
        if (!g_list_first(source)) {
            return false;
        }
        model  = get_tree_model(source);
        g_list_free(source);
    } else if (type == VB_INPUT_OPEN || type == VB_INPUT_TABOPEN) {
        /* if search string begins with TAG_INDICATOR lookup the bookmarks */
        if (suffix && *suffix == TAG_INDICATOR) {
            source = g_list_sort(bookmark_get_by_tags(suffix + 1), (GCompareFunc)g_strcmp0);
        } else {
            source = history_get_by_tags(HISTORY_URL, suffix);
        }
        if (!g_list_first(source)) {
            return false;
        }
        model = get_tree_model(source);
        g_list_free_full(source, (GDestroyNotify)g_free);
    } else if (type == VB_INPUT_COMMAND) {
        char *command = NULL;
        /* remove counts before command and save it to print it later in inputbox */
        comp.count = g_ascii_strtoll(suffix, &command, 10);

        source = g_list_sort(command_get_by_prefix(command), (GCompareFunc)g_strcmp0);
        if (!g_list_first(source)) {
            return false;
        }
        model = get_tree_model(source);
        g_list_free(source);
    } else if (type == VB_INPUT_SEARCH_FORWARD || type == VB_INPUT_SEARCH_BACKWARD) {
        source = g_list_sort(history_get_by_tags(HISTORY_SEARCH, suffix), (GCompareFunc)g_strcmp0);
        if (!g_list_first(source)) {
            return false;
        }
        model  = get_tree_model(source);
        g_list_free_full(source, (GDestroyNotify)g_free);
    }

    vb_set_mode(VB_MODE_COMMAND | VB_MODE_COMPLETE, false);

    OVERWRITE_STRING(comp.prefix, prefix);
    init_completion(model);
    show(back);

    return true;
}

void completion_clean(void)
{
    if (comp.win) {
        gtk_widget_destroy(comp.win);
        comp.win = comp.tree = NULL;
    }
    OVERWRITE_STRING(comp.prefix, NULL);
    OVERWRITE_STRING(comp.text, NULL);
    comp.count = 0;

    /* remove completion flag from mode */
    vb.state.mode &= ~VB_MODE_COMPLETE;
}

static GtkTreeModel *get_tree_model(GList *source)
{
    GtkTreeIter iter;
    GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);

    for (GList *l = source; l; l = l->next) {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter, COMP_ITEM, l->data, -1);
    }
    return GTK_TREE_MODEL(store);
}

static void init_completion(GtkTreeModel *model)
{
    GtkCellRenderer *renderer;
    GtkTreeSelection *selection;
    GtkRequisition size;
    int height;

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

    /* prepare the selection */
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(comp.tree));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
    gtk_tree_selection_set_select_function(selection, tree_selection_func, NULL, NULL);

    /* prepare the column renderer */
    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer,
        "font-desc", vb.style.comp_font,
        "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
        NULL
    );
    gtk_tree_view_insert_column_with_attributes(
        GTK_TREE_VIEW(comp.tree), -1, "", renderer,
        "text", COMP_ITEM,
        NULL
    );

    /* use max 1/3 of window height for the completion */
#ifdef HAS_GTK3
    gtk_widget_get_preferred_size(comp.tree, NULL, &size);
    gtk_window_get_size(GTK_WINDOW(vb.gui.window), NULL, &height);
    height /= 3;
    gtk_scrolled_window_set_min_content_height(
        GTK_SCROLLED_WINDOW(comp.win),
        size.height > height ? height : size.height
    );
#else
    gtk_widget_size_request(comp.tree, &size);
    gtk_window_get_size(GTK_WINDOW(vb.gui.window), NULL, &height);
    height /= 3;
    if (size.height > height) {
        gtk_widget_set_size_request(comp.win, -1, height);
    }
#endif
}

static void show(gboolean back)
{
    /* this prevents the first item to be placed out of view if the completion
     * is shown */
    gtk_widget_show_all(comp.win);
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }

    /* set to -1 to have the cursor on first or last item set in move_cursor */
    comp.active = -1;
    move_cursor(back);
}

static void move_cursor(gboolean back)
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

static gboolean tree_selection_func(GtkTreeSelection *selection,
    GtkTreeModel *model, GtkTreePath *path, gboolean selected, gpointer data)
{
    char *value;
    GtkTreeIter iter;

    /* if not selected means the item is going to be selected which we are
     * interested in */
    if (!selected && gtk_tree_model_get_iter(model, &iter, path)) {
        gtk_tree_model_get(model, &iter, COMP_ITEM, &value, -1);
        /* save the content of the selected item so wen can access it easy */
        if (comp.text) {
            g_free(comp.text);
            comp.text = NULL;
        }
        if (comp.count) {
            comp.text = g_strdup_printf("%s%d%s", comp.prefix, comp.count, value);
        } else {
            comp.text = g_strdup_printf("%s%s", comp.prefix, value);
        }
        /* print the text also into inputbox */
        PUT_TEXT(comp.text);
        g_free(value);
    }

    return true;
}
