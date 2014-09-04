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
#ifdef FEATURE_AUTOCMD
#include "autocmd.h"
#include "ascii.h"
#include "ex.h"
#include "util.h"

typedef struct {
    guint bits;
    char *excmd;
    char *pattern;
} AutoCmd;

typedef struct {
    char    *name;
    GSList  *cmds;
} AuGroup;

static struct {
    const char *name;
    guint      bits;
} events[] = {
    {"*",                   0x001f},
    {"PageLoadProvisional", 0x0001},
    {"PageLoadCommited",    0x0002},
    {"PageLoadFirstLayout", 0x0004},
    {"PageLoadFinished",    0x0008},
    {"PageLoadFailed",      0x0010},
};

extern VbCore vb;

static AuGroup *curgroup = NULL;
static GSList  *groups = NULL;

static guint get_event_bits(const char *name);
static GSList *get_group(const char *name);
static char *get_next_word(char **line);
static gboolean wildmatch(char *patterns, const char *uri);
static AuGroup *new_group(const char *name);
static void free_group(AuGroup *group);
static AutoCmd *new_autocmd(const char *excmd, const char *pattern);
static void free_autocmd(AutoCmd *cmd);


void autocmd_init(void)
{
    curgroup = new_group("end");
    groups   = g_slist_prepend(groups, curgroup);
}

void autocmd_cleanup(void)
{
    if (groups) {
        g_slist_free_full(groups, (GDestroyNotify)free_group);
    }
}

/**
 * Handle the :augroup {group} ex command.
 */
gboolean autocmd_augroup(char *name, gboolean delete)
{
    GSList *item;

    if (!*name) {
        return false;
    }

    /* check for group "end" that marks the default group */
    if (!strcmp(name, "end")) {
        curgroup = (AuGroup*)groups->data;
        return true;
    }

    item = get_group(name);

    /* check if the group is going to be removed */
    if (delete) {
        /* group does not exist - so do nothing */
        if (!item) {
            return true;
        }
        if (curgroup == (AuGroup*)item->data) {
            /* if the group to delete is the current - switch the the default
             * group after removing it */
            curgroup = (AuGroup*)groups->data;
        }

        /* now remove the group */
        free_group((AuGroup*)item->data);
        groups = g_slist_delete_link(groups, item);

        return true;
    }

    /* check if the group does already exists */
    if (item) {
        /* if the group is found in the known groups use it as current */
        curgroup = (AuGroup*)item->data;

        return true;
    }

    /* create a new group and use it as current */
    curgroup = new_group(name);

    /* append it to known groups */
    groups = g_slist_prepend(groups, curgroup);

    return true;
}

/**
 * Add or delete auto commands.
 *
 * :au[tocmd]! [group] {event} {pat} [nested] {cmd}
 * group and nested flag are not supported at the moment.
 */
gboolean autocmd_add(char *name, gboolean delete)
{
    guint bits;
    char *parse, *word, *pattern, *excmd;
    GSList *item;
    AuGroup *grp;

    parse = name;

    /* parse group name if it's there */
    word = get_next_word(&parse);
    if (word) {
        /* check if the word is a known group name */
        item = get_group(word);
        if (item) {
            grp = (AuGroup*)item->data;

            /* group is found - get the next word */
            word = get_next_word(&parse);
        } else {
            /* no group found - use the current one */
            grp = curgroup;
        }
    }

    /* parse event name - if none was matched run it for all events */
    if (word) {
        bits = get_event_bits(word);
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

        /* check if the group does already exists */
        for (lc = grp->cmds; lc; lc = lc->next) {
            cmd = (AutoCmd*)lc->data;
            /* if not bits match - skip the command */
            if (!(cmd->bits & bits)) {
                continue;
            }
            /* skip if pattern does not match - we check the pattern against
             * another pattern */
            if (!wildmatch(pattern, cmd->pattern)) {
                continue;
            }
            /* remove the bits from the item */
            cmd->bits &= ~bits;

            /* if the command has no matching events - remove it */
            grp->cmds = g_slist_delete_link(grp->cmds, lc);
            free_autocmd(cmd);
        }

        return true;
    }

    /* add the new autocmd */
    if (excmd) {
        AutoCmd *cmd = new_autocmd(excmd, pattern);
        cmd->bits    = bits;

        /* add the new autocmd to the group */
        grp->cmds = g_slist_append(grp->cmds, cmd);
    }

    return true;
}

/**
 * Run named auto command.
 */
gboolean autocmd_run(const char *group, AuEvent event, const char *uri)
{
    GSList  *lg, *lc;
    AuGroup *grp;
    AutoCmd *cmd;
    guint bits = events[event].bits;

    /* don't record commands in history runed by autocmd */
    vb.state.enable_history = false;

    /* loop over the groups and find matching commands */
    for (lg = groups; lg; lg = lg->next) {
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
            if (uri && !wildmatch(cmd->pattern, uri)) {
                continue;
            }
            /* run the command */
            /* TODO shoult the result be tested for RESULT_COMPLETE? */
            ex_run_string(cmd->excmd);
        }
    }
    vb.state.enable_history = true;

    return true;
}

/**
 * Get the augroup by it's name.
 */
static GSList *get_group(const char *name)
{
    GSList  *lg;
    AuGroup *grp;

    for (lg = groups; lg; lg = lg->next) {
        grp = lg->data;
        if (!strcmp(grp->name, name)) {
            return lg;
        }
    }

    return NULL;
}

static guint get_event_bits(const char *name)
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
            vb_echo(VB_MSG_ERROR, true, "Bad autocmd event name: %s", name);
            return 0;
        }
    }

    return result;
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

/**
 * Check if given uri matches one of the patterns given as comma separated
 * string.
 */
static gboolean wildmatch(char *patterns, const char *uri)
{
    char *cur, *start;
    gboolean matched;

    for (cur = start = patterns; *cur; cur++) {
        /* iterate through the patterns until the next ',' */
        if (*cur == ',') {
            *cur    = '\0';
            matched = util_wildmatch(start, uri);
            *cur    = ',';

            /* return if the first match is found */
            if (matched) {
                return true;
            }

            /* set the start right after the ',' */
            start = cur + 1;
        }
    }

    /* still no match found - check the last pattern */
    return util_wildmatch(start, uri);
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
