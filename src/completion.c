/**
 * vimp - a webkit based vim like browser.
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

#include "completion.h"
#include "util.h"
#include "url_history.h"

typedef gboolean (*Comp_Func)(char*, const char*);
typedef struct {
    GtkWidget* label;
    GtkWidget* event;
    char*     prefix;
} Completion;

static GList* completion_init_completion(Client* c, GList* target, GList* source,
    Comp_Func func, const char* input, const char* prefix);
static GList* completion_update(Client* c, GList* completion, GList* active, gboolean back);
static void completion_show(Client* c, gboolean back);
static void completion_set_entry_text(Client* c, Completion* completion);
static char* completion_get_text(Client* c, Completion* completion);
static Completion* completion_get_new(const char* label, const char* prefix);
static void completion_free(Completion* completion);

gboolean completion_complete(Client* c, gboolean back)
{
    const char* input = gtk_entry_get_text(GTK_ENTRY(c->gui.inputbox));
    GList* source = NULL;

    if (c->comps.completions
        && c->comps.active
        && (c->state.mode & VP_MODE_COMPLETE)
    ) {
        char* text = completion_get_text(c, (Completion*)c->comps.active->data);
        if (!strcmp(input, text)) {
            /* updatecompletions */
            c->comps.active = completion_update(c, c->comps.completions, c->comps.active, back);
            g_free(text);
            return TRUE;
        } else {
            g_free(text);
            /* if current input isn't the content of the completion item */
            completion_clean(c);
        }
    }

    /* create new completion */
#ifdef HAS_GTK3
    c->gui.compbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_set_homogeneous(GTK_BOX(c->gui.compbox), TRUE);
#else
    c->gui.compbox = gtk_vbox_new(TRUE, 0);
#endif
    gtk_box_pack_start(GTK_BOX(c->gui.box), c->gui.compbox, FALSE, FALSE, 0);

    /* TODO move these decision to a more generic place */
    if (!strncmp(input, ":set ", 5)) {
        source = g_hash_table_get_keys(core.settings);
        source = g_list_sort(source, (GCompareFunc)g_strcmp0);
        c->comps.completions = completion_init_completion(
            c,
            c->comps.completions, source, (Comp_Func)g_str_has_prefix, &input[5], ":set "
        );
    } else if (!strncmp(input, ":open ", 6)) {
        url_history_get_all(&source);
        c->comps.completions = completion_init_completion(
            c,
            c->comps.completions, source, (Comp_Func)util_strcasestr, &input[6], ":open "
        );
        g_list_free(source);
    } else if (!strncmp(input, ":tabopen ", 9)) {
        url_history_get_all(&source);
        c->comps.completions = completion_init_completion(
            c,
            c->comps.completions, source, (Comp_Func)util_strcasestr, &input[9], ":tabopen "
        );
    } else {
        source = g_hash_table_get_keys(core.behave.commands);
        source = g_list_sort(source, (GCompareFunc)g_strcmp0);
        c->comps.completions = completion_init_completion(
            c,
            c->comps.completions, source, (Comp_Func)g_str_has_prefix, &input[1], ":"
        );
    }

    if (!c->comps.completions) {
        return FALSE;
    }
    completion_show(c, back);

    return TRUE;
}

void completion_clean(Client* c)
{
    g_list_free_full(c->comps.completions, (GDestroyNotify)completion_free);
    c->comps.completions = NULL;

    if (c->gui.compbox) {
        gtk_widget_destroy(c->gui.compbox);
        c->gui.compbox = NULL;
    }
    OVERWRITE_STRING(c->comps.prefix, NULL);
    c->comps.active = NULL;
    c->comps.count  = 0;

    /* remove completion flag from mode */
    c->state.mode &= ~VP_MODE_COMPLETE;
}

static GList* completion_init_completion(Client* c, GList* target, GList* source,
    Comp_Func func, const char* input, const char* prefix)
{
    char* command = NULL;
    char* data = NULL;
    gboolean match;
    char **token = NULL;

    /* remove counts before command and save it to print it later in inputbox */
    c->comps.count  = g_ascii_strtoll(input, &command, 10);
    OVERWRITE_STRING(c->comps.prefix, prefix);

    token = g_strsplit(command, " ", -1);

    for (GList* l = source; l; l = l->next) {
        data = l->data;
        match = FALSE;
        if (*command == 0) {
            match = TRUE;
        } else {
            for (int i = 0; token[i]; i++) {
                if (func(data, token[i])) {
                    match = TRUE;
                } else {
                    match = FALSE;
                    break;
                }
            }
        }
        if (match) {
            Completion* completion = completion_get_new(data, prefix);
            gtk_box_pack_start(GTK_BOX(c->gui.compbox), completion->event, TRUE, TRUE, 0);
            /* use prepend because that faster */
            target = g_list_prepend(target, completion);
        }
    }

    target = g_list_reverse(target);
    g_strfreev(token);

    return target;
}

static GList* completion_update(Client* c, GList* completion, GList* active, gboolean back)
{
    GList *old, *new;
    Completion *comp;

    int length = g_list_length(completion);
    int max    = core.config.max_completion_items;
    int items  = MAX(length, max);
    int r      = (max) % 2;
    int offset = max / 2 - 1 + r;

    old = active;
    int position = g_list_position(completion, active) + 1;
    if (back) {
        if (!(new = old->prev)) {
            new = g_list_last(completion);
        }
        if (position - 1 > offset && position < items - offset + r) {
            comp = g_list_nth(completion, position - offset - 2)->data;
            gtk_widget_show_all(comp->event);
            comp = g_list_nth(completion, position + offset - r)->data;
            gtk_widget_hide(comp->event);
        } else if (position == 1) {
            int i = 0;
            for (GList *l = g_list_first(completion); l && i < max; l = l->next, i++) {
                gtk_widget_hide(((Completion*)l->data)->event);
            }
            i = 0;
            for (GList *l = g_list_last(completion); l && i < max; l = l->prev, i++) {
                comp = l->data;
                gtk_widget_show_all(comp->event);
            }
        }
    } else {
        if (!(new = old->next)) {
            new = g_list_first(completion);
        }
        if (position > offset && position < items - offset - 1 + r) {
            comp = g_list_nth(completion, position - offset - 1)->data;
            gtk_widget_hide(comp->event);
            comp = g_list_nth(completion, position + offset + 1 - r)->data;
            gtk_widget_show_all(comp->event);
        } else if (position == items || position  == 1) {
            int i = 0;
            for (GList *l = g_list_last(completion); l && i < max; l = l->prev, i++) {
                gtk_widget_hide(((Completion*)l->data)->event);
            }
            i = 0;
            for (GList *l = g_list_first(completion); l && i < max; l = l->next, i++) {
                gtk_widget_show_all(((Completion*)l->data)->event);
            }
        }
    }

    VP_WIDGET_SET_STATE(((Completion*)old->data)->label, VP_GTK_STATE_NORMAL);
    VP_WIDGET_SET_STATE(((Completion*)old->data)->event, VP_GTK_STATE_NORMAL);
    VP_WIDGET_SET_STATE(((Completion*)new->data)->label, VP_GTK_STATE_ACTIVE);
    VP_WIDGET_SET_STATE(((Completion*)new->data)->event, VP_GTK_STATE_ACTIVE);

    active = new;
    completion_set_entry_text(c, active->data);
    return active;
}

/* allow to chenge the direction of display */
static void completion_show(Client* c, gboolean back)
{
    guint max = core.config.max_completion_items;
    int i = 0;
    if (back) {
        c->comps.active = g_list_last(c->comps.completions);
        for (GList *l = c->comps.active; l && i < max; l = l->prev, i++) {
            gtk_widget_show_all(((Completion*)l->data)->event);
        }
    } else {
        c->comps.active = g_list_first(c->comps.completions);
        for (GList *l = c->comps.active; l && i < max; l = l->next, i++) {
            gtk_widget_show_all(((Completion*)l->data)->event);
        }
    }
    if (c->comps.active != NULL) {
        Completion* active = (Completion*)c->comps.active->data;
        VP_WIDGET_SET_STATE(active->label, VP_GTK_STATE_ACTIVE);
        VP_WIDGET_SET_STATE(active->event, VP_GTK_STATE_ACTIVE);

        completion_set_entry_text(c, active);
        gtk_widget_show(c->gui.compbox);
    }
}

static void completion_set_entry_text(Client* c, Completion* completion)
{
    char* text = completion_get_text(c, completion);
    gtk_entry_set_text(GTK_ENTRY(c->gui.inputbox), text);
    gtk_editable_set_position(GTK_EDITABLE(c->gui.inputbox), -1);
    g_free(text);
}

/**
 * Retrieves the full new allocated entry text for given completion item.
 */
static char* completion_get_text(Client* c, Completion* completion)
{
    char* text = NULL;

    /* print the previous typed command count into inputbox too */
    if (c->comps.count) {
        text = g_strdup_printf(
            "%s%d%s", completion->prefix, c->comps.count, gtk_label_get_text(GTK_LABEL(completion->label))
        );
    } else {
        text = g_strdup_printf(
            "%s%s", completion->prefix, gtk_label_get_text(GTK_LABEL(completion->label))
        );
    }

    return text;
}

static Completion* completion_get_new(const char* label, const char* prefix)
{
    const int padding = 2;
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

    VP_WIDGET_SET_STATE(c->label, VP_GTK_STATE_NORMAL);
    VP_WIDGET_SET_STATE(c->event, VP_GTK_STATE_NORMAL);

    VP_WIDGET_OVERRIDE_COLOR(c->label, GTK_STATE_NORMAL, &core.style.comp_fg[VP_COMP_NORMAL]);
    VP_WIDGET_OVERRIDE_COLOR(c->label, GTK_STATE_ACTIVE, &core.style.comp_fg[VP_COMP_ACTIVE]);
    VP_WIDGET_OVERRIDE_BACKGROUND(c->event, GTK_STATE_NORMAL, &core.style.comp_bg[VP_COMP_NORMAL]);
    VP_WIDGET_OVERRIDE_BACKGROUND(c->event, GTK_STATE_ACTIVE, &core.style.comp_bg[VP_COMP_ACTIVE]);
    VP_WIDGET_OVERRIDE_FONT(c->label, core.style.comp_font);

    GtkWidget *alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), padding, padding, padding, padding);
    gtk_container_add(GTK_CONTAINER(alignment), hbox);
    gtk_container_add(GTK_CONTAINER(c->event), alignment);

    return c;
}

static void completion_free(Completion* completion)
{
    gtk_widget_destroy(completion->event);
    g_free(completion->prefix);
    g_free(completion);
}
