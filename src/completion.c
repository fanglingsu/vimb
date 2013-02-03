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
    gchar*     prefix;
} Completion;

static GList* completion_init_completion(GList* target, GList* source, const gchar* prefix);
static GList* completion_update(GList* completion, GList* active, gboolean back);
static void completion_show(gboolean back);
static void completion_set_color(Completion* completion, const VpColor* fg, const VpColor* bg, PangoFontDescription* font);
static void completion_set_entry_text(Completion* completion);
static Completion* completion_get_new(const gchar* label, const gchar* prefix);

gboolean completion_complete(gboolean back)
{
    const gchar* input = NULL;
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
#ifdef HAS_GTK3
    vp.gui.compbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_set_homogeneous(GTK_BOX(vp.gui.compbox), TRUE);
#else
    vp.gui.compbox = gtk_vbox_new(TRUE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(vp.gui.box), vp.gui.compbox, FALSE, FALSE, 0);

    input = GET_TEXT();
    /* TODO move these decision to a more generic place */
    if (g_str_has_prefix(input, ":set ")) {
        source = g_hash_table_get_keys(vp.settings);
        source = g_list_sort(source, (GCompareFunc)g_strcmp0);
        vp.comps.completions = completion_init_completion(vp.comps.completions, source, ":set ");
    } else {
        source = g_hash_table_get_keys(vp.behave.commands);
        source = g_list_sort(source, (GCompareFunc)g_strcmp0);
        vp.comps.completions = completion_init_completion(vp.comps.completions, source, ":");
    }

    if (!vp.comps.completions) {
        return FALSE;
    }
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
        vp.gui.compbox = NULL;
    }
    vp.comps.completions = NULL;
    vp.comps.active = NULL;
    vp.comps.count = 0;

    /* remove completion flag from mode */
    vp.state.mode &= ~VP_MODE_COMPLETE;
}

static GList* completion_init_completion(GList* target, GList* source, const gchar* prefix)
{
    const gchar* input = GET_TEXT();
    gchar* command = NULL;
    gchar* data = NULL;
    gboolean match;
    gchar **token = NULL;

    /* skip prefix for completion */
    if (g_str_has_prefix(input, prefix)) {
        input = input + strlen(prefix);
    }

    /* remove counts before command and save it to print it later in inputbox */
    vp.comps.count = g_ascii_strtoll(input, &command, 10);

    token = g_strsplit(command, " ", -1);

    for (GList* l = source; l; l = l->next) {
        data = l->data;
        match = FALSE;
        if (*command == 0) {
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
            Completion* c = completion_get_new(data, prefix);
            gtk_box_pack_start(GTK_BOX(vp.gui.compbox), c->event, TRUE, TRUE, 0);
            /* use prepend because that faster */
            target = g_list_prepend(target, c);
        }
    }

    target = g_list_reverse(target);
    g_strfreev(token);

    return target;
}

static GList* completion_update(GList* completion, GList* active, gboolean back)
{
    GList *old, *new;
    Completion *c;

    gint length = g_list_length(completion);
    gint max    = vp.config.max_completion_items;
    gint items  = MAX(length, max);
    gint r      = (max) % 2;
    gint offset = max / 2 - 1 + r;

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
            for (GList *l = g_list_first(completion); l && i < max; l = l->next, i++) {
                gtk_widget_hide(((Completion*)l->data)->event);
            }
            gtk_widget_show(vp.gui.compbox);
            i = 0;
            for (GList *l = g_list_last(completion); l && i < max; l = l->prev, i++) {
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
            for (GList *l = g_list_last(completion); l && i < max; l = l->prev) {
                gtk_widget_hide(((Completion*)l->data)->event);
            }
            gtk_widget_show(vp.gui.compbox);
            i = 0;
            for (GList *l = g_list_first(completion); l && i < max; l = l->next, i++) {
                gtk_widget_show_all(((Completion*)l->data)->event);
            }
        }
    }

    completion_set_color(
        old->data,
        &vp.style.comp_fg[VP_COMP_NORMAL],
        &vp.style.comp_bg[VP_COMP_NORMAL],
        vp.style.comp_font[VP_COMP_NORMAL]
    );
    completion_set_color(
        new->data,
        &vp.style.comp_fg[VP_COMP_ACTIVE],
        &vp.style.comp_bg[VP_COMP_ACTIVE],
        vp.style.comp_font[VP_COMP_ACTIVE]
    );

    active = new;
    completion_set_entry_text(active->data);
    return active;
}

/* allow to chenge the direction of display */
static void completion_show(gboolean back)
{
    guint max = vp.config.max_completion_items;
    gint i = 0;
    if (back) {
        vp.comps.active = g_list_last(vp.comps.completions);
        for (GList *l = vp.comps.active; l && i < max; l = l->prev, i++) {
            gtk_widget_show_all(((Completion*)l->data)->event);
        }
    } else {
        vp.comps.active = g_list_first(vp.comps.completions);
        for (GList *l = vp.comps.active; l && i < max; l = l->next, i++) {
            gtk_widget_show_all(((Completion*)l->data)->event);
        }
    }
    if (vp.comps.active != NULL) {
        completion_set_color(
            vp.comps.active->data,
            &vp.style.comp_fg[VP_COMP_ACTIVE],
            &vp.style.comp_bg[VP_COMP_ACTIVE],
            vp.style.comp_font[VP_COMP_ACTIVE]
        );
        completion_set_entry_text(vp.comps.active->data);
        gtk_widget_show(vp.gui.compbox);
    }
}

static void completion_set_color(Completion* completion, const VpColor* fg, const VpColor* bg, PangoFontDescription* font)
{
    VP_WIDGET_OVERRIDE_COLOR(completion->label, GTK_STATE_NORMAL, fg);
    VP_WIDGET_OVERRIDE_BACKGROUND(completion->event, GTK_STATE_NORMAL, bg);
    /* TODO is it really necessary to set the font for each item */
    VP_WIDGET_OVERRIDE_FONT(completion->label, font);
}

static void completion_set_entry_text(Completion* completion)
{
    GString* string = g_string_new(completion->prefix);
    const gchar* text;

    text = gtk_label_get_text(GTK_LABEL(completion->label));

    /* print the previous typed command count into inputbox too */
    if (vp.comps.count) {
        g_string_append_printf(string, "%d", vp.comps.count);
    }
    g_string_append(string, text);
    gtk_entry_set_text(GTK_ENTRY(vp.gui.inputbox), string->str);
    gtk_editable_set_position(GTK_EDITABLE(vp.gui.inputbox), -1);

    g_string_free(string, TRUE);
}

static Completion* completion_get_new(const gchar* label, const gchar* prefix)
{
    const gint padding = 2;
    Completion* c = g_new0(Completion, 1);

    c->label  = gtk_label_new(label);
    c->event  = gtk_event_box_new();
    c->prefix = g_strdup(prefix);

#ifdef HAS_GTK3
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_set_homogeneous(GTK_BOX(hbox), TRUE);
#else
    GtkWidget *hbox = gtk_hbox_new(TRUE, 0);
#endif

    gtk_box_pack_start(GTK_BOX(hbox), c->label, TRUE, TRUE, 5);
    gtk_label_set_ellipsize(GTK_LABEL(c->label), PANGO_ELLIPSIZE_MIDDLE);
    gtk_misc_set_alignment(GTK_MISC(c->label), 0.0, 0.5);

    completion_set_color(
        c,
        &vp.style.comp_fg[VP_COMP_NORMAL],
        &vp.style.comp_bg[VP_COMP_NORMAL],
        vp.style.comp_font[VP_COMP_NORMAL]
    );

    GtkWidget *alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), padding, padding, padding, padding);
    gtk_container_add(GTK_CONTAINER(alignment), hbox);
    gtk_container_add(GTK_CONTAINER(c->event), alignment);

    return c;
}
