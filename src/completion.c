/**
 * vimp - a webkit based vim like browser.
 *
 * Copyright (C) 2012 Daniel Carl
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

typedef struct {
    GtkWidget* label;
    GtkWidget* event;
} Completion;

static GList* completion_init_completion(GList* target, GList* source);
static GList* completion_update(GList* completion, GList* active, gboolean back);
static void completion_show(gboolean back);
static void completion_set_color(Completion* completion, gchar* fg, gchar* bg);
static void completion_set_entry_text(Completion* completion);
static Completion* completion_get_new(const gchar* label);

gboolean completion_complete(const CompletionType type, gboolean back)
{
    GList* source = NULL;

    if (vp.comps.completions
        && vp.comps.active
        && (vp.state.mode & VP_MODE_COMPLETE)
    ) {
        /* updatecompletions */
        vp.comps.active = completion_update(vp.comps.completions, vp.comps.active, back);

        return TRUE;
    }
    /* create new completion */
    switch (type) {
        case COMPLETE_COMMAND:
            source = g_hash_table_get_keys(vp.behave.commands);
            source = g_list_sort(source, (GCompareFunc)g_strcmp0);
            break;
    }

#if _HAS_GTK3
    vp.gui.compbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_set_homogeneous(GTK_BOX(vp.gui.compbox), TRUE);
#else
    vp.gui.compbox = gtk_vbox_new(TRUE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(vp.gui.box), vp.gui.compbox, FALSE, FALSE, 0);

    vp.comps.completions = completion_init_completion(vp.comps.completions, source);

    if (!vp.comps.completions) {
        return FALSE;
    }
    /* set mode flag for complation */
    vp.state.mode |= VP_MODE_COMPLETE;
    completion_show(back);

    return TRUE;
}

void completion_clean(void)
{
    for (GList *l = vp.comps.completions; l; l = l->next) {
        g_free(l->data);
    }
    g_list_free(vp.comps.completions);

    if (vp.gui.compbox) {
        gtk_widget_destroy(vp.gui.compbox);
    }
    vp.comps.completions = NULL;
    vp.comps.active = NULL;

    /* remove completion flag from mode */
    vp.state.mode &= ~VP_MODE_COMPLETE;
}

static GList* completion_init_completion(GList* target, GList* source)
{
    const gchar* input = gtk_entry_get_text(GTK_ENTRY(vp.gui.inputbox));
    gchar* data = NULL;
    gboolean match;
    gchar **token = NULL;

    if (input && input[0] == ':') {
        input++;
    }
    token = g_strsplit(input, " ", -1);

    for (GList* l = source; l; l = l->next) {
        data = l->data;
        match = FALSE;
        if (*input == 0) {
            match = TRUE;
        } else {
            for (gint i = 0; token[i]; i++) {
                if (g_str_has_prefix(data, token[i])) {
                    match = TRUE;
                } else {
                    match = FALSE;
                    break;
                }
            }
        }
        if (match) {
            Completion* c = completion_get_new(data);
            gtk_box_pack_start(GTK_BOX(vp.gui.compbox), c->event, FALSE, FALSE, 0);
            target = g_list_append(target, c);
        }
    }
    g_strfreev(token);

    return target;
}

static GList* completion_update(GList* completion, GList* active, gboolean back)
{
    GList *old, *new;
    Completion *c;

    int length = g_list_length(completion);
    int max = 15;
    int items = MAX(length, max);
    int r = (max) % 2;
    int offset = max / 2 - 1 + r;

    old = active;
    int position = g_list_position(completion, active) + 1;
    if (back) {
        if (!(new = old->prev)) {
            new = g_list_last(completion);
        }
        if (position - 1 > offset && position < items - offset + r) {
            c = g_list_nth(completion, position - offset - 2)->data;
            gtk_widget_show_all(c->event);
            c = g_list_nth(completion, position + offset - r)->data;
            gtk_widget_hide(c->event);
        } else if (position == 1) {
            int i = 0;
            for (GList *l = g_list_first(completion); l && i<max; l=l->next, i++) {
                gtk_widget_hide(((Completion*)l->data)->event);
            }
            gtk_widget_show(vp.gui.compbox);
            i = 0;
            for (GList *l = g_list_last(completion); l && i<max ;l=l->prev, i++) {
                c = l->data;
                gtk_widget_show_all(c->event);
            }
        }
    } else {
        if (!(new = old->next)) {
            new = g_list_first(completion);
        }
        if (position > offset && position < items - offset - 1 + r) {
            c = g_list_nth(completion, position - offset - 1)->data;
            gtk_widget_hide(c->event);
            c = g_list_nth(completion, position + offset + 1 - r)->data;
            gtk_widget_show_all(c->event);
        } else if (position == items || position  == 1) {
            int i = 0;
            for (GList *l = g_list_last(completion); l && i<max; l=l->prev) {
                gtk_widget_hide(((Completion*)l->data)->event);
            }
            gtk_widget_show(vp.gui.compbox);
            i = 0;
            for (GList *l = g_list_first(completion); l && i<max ;l=l->next, i++) {
                gtk_widget_show_all(((Completion*)l->data)->event);
            }
        }
    }

    gchar* fg = "#77ff77";
    gchar* bg = "#333333";
    completion_set_color(old->data, fg, bg);
    completion_set_color(new->data, fg, bg);

    active = new;
    completion_set_entry_text(active->data);
    return active;
}

/* allow to chenge the direction of display */
static void completion_show(gboolean back)
{
    /* TODO make this configurable */
    const gint max_items = 15;
    gint i = 0;
    if (back) {
        vp.comps.active = g_list_last(vp.comps.completions);
        for (GList *l = vp.comps.active; l && i < max_items; l = l->prev, i++) {
            gtk_widget_show_all(((Completion*)l->data)->event);
        }
    } else {
        vp.comps.active = g_list_first(vp.comps.completions);
        for (GList *l = vp.comps.active; l && i < max_items; l = l->next, i++) {
            gtk_widget_show_all(((Completion*)l->data)->event);
        }
    }
    if (vp.comps.active != NULL) {
        gchar* fg = "#77ff77";
        gchar* bg = "#333333";
        completion_set_color(vp.comps.active->data, fg, bg);
        completion_set_entry_text(vp.comps.active->data);
        gtk_widget_show(vp.gui.compbox);
    }
}

static void completion_set_color(Completion* completion, gchar* fg, gchar* bg)
{
    GdkColor color;
    gdk_color_parse(fg, &color);
    gtk_widget_modify_fg(completion->label, GTK_STATE_NORMAL, &color);
    gdk_color_parse(bg, &color);
    gtk_widget_modify_bg(completion->label, GTK_STATE_NORMAL, &color);
}

static void completion_set_entry_text(Completion* completion)
{
    const gchar* text;
    const gchar* pre = ":";
    gint len = strlen(pre);

    text = gtk_label_get_text(GTK_LABEL(completion->label));
    gtk_entry_set_text(GTK_ENTRY(vp.gui.inputbox), pre);
    gtk_editable_insert_text(GTK_EDITABLE(vp.gui.inputbox), text, -1, &len);
    gtk_editable_set_position(GTK_EDITABLE(vp.gui.inputbox), -1);
}

static Completion* completion_get_new(const gchar* label)
{
    /* TODO make this configurable */
    const gint padding = 2;
    Completion* c = g_new0(Completion, 1);

    c->label = gtk_label_new(label);
    c->event = gtk_event_box_new();

#if _HAS_GTK3
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_set_homogeneous(GTK_BOX(hbox), TRUE);
#else
    GtkWidget *hbox = gtk_hbox_new(TRUE, 0);
#endif

    gtk_box_pack_start(GTK_BOX(hbox), c->label, TRUE, TRUE, 5);
    gtk_label_set_ellipsize(GTK_LABEL(c->label), PANGO_ELLIPSIZE_MIDDLE);
    gtk_misc_set_alignment(GTK_MISC(c->label), 0.0, 0.5);

    gchar* fg = "#77ff77";
    gchar* bg = "#333333";
    completion_set_color(c, fg, bg);

    GtkWidget *alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), padding, padding, padding, padding);
    gtk_container_add(GTK_CONTAINER(alignment), hbox);
    gtk_container_add(GTK_CONTAINER(c->event), alignment);

    return c;
}
