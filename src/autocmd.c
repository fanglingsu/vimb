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

#include <string.h>

#include "config.h"
#ifdef FEATURE_AUTOCMD
#include "autocmd.h"
#include "ascii.h"
#include "ex.h"
#include "util.h"
#include "completion.h"

typedef struct {
    guint bits;     /* the bits identify the events the command applies to */
    char *excmd;    /* ex command string to be run on matches event */
    char *pattern;  /* list of patterns the uri is matched agains */
} AutoCmd;

struct AuGroup {
    char    *name;
    GSList  *cmds;
};

typedef struct AuGroup AuGroup;

static struct {
    const char *name;
    guint      bits;
} events[] = {
    {"*",                0x01ff},
    {"LoadStarting",     0x0001},
    {"LoadStarted",      0x0002},
    {"LoadCommitted",    0x0004},
    /*{"LoadFirstLayout",  0x0008},*/
    {"LoadFinished",     0x0010},
    /*{"LoadFailed",       0x0020},*/
    {"DownloadStarted",  0x0040},
    {"DownloadFinished", 0x0080},
    {"DownloadFailed",   0x0100},
};

static GSList *get_group(Client *c, const char *name);
static guint get_event_bits(Client *c, const char *name);
static void rebuild_used_bits(Client *c);
static char *get_next_word(char **line);
static AuGroup *new_group(const char *name);
static void free_group(AuGroup *group);
static AutoCmd *new_autocmd(const char *excmd, const char *pattern);
static void free_autocmd(AutoCmd *cmd);


void autocmd_init(Client *c)
{
    c->autocmd.curgroup = new_group("end");
    c->autocmd.groups   = g_slist_prepend(c->autocmd.groups, c->autocmd.curgroup);
    c->autocmd.usedbits = 0;
}

void autocmd_cleanup(Client *c)
{
    if (c->autocmd.groups) {
        g_slist_free_full(c->autocmd.groups, (GDestroyNotify)free_group);
    }
}

/**
 * Handle the :augroup {group} ex command.
 */
gboolean autocmd_augroup(Client *c, char *name, gboolean delete)
{
    GSList *item;

    if (!*name) {
        return false;
    }

    /* check for group "end" that marks the default group */
    if (!strcmp(name, "end")) {
        c->autocmd.curgroup = (AuGroup*)c->autocmd.groups->data;
        return true;
    }

    item = get_group(c, name);

    /* check if the group is going to be removed */
    if (delete) {
        /* group does not exist - so do nothing */
        if (!item) {
            return true;
        }
        if (c->autocmd.curgroup == (AuGroup*)item->data) {
            /* if the group to delete is the current - switch the the default
             * group after removing it */
            c->autocmd.curgroup = (AuGroup*)c->autocmd.groups->data;
        }

        /* now remove the group */
        free_group((AuGroup*)item->data);
        c->autocmd.groups = g_slist_delete_link(c->autocmd.groups, item);

        /* there where autocmds remove - so recreate the usedbits */
        rebuild_used_bits(c);

        return true;
    }

    /* check if the group does already exists */
    if (item) {
        /* if the group is found in the known groups use it as current */
        c->autocmd.curgroup = (AuGroup*)item->data;

        return true;
    }

    /* create a new group and use it as current */
    c->autocmd.curgroup = new_group(name);

    /* append it to known groups */
    c->autocmd.groups = g_slist_prepend(c->autocmd.groups, c->autocmd.curgroup);

    return true;
}

/**
 * Add or delete auto commands.
 *
 * :au[tocmd]! [group] {event} {pat} [nested] {cmd}
 * group and nested flag are not supported at the moment.
 */
gboolean autocmd_add(Client *c, char *name, gboolean delete)
{
    guint bits;
    char *parse, *word, *pattern, *excmd;
    GSList *item;
    AuGroup *grp = NULL;

    parse = name;

    /* parse group name if it's there */
    word = get_next_word(&parse);
    if (word) {
        /* check if the word is a known group name */
        item = get_group(c, word);
        if (item) {
            grp = (AuGroup*)item->data;

            /* group is found - get the next word */
            word = get_next_word(&parse);
        }
    }
    if (!grp) {
        /* no group found - use the current one */
        grp = c->autocmd.curgroup;
    }

    /* parse event name - if none was matched run it for all events */
    if (word) {
        bits = get_event_bits(c, word);
        if (!bits) {
            return false;
        }
        word = get_next_word(&parse);
    } else {
        bits = events[AU_ALL].bits;
    }

    /* last word is the pattern - if not found use '*' */
    pattern = word ? word : "*";

    /* the rest of the line becoes the ex command to run */
    if (parse && !*parse) {
        parse = NULL;
    }
    excmd = parse;

    /* delete the autocmd if bang was given */
    if (delete) {
        GSList *lc;
        AutoCmd *cmd;
        gboolean removed = false;

        /* check if the group does already exists */
        for (lc = grp->cmds; lc; lc = lc->next) {
            cmd = (AutoCmd*)lc->data;
            /* if not bits match - skip the command */
            if (!(cmd->bits & bits)) {
                continue;
            }
            /* skip if pattern does not match - we check the pattern against
             * another pattern */
            if (!util_wildmatch(pattern, cmd->pattern)) {
                continue;
            }
            /* remove the bits from the item */
            cmd->bits &= ~bits;

            /* if the command has no matching events - remove it */
            grp->cmds = g_slist_delete_link(grp->cmds, lc);
            free_autocmd(cmd);

            removed = true;
        }

        /* if ther was at least one command removed - rebuilt the used bits */
        if (removed) {
            rebuild_used_bits(c);
        }

        return true;
    }

    /* add the new autocmd */
    if (excmd && grp) {
        AutoCmd *cmd = new_autocmd(excmd, pattern);
        cmd->bits    = bits;

        /* add the new autocmd to the group */
        grp->cmds = g_slist_append(grp->cmds, cmd);

        /* merge the autocmd bits into the used bits */
        c->autocmd.usedbits |= cmd->bits;
    }

    return true;
}

/**
 * Run named auto command.
 */
gboolean autocmd_run(Client *c, AuEvent event, const char *uri, const char *group)
{
    GSList  *lg, *lc;
    AuGroup *grp;
    AutoCmd *cmd;
    guint bits = events[event].bits;

    /* if there is no autocmd for this event - skip here */
    if (!(c->autocmd.usedbits & bits)) {
        return true;
    }

    /* loop over the groups and find matching commands */
    for (lg = c->autocmd.groups; lg; lg = lg->next) {
        grp = lg->data;
        /* if a group was given - skip all none matching groupes */
        if (group && strcmp(group, grp->name)) {
            continue;
        }
        /* test each command in group */
        for (lc = grp->cmds; lc; lc = lc->next) {
            cmd = lc->data;
            /* skip if this dos not match the event bits */
            if (!(bits & cmd->bits)) {
                continue;
            }
            /* check pattern only if uri was given */
            /* skip if pattern does not match */
            if (uri && !util_wildmatch(cmd->pattern, uri)) {
                continue;
            }
            /* run the command */
            /* TODO shoult the result be tested for RESULT_COMPLETE? */
            /* run command and make sure it's not writte to command history */
            ex_run_string(c, cmd->excmd, false);
        }
    }

    return true;
}

gboolean autocmd_fill_group_completion(Client *c, GtkListStore *store, const char *input)
{
    GSList *lg;
    gboolean found = false;
    GtkTreeIter iter;

    if (!input || !*input) {
        for (lg = c->autocmd.groups; lg; lg = lg->next) {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, COMPLETION_STORE_FIRST, ((AuGroup*)lg->data)->name, -1);
            found = true;
        }
    } else {
        for (lg = c->autocmd.groups; lg; lg = lg->next) {
            char *value = ((AuGroup*)lg->data)->name;
            if (g_str_has_prefix(value, input)) {
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter, COMPLETION_STORE_FIRST, value, -1);
                found = true;
            }
        }
    }

    return found;
}

gboolean autocmd_fill_event_completion(Client *c, GtkListStore *store, const char *input)
{
    int i;
    const char *value;
    gboolean found = false;
    GtkTreeIter iter;

    if (!input || !*input) {
        for (i = 0; i < LENGTH(events); i++) {
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, COMPLETION_STORE_FIRST, events[i].name, -1);
            found = true;
        }
    } else {
        for (i = 0; i < LENGTH(events); i++) {
            value = events[i].name;
            if (g_str_has_prefix(value, input)) {
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter, COMPLETION_STORE_FIRST, value, -1);
                found = true;
            }
        }
    }

    return found;
}

/**
 * Get the augroup by it's name.
 */
static GSList *get_group(Client *c, const char *name)
{
    GSList  *lg;
    AuGroup *grp;

    for (lg = c->autocmd.groups; lg; lg = lg->next) {
        grp = lg->data;
        if (!strcmp(grp->name, name)) {
            return lg;
        }
    }

    return NULL;
}

static guint get_event_bits(Client *c, const char *name)
{
    int result = 0;

    while (*name) {
        int i, len;
        for (i = 0; i < LENGTH(events); i++) {
            /* we count the chars that given name has in common with the
             * current processed event */
            len = 0;
            while (name[len] && events[i].name[len] && name[len] == events[i].name[len]) {
                len++;
            }

            /* check if the matching chars built a full word match */
            if (events[i].name[len] == '\0'
                && (name[len] == '\0' || name[len] == ',')
            ) {
                /* add the bits to the result */
                result |= events[i].bits;

                /* move pointer after the match and skip possible ',' */
                name += len;
                if (*name == ',') {
                    name++;
                }
                break;
            }
        }

        /* check if the end was reached without a match */
        if (i >= LENGTH(events)) {
            /* is this the right place to write the error */
            vb_echo(c, MSG_ERROR, TRUE, "Bad autocmd event name: %s", name);
            return 0;
        }
    }

    return result;
}

/**
 * Rebuild the usedbits from scratch.
 * Save all used autocmd event bits in the bitmap.
 */
static void rebuild_used_bits(Client *c)
{
    GSList *lc, *lg;
    AuGroup *grp;

    /* clear the current set bits */
    c->autocmd.usedbits = 0;
    /* loop over the groups */
    for (lg = c->autocmd.groups; lg; lg = lg->next) {
        grp = (AuGroup*)lg->data;

        /* merge the used event bints into the bitmap */
        for (lc = grp->cmds; lc; lc = lc->next) {
            c->autocmd.usedbits |= ((AutoCmd*)lc->data)->bits;
        }
    }
}

/**
 * Get the next word from given line.
 * Given line pointer is set past the word and and a 0-byte is added there.
 */
static char *get_next_word(char **line)
{
    char *word;

    if (!*line || !**line) {
        return NULL;
    }

    /* remember where the word starts */
    word = *line;

    /* move pointer to the end of the word or of the line */
    while (**line && !VB_IS_SPACE(**line)) {
        (*line)++;
    }

    /* end the word */
    if (**line) {
        *(*line)++ = '\0';
    }

    /* skip trailing whitespace */
    while (VB_IS_SPACE(**line)) {
        (*line)++;
    }

    return word;
}

static AuGroup *new_group(const char *name)
{
    AuGroup *new = g_slice_new(AuGroup);
    new->name    = g_strdup(name);
    new->cmds    = NULL;

    return new;
}

static void free_group(AuGroup *group)
{
    g_free(group->name);
    if (group->cmds) {
        g_slist_free_full(group->cmds, (GDestroyNotify)free_autocmd);
    }
    g_slice_free(AuGroup, group);
}

static AutoCmd *new_autocmd(const char *excmd, const char *pattern)
{
    AutoCmd *new = g_slice_new(AutoCmd);
    new->excmd   = g_strdup(excmd);
    new->pattern = g_strdup(pattern);
    return new;
}

static void free_autocmd(AutoCmd *cmd)
{
    g_free(cmd->excmd);
    g_free(cmd->pattern);
    g_slice_free(AutoCmd, cmd);
}

#endif
