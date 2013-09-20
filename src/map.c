/**
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

#include <gdk/gdkkeysyms.h>
#include <gdk/gdkkeysyms-compat.h>
#include "config.h"
#include "main.h"
#include "map.h"
#include "mode.h"

extern VbCore vb;

typedef struct {
    char *in;         /* input keys */
    int  inlen;       /* length of the input keys */
    char *mapped;     /* mapped keys */
    int  mappedlen;   /* length of the mapped keys string */
    char mode;        /* mode for which the map is available */
} Map;

static struct {
    GSList *list;
    char   queue[MAP_QUEUE_SIZE];   /* queue holding typed keys */
    int    qlen;                    /* pointer to last char in queue */
    int    resolved;                /* number of resolved keys (no mapping required) */
    char   showbuf[12];             /* buffer used to show ambiguous keys to the user */
    int    slen;                    /* pointer to last char in showbuf */
    guint  timout_id;               /* source id of the timeout function */
} map;

static char *map_convert_keys(char *in, int inlen, int *len);
static char *map_convert_keylabel(char *in, int inlen, int *len);
static gboolean map_timeout(gpointer data);
static void showcmd(char *keys, int keylen, gboolean append);
static void free_map(Map *map);


void map_init(void)
{
    /* bind cursor keys to ^P and ^N but only in command mode else down would
     * trigger the queue pop */
    map_insert("<Up>", "\x10", 'c');
    map_insert("<Down>", "\x0e", 'c');
    map_insert("<Up>", "k", 'n');
    map_insert("<Down>", "j", 'n');
}

void map_cleanup(void)
{
    if (map.list) {
        g_slist_free_full(map.list, (GDestroyNotify)free_map);
    }
}

/**
 * Handle all key events, convert the key event into the internal used ASCII
 * representation and put this into the key queue to be mapped.
 */
gboolean map_keypress(GtkWidget *widget, GdkEventKey* event, gpointer data)
{
    unsigned int key = 0;
    static struct {
        guint keyval;
        int   ctrl;    /* -1 = indifferent, 1 = ctrl, 0 = no ctrl */
        char  *ch;
        int   chlen;
    } keys[] = {
        {GDK_Escape,       -1, "\x1b",         1}, /* ^[ */
        {GDK_Tab,          -1, "\x09",         1}, /* ^I */
        {GDK_ISO_Left_Tab, -1, "\x0f",         1}, /* ^O */
        {GDK_Return,       -1, "\x0a",         1},
        {GDK_KP_Enter,     -1, "\x0a",         1},
        {GDK_BackSpace,    -1, "\x08",         1}, /* ^H */
        {GDK_Up,           -1, CSI_STR "ku",   3},
        {GDK_Down,         -1, CSI_STR "kd",   3},
        {GDK_Left,         -1, CSI_STR "kl",   3},
        {GDK_Right,        -1, CSI_STR "kr",   3},
        /* function keys are prefixed by gEsc like in vim the CSI Control
         * Sequence Introducer \233 */
        /* TODO calculate this automatic */
        {GDK_KEY_F1,        1, CSI_STR "\x01", 2},
        {GDK_KEY_F2,        1, CSI_STR "\x02", 2},
        {GDK_KEY_F3,        1, CSI_STR "\x03", 2},
        {GDK_KEY_F4,        1, CSI_STR "\x04", 2},
        {GDK_KEY_F5,        1, CSI_STR "\x05", 2},
        {GDK_KEY_F6,        1, CSI_STR "\x06", 2},
        {GDK_KEY_F7,        1, CSI_STR "\x07", 2},
        {GDK_KEY_F8,        1, CSI_STR "\x08", 2},
        {GDK_KEY_F9,        1, CSI_STR "\x09", 2},
        {GDK_KEY_F10,       1, CSI_STR "\x0a", 2},
        {GDK_KEY_F11,       1, CSI_STR "\x0b", 2},
        {GDK_KEY_F12,       1, CSI_STR "\x0c", 2},
        {GDK_KEY_F1,        0, CSI_STR "\x0d", 2},
        {GDK_KEY_F2,        0, CSI_STR "\x0e", 2},
        {GDK_KEY_F3,        0, CSI_STR "\x0f", 2},
        {GDK_KEY_F4,        0, CSI_STR "\x10", 2},
        {GDK_KEY_F5,        0, CSI_STR "\x11", 2},
        {GDK_KEY_F6,        0, CSI_STR "\x12", 2},
        {GDK_KEY_F7,        0, CSI_STR "\x13", 2},
        {GDK_KEY_F8,        0, CSI_STR "\x14", 2},
        {GDK_KEY_F9,        0, CSI_STR "\x15", 2},
        {GDK_KEY_F10,       0, CSI_STR "\x16", 2},
        {GDK_KEY_F11,       0, CSI_STR "\x17", 2},
        {GDK_KEY_F12,       0, CSI_STR "\x18", 2},
    };

    int ctrl = (event->state & GDK_CONTROL_MASK) != 0;

    /* set initial value for the flag that should be changed in the modes key
     * handler functions */
    vb.state.processed_key = true;
    if (!ctrl && event->keyval > 0 && event->keyval < 0xff) {
        map_handle_keys((char*)(&event->keyval), 1);

        return vb.state.processed_key;
    }

    /* convert chars A-]a-z with ctrl flag <ctrl-a> or <ctrl-A> -> \001
     * and <ctrl-z> -> \032 like vi */
    if (event->keyval >= 0x41 && event->keyval <= 0x5d) {/* chars A-] */
        key = event->keyval - 0x40;
        map_handle_keys((char*)(&key), 1);

        return vb.state.processed_key;
    } else if (event->keyval >= 0x61 && event->keyval <= 0x7a) {/* chars a-z */
        key = event->keyval - 0x60;
        map_handle_keys((char*)(&key), 1);

        return vb.state.processed_key;
    }

    for (int i = 0; i < LENGTH(keys); i++) {
        if (keys[i].keyval == event->keyval
            && (keys[i].ctrl == ctrl ||keys[i].ctrl == -1)
        ) {
            map_handle_keys(keys[i].ch, keys[i].chlen);

            return vb.state.processed_key;
        }
    }

    /* mark all unknown key events as unhandled to not break some gtk features
     * like <S-Einf> to copy clipboard content into inputbox */
    return false;
}

/**
 * Added the given key sequence ot the key queue and precesses the mapping of
 * chars. The key sequence do not need to be NUL terminated.
 * Keylen of 0 signalized a key timeout.
 */
MapState map_handle_keys(const char *keys, int keylen)
{
    int ambiguous;
    Map *match = NULL;
    gboolean timeout = (keylen == 0); /* keylen 0 signalized timeout */

    /* don't set the timeout function if a timeout is handled */
    if (!timeout) {
        /* if a previous timeout function was set remove this to start the
         * timeout new */
        if (map.timout_id) {
            g_source_remove(map.timout_id);
        }
        map.timout_id = g_timeout_add(1000, (GSourceFunc)map_timeout, NULL);
    }

    /* copy the keys onto the end of queue */
    while (map.qlen < LENGTH(map.queue) && keylen > 0) {
        map.queue[map.qlen++] = *keys++;
        keylen--;
    }

    /* try to resolve keys against the map */
    while (true) {
        /* send any resolved key to the parser */
        static int csi = 0;
        while (map.resolved > 0) {
            /* pop the next char from queue */
            map.resolved--;
            map.qlen--;

            /* get first char of queue */
            char qk = map.queue[0];
            /* move all other queue entries one step to the left */
            for (int i = 0; i < map.qlen; i++) {
                map.queue[i] = map.queue[i + 1];
            }

            /* skip csi indicator and the next 2 chars - if the csi sequence
             * isn't part of a mapped command we let gtk handle the key - this
             * is required allo to move cursor in inputbox with <Left> and
             * <Right> keys */
            /* TODO make it simplier to skip the special keys here */
            if ((qk & 0xff) == CSI) {
                csi = 2;
                vb.state.processed_key = false;
                continue;
            }
            if (csi > 0) {
                csi--;
                vb.state.processed_key = false;
                showcmd(NULL, 0, false);
                continue;
            }

            /* remove the nomap flag */
            vb.mode->flags &= ~FLAG_NOMAP;

            /* send the key to the parser */
            if (RESULT_MORE != mode_handle_key((unsigned int)qk)) {
                showcmd(NULL, 0, false);
            }
        }

        /* if all keys where processed return MAP_DONE */
        if (map.qlen == 0) {
            map.resolved = 0;
            return match ? MAP_DONE : MAP_NOMATCH;
        }

        /* try to find matching maps */
        match     = NULL;
        ambiguous = 0;
        if (!(vb.mode->flags & FLAG_NOMAP)) {
            for (GSList *l = map.list; l != NULL; l = l->next) {
                Map *m = (Map*)l->data;
                /* ignore maps for other modes */
                if (m->mode != vb.mode->id) {
                    continue;
                }

                /* find ambiguous matches */
                if (!timeout && m->inlen > map.qlen && !strncmp(m->in, map.queue, map.qlen)) {
                    ambiguous++;
                    showcmd(map.queue, map.qlen, false);
                }
                /* complete match or better/longer match than previous found */
                if (m->inlen <= map.qlen
                    && !strncmp(m->in, map.queue, m->inlen)
                    && (!match || match->inlen < m->inlen)
                ) {
                    /* backup this found possible match */
                    match = m;
                }
            }
        }

        /* if there are ambiguous matches return MAP_KEY and flush queue
         * after a timeout if the user do not type more keys */
        if (ambiguous) {
            return MAP_AMBIGUOUS;
        }

        /* replace the matched chars from queue by the cooked string that
         * is the result of the mapping */
        if (match) {
            int i, j;
            if (match->inlen < match->mappedlen) {
                /* make some space within the queue */
                for (i = map.qlen + match->mappedlen - match->inlen, j = map.qlen; j > match->inlen; ) {
                    map.queue[--i] = map.queue[--j];
                }
            } else if (match->inlen > match->mappedlen) {
                /* delete some keys */
                for (i = match->mappedlen, j = match->inlen; i < map.qlen; ) {
                    map.queue[i++] = map.queue[j++];
                }
            }

            /* copy the mapped string into the queue */
            strncpy(map.queue, match->mapped, match->mappedlen);
            map.qlen += match->mappedlen - match->inlen;
            if (match->inlen <= match->mappedlen) {
                map.resolved = match->inlen;
            } else {
                map.resolved = match->mappedlen;
            }
        } else {
            /* first char is not mapped but resolved */
            map.resolved = 1;
            showcmd(map.queue, map.resolved, true);
        }
    }

    /* should never be reached */
    return MAP_DONE;
}

void map_insert(char *in, char *mapped, char mode)
{
    int inlen, mappedlen;
    char *lhs = map_convert_keys(in, strlen(in), &inlen);
    char *rhs = map_convert_keys(mapped, strlen(mapped), &mappedlen);

    /* TODO replace keysymbols in 'in' and 'mapped' string */
    Map *new = g_new(Map, 1);
    new->in        = lhs;
    new->inlen     = inlen;
    new->mapped    = rhs;
    new->mappedlen = mappedlen;
    new->mode      = mode;

    map.list = g_slist_prepend(map.list, new);
}

gboolean map_delete(char *in, char mode)
{
    int len;
    char *lhs = map_convert_keys(in, strlen(in), &len);

    for (GSList *l = map.list; l != NULL; l = l->next) {
        Map *m = (Map*)l->data;

        /* remove only if the map's lhs matches the given key sequence */
        if (m->mode == mode && m->inlen == len && !strcmp(m->in, lhs)) {
            /* remove the found list item */
            map.list = g_slist_delete_link(map.list, l);

            return true;
        }
    }

    return false;
}

/**
 * Converts a keysequence into a internal raw keysequence.
 * Returned keyseqence must be freed if not used anymore.
 */
static char *map_convert_keys(char *in, int inlen, int *len)
{
    int symlen, rawlen;
    char *p, *dest, *raw;
    char ch[1];
    GString *str = g_string_new("");

    *len = 0;
    for (p = in; p < &in[inlen]; p++) {
        /* if it starts not with < we can add it literally */
        if (*p != '<') {
            g_string_append_len(str, p, 1);
            *len += 1;
            continue;
        }

        /* search matching > of symbolic name */
        symlen = 1;
        do {
            if (&p[symlen] == &in[inlen]
                || p[symlen] == '<'
                || p[symlen] == ' '
            ) {
                break;
            }
        } while (p[symlen++] != '>');

        raw    = NULL;
        rawlen = 0;
        /* check if we found a real keylabel */
        if (p[symlen - 1] == '>') {
            if (symlen == 5 && p[2] == '-') {
                /* is it a <C-X> */
                if (p[1] == 'C') {
                    /* TODO add macro to check if the char is a upper or lower
                     * with these ranges */
                    if (p[3] >= 0x41 && p[3] <= 0x5d) {
                        ch[0]  = p[3] - 0x40;
                        raw    = ch;
                        rawlen = 1;
                    } else if (p[3] >= 0x61 && p[3] <= 0x7a) {
                        ch[0]  = p[3] - 0x60;
                        raw    = ch;
                        rawlen = 1;
                    }
                }
            }

            if (!raw) {
                raw = map_convert_keylabel(p, symlen, &rawlen);
            }
        }

        /* we found no known keylabel - so use the chars literally */
        if (!rawlen) {
            rawlen = symlen;
            raw    = p;
        }

        /* write the converted keylabel into the buffer */
        g_string_append_len(str, raw, rawlen);

        /* move p after the keylabel */
        p += symlen - 1;

        *len += rawlen;
    }
    dest = str->str;

    /* don't free the character data of the GString */
    g_string_free(str, false);

    return dest;
}

/**
 * Translate given key string into a internal representation <cr> -> \n.
 * The len of the translated key sequence is put into given *len pointer.
 */
static char *map_convert_keylabel(char *in, int inlen, int *len)
{
    static struct {
        char *label;
        int  len;
        char *ch;
        int  chlen;
    } keys[] = {
        {"<CR>",    4, "\n",           1},
        {"<Tab>",   5, "\t",           1},
        {"<Esc>",   5, "\x1b",         1},
        {"<Up>",    4, CSI_STR "ku",   3},
        {"<Down>",  6, CSI_STR "kd",   3},
        {"<Left>",  6, CSI_STR "kl",   3},
        {"<Right>", 7, CSI_STR "kr",   3},
        /* convert function keys to gEsc+num */
        /* TODO allow to calculate the ch programmatic instead of mapping the
         * function keys here */
        {"<C-F1>",  6, CSI_STR "\x01", 2},
        {"<C-F2>",  6, CSI_STR "\x02", 2},
        {"<C-F3>",  6, CSI_STR "\x03", 2},
        {"<C-F4>",  6, CSI_STR "\x04", 2},
        {"<C-F5>",  6, CSI_STR "\x05", 2},
        {"<C-F6>",  6, CSI_STR "\x06", 2},
        {"<C-F7>",  6, CSI_STR "\x07", 2},
        {"<C-F8>",  6, CSI_STR "\x08", 2},
        {"<C-F9>",  6, CSI_STR "\x09", 2},
        {"<C-F10>", 7, CSI_STR "\x0a", 2},
        {"<C-F11>", 7, CSI_STR "\x0b", 2},
        {"<C-F12>", 7, CSI_STR "\x0c", 2},
        {"<F1>",    4, CSI_STR "\x0d", 2},
        {"<F2>",    4, CSI_STR "\x0e", 2},
        {"<F3>",    4, CSI_STR "\x0f", 2},
        {"<F4>",    4, CSI_STR "\x10", 2},
        {"<F5>",    4, CSI_STR "\x11", 2},
        {"<F6>",    4, CSI_STR "\x12", 2},
        {"<F7>",    4, CSI_STR "\x13", 2},
        {"<F8>",    4, CSI_STR "\x14", 2},
        {"<F9>",    4, CSI_STR "\x15", 2},
        {"<F10>",   5, CSI_STR "\x16", 2},
        {"<F11>",   5, CSI_STR "\x17", 2},
        {"<F12>",   5, CSI_STR "\x18", 2},
    };

    for (int i = 0; i < LENGTH(keys); i++) {
        if (inlen == keys[i].len && !strncmp(keys[i].label, in, inlen)) {
            *len = keys[i].chlen;
            return keys[i].ch;
        }
    }
    *len = 0;

    return NULL;
}

/**
 * Timeout function to signalize a key timeout to the map.
 */
static gboolean map_timeout(gpointer data)
{
    /* signalize the timeout to the key handler */
    map_handle_keys("", 0);

    /* call only once */
    return false;
}

/**
 * Add given keys to the show command queue to show them to the user.
 * If the keylen of 0 is given, the show command queue is cleared.
 */
static void showcmd(char *keys, int keylen, gboolean append)
{
    /* make sure we keep in buffer range also for ^X chars */
    int max = LENGTH(map.showbuf) - 2;

    if (!append) {
        map.slen = 0;
    }

    /* truncate the buffer */
    if (!keylen) {
        map.showbuf[(map.slen = 0)] = '\0';
    } else {
        /* TODO if not all keys would fit into the buffer use the last significat
         * chars instead */
        while (keylen-- && map.slen < max) {
            char key = *keys++;
            if (IS_CTRL(key)) {
                map.showbuf[map.slen++] = '^';
                map.showbuf[map.slen++] = CTRL(key);
            } else if ((key & 0xff) == CSI) {
                map.showbuf[map.slen++] = '@';
            } else {
                map.showbuf[map.slen++] = key;
            }
        }
        map.showbuf[map.slen] = '\0';
    }

    /* show the typed keys */
    gtk_label_set_text(GTK_LABEL(vb.gui.statusbar.cmd), map.showbuf);
}

static void free_map(Map *map)
{
    g_free(map->in);
    g_free(map->mapped);
    g_free(map);
}
