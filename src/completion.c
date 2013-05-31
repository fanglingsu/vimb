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

extern VbCore vb;

typedef struct {
    GtkWidget *label;
    GtkWidget *event;
    char      *prefix;
} Completion;

static struct {
    GList *completions;
    GList *active;
    int   count;
    char  *prefix;
} comps;

static GList *init_completion(GList *target, GList *source, const char *prefix);
static GList *update(GList *completion, GList *active, gboolean back);
static void show(gboolean back);
static void set_entry_text(Completion *completion);
static char *get_text(Completion *completion);
static Completion *get_new(const char *label, const char *prefix);
static void free_completion(Completion *completion);

gboolean completion_complete(gboolean back)
{
    VbInputType type;
    const char *input, *prefix, *suffix;
    GList *source = NULL;

    input = GET_TEXT();
    type  = vb_get_input_parts(input, &prefix, &suffix);

    if (comps.completions && comps.active
        && (vb.state.mode & VB_MODE_COMPLETE)
    ) {
        char *text = get_text((Completion*)comps.active->data);
        if (!strcmp(input, text)) {
            /* updatecompletions */
            comps.active = update(comps.completions, comps.active, back);
            g_free(text);
            return true;
        } else {
            g_free(text);
            /* if current input isn't the content of the completion item */
            completion_clean();
        }
    }

    /* don't disturb other command sub modes - complate only if no sub mode
     * is set before */
    if (vb.state.mode != VB_MODE_COMMAND) {
        return false;
    }

    /* create new completion */
#ifdef HAS_GTK3
    vb.gui.compbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_set_homogeneous(GTK_BOX(vb.gui.compbox), true);
#else
    vb.gui.compbox = gtk_vbox_new(true, 0);
#endif
    gtk_box_pack_start(GTK_BOX(vb.gui.box), vb.gui.compbox, false, false, 0);

    if (type == VB_INPUT_SET) {
        source = g_list_sort(setting_get_by_prefix(suffix), (GCompareFunc)g_strcmp0);
        comps.completions = init_completion(comps.completions, source, prefix);
        g_list_free(source);
    } else if (type == VB_INPUT_OPEN || type == VB_INPUT_TABOPEN) {
        /* if search string begins with TAG_INDICATOR lookup the bookmarks */
        if (suffix && *suffix == TAG_INDICATOR) {
            source = bookmark_get_by_tags(suffix + 1);
            comps.completions = init_completion(comps.completions, source, prefix);
        } else {
            source = history_get_by_tags(HISTORY_URL, suffix);
            comps.completions = init_completion(comps.completions, source, prefix);
        }
        g_list_free_full(source, (GDestroyNotify)g_free);
    } else if (type == VB_INPUT_COMMAND) {
        char *command = NULL;
        /* remove counts before command and save it to print it later in inputbox */
        comps.count = g_ascii_strtoll(suffix, &command, 10);

        source = g_list_sort(command_get_by_prefix(suffix), (GCompareFunc)g_strcmp0);
        comps.completions = init_completion(comps.completions, source, prefix);
        g_list_free(source);
    } else if (type == VB_INPUT_SEARCH_FORWARD || type == VB_INPUT_SEARCH_BACKWARD) {
        source = g_list_sort(history_get_by_tags(HISTORY_SEARCH, suffix), (GCompareFunc)g_strcmp0);
        comps.completions = init_completion(comps.completions, source, prefix);
        g_list_free_full(source, (GDestroyNotify)g_free);
    }

    if (!comps.completions) {
        return false;
    }

    vb_set_mode(VB_MODE_COMMAND | VB_MODE_COMPLETE, false);

    show(back);

    return true;
}

void completion_clean()
{
    g_list_free_full(comps.completions, (GDestroyNotify)free_completion);
    comps.completions = NULL;

    if (vb.gui.compbox) {
        gtk_widget_destroy(vb.gui.compbox);
        vb.gui.compbox = NULL;
    }
    OVERWRITE_STRING(comps.prefix, NULL);
    comps.active = NULL;
    comps.count  = 0;

    /* remove completion flag from mode */
    vb.state.mode &= ~VB_MODE_COMPLETE;
}

static GList *init_completion(GList *target, GList *source, const char *prefix)
{
    OVERWRITE_STRING(comps.prefix, prefix);

    for (GList *l = source; l; l = l->next) {
        Completion *c = get_new(l->data, prefix);
        target        = g_list_prepend(target, c);
        gtk_box_pack_start(GTK_BOX(vb.gui.compbox), c->event, true, true, 0);
    }

    target = g_list_reverse(target);

    return target;
}

static GList *update(GList *completion, GList *active, gboolean back)
{
    GList *old, *new;
    Completion *comp;

    int length = g_list_length(completion);
    int max    = vb.config.max_completion_items;
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

    VB_WIDGET_SET_STATE(((Completion*)old->data)->label, VB_GTK_STATE_NORMAL);
    VB_WIDGET_SET_STATE(((Completion*)old->data)->event, VB_GTK_STATE_NORMAL);
    VB_WIDGET_SET_STATE(((Completion*)new->data)->label, VB_GTK_STATE_ACTIVE);
    VB_WIDGET_SET_STATE(((Completion*)new->data)->event, VB_GTK_STATE_ACTIVE);

    active = new;
    set_entry_text(active->data);
    return active;
}

/* allow to chenge the direction of display */
static void show(gboolean back)
{
    guint max = vb.config.max_completion_items;
    int i = 0;
    if (back) {
        comps.active = g_list_last(comps.completions);
        for (GList *l = comps.active; l && i < max; l = l->prev, i++) {
            gtk_widget_show_all(((Completion*)l->data)->event);
        }
    } else {
        comps.active = g_list_first(comps.completions);
        for (GList *l = comps.active; l && i < max; l = l->next, i++) {
            gtk_widget_show_all(((Completion*)l->data)->event);
        }
    }
    if (comps.active != NULL) {
        Completion *active = (Completion*)comps.active->data;
        VB_WIDGET_SET_STATE(active->label, VB_GTK_STATE_ACTIVE);
        VB_WIDGET_SET_STATE(active->event, VB_GTK_STATE_ACTIVE);

        set_entry_text(active);
        gtk_widget_show(vb.gui.compbox);
    }
}

static void set_entry_text(Completion *completion)
{
    char *text = get_text(completion);
    gtk_entry_set_text(GTK_ENTRY(vb.gui.inputbox), text);
    gtk_editable_set_position(GTK_EDITABLE(vb.gui.inputbox), -1);
    g_free(text);
}

/**
 * Retrieves the full new allocated entry text for given completion item.
 */
static char *get_text(Completion *completion)
{
    char *text = NULL;

    /* print the previous typed command count into inputbox too */
    if (comps.count) {
        text = g_strdup_printf(
            "%s%d%s", completion->prefix, comps.count, gtk_label_get_text(GTK_LABEL(completion->label))
        );
    } else {
        text = g_strdup_printf(
            "%s%s", completion->prefix, gtk_label_get_text(GTK_LABEL(completion->label))
        );
    }

    return text;
}

static Completion *get_new(const char *label, const char *prefix)
{
    const int padding = 2;
    Completion *c = g_new0(Completion, 1);

    c->label  = gtk_label_new(label);
    c->event  = gtk_event_box_new();
    c->prefix = g_strdup(prefix);

#ifdef HAS_GTK3
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_set_homogeneous(GTK_BOX(hbox), true);
#else
    GtkWidget *hbox = gtk_hbox_new(true, 0);
#endif

    gtk_box_pack_start(GTK_BOX(hbox), c->label, true, true, 5);
    gtk_label_set_ellipsize(GTK_LABEL(c->label), PANGO_ELLIPSIZE_MIDDLE);
    gtk_misc_set_alignment(GTK_MISC(c->label), 0.0, 0.5);

    VB_WIDGET_SET_STATE(c->label, VB_GTK_STATE_NORMAL);
    VB_WIDGET_SET_STATE(c->event, VB_GTK_STATE_NORMAL);

    VB_WIDGET_OVERRIDE_COLOR(c->label, GTK_STATE_NORMAL, &vb.style.comp_fg[VB_COMP_NORMAL]);
    VB_WIDGET_OVERRIDE_COLOR(c->label, GTK_STATE_ACTIVE, &vb.style.comp_fg[VB_COMP_ACTIVE]);
    VB_WIDGET_OVERRIDE_BACKGROUND(c->event, GTK_STATE_NORMAL, &vb.style.comp_bg[VB_COMP_NORMAL]);
    VB_WIDGET_OVERRIDE_BACKGROUND(c->event, GTK_STATE_ACTIVE, &vb.style.comp_bg[VB_COMP_ACTIVE]);
    VB_WIDGET_OVERRIDE_FONT(c->label, vb.style.comp_font);

    GtkWidget *alignment = gtk_alignment_new(0.5, 0.5, 1, 1);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), padding, padding, padding, padding);
    gtk_container_add(GTK_CONTAINER(alignment), hbox);
    gtk_container_add(GTK_CONTAINER(c->event), alignment);

    return c;
}

static void free_completion(Completion *completion)
{
    gtk_widget_destroy(completion->event);
    g_free(completion->prefix);
    g_free(completion);
}
