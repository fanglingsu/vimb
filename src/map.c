/**
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

#include <gdk/gdkkeysyms.h>
#include <gdk/gdkkeysyms-compat.h>
#include "config.h"
#include "main.h"
#include "map.h"
#include "normal.h"
#include "ascii.h"
#include "mode.h"

/* convert the lower 4 bits of byte n to its hex character */
#define NR2HEX(n)   (n & 0xf) <= 9 ? (n & 0xf) + '0' : (c & 0xf) - 10 + 'a'

typedef struct {
    char     *in;       /* input keys */
    int      inlen;     /* length of the input keys */
    char     *mapped;   /* mapped keys */
    int      mappedlen; /* length of the mapped keys string */
    char     mode;      /* mode for which the map is available */
    gboolean remap;     /* if false do not remap the {rhs} of this map */
} Map;

/* this is only to keep the variables together */
static struct {
    GSList *list;
    char   queue[MAP_QUEUE_SIZE];       /* queue holding typed keys */
    int    qlen;                        /* pointer to last char in queue */
    int    resolved;                    /* number of resolved keys (no mapping required) */
    guint  timout_id;                   /* source id of the timeout function */
    char   showcmd[SHOWCMD_LEN + 1];    /* buffer to show ambiguous key sequence */
} map;

extern VbCore vb;

static char *transchar(int c);
static gboolean map_delete_by_lhs(const char *lhs, int len, char mode);
static int keyval_to_string(guint keyval, guint state, guchar *string);
static int utf_char2bytes(guint c, guchar *buf);
static char *convert_keys(const char *in, int inlen, int *len);
static char *convert_keylabel(const char *in, int inlen, int *len);
static gboolean do_timeout(gpointer data);
static void free_map(Map *map);

static struct {
    guint state;
    guint keyval;
    char one;
    char two;
} special_keys[] = {
    {GDK_SHIFT_MASK,    GDK_Tab,       'k', 'B'},
    {0,                 GDK_Up,        'k', 'u'},
    {0,                 GDK_Down,      'k', 'd'},
    {0,                 GDK_Left,      'k', 'l'},
    {0,                 GDK_Right,     'k', 'r'},
    {0,                 GDK_F1,        'k', '1'},
    {0,                 GDK_F2,        'k', '2'},
    {0,                 GDK_F3,        'k', '3'},
    {0,                 GDK_F4,        'k', '4'},
    {0,                 GDK_F5,        'k', '5'},
    {0,                 GDK_F6,        'k', '6'},
    {0,                 GDK_F7,        'k', '7'},
    {0,                 GDK_F8,        'k', '8'},
    {0,                 GDK_F9,        'k', '9'},
    {0,                 GDK_F10,       'k', ';'},
    {0,                 GDK_F11,       'F', '1'},
    {0,                 GDK_F12,       'F', '2'},
};

static struct {
    char *label;
    int  len;
    char *ch;
    int  chlen;
} key_labels[] = {
    {"<CR>",    4, "\x0d",       1},
    {"<Tab>",   5, "\t",         1},
    {"<S-Tab>", 7, CSI_STR "kB", 3},
    {"<Esc>",   5, "\x1b",       1},
    {"<Up>",    4, CSI_STR "ku", 3},
    {"<Down>",  6, CSI_STR "kd", 3},
    {"<Left>",  6, CSI_STR "kl", 3},
    {"<Right>", 7, CSI_STR "kr", 3},
    {"<F1>",    4, CSI_STR "k1", 3},
    {"<F2>",    4, CSI_STR "k2", 3},
    {"<F3>",    4, CSI_STR "k3", 3},
    {"<F4>",    4, CSI_STR "k4", 3},
    {"<F5>",    4, CSI_STR "k5", 3},
    {"<F6>",    4, CSI_STR "k6", 3},
    {"<F7>",    4, CSI_STR "k7", 3},
    {"<F8>",    4, CSI_STR "k8", 3},
    {"<F9>",    4, CSI_STR "k9", 3},
    {"<F10>",   5, CSI_STR "k;", 3},
    {"<F11>",   5, CSI_STR "F1", 3},
    {"<F12>",   5, CSI_STR "F2", 3},
};


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
    guint state  = event->state;
    guint keyval = event->keyval;
    guchar string[32];
    int len;

    len = keyval_to_string(keyval, state, string);

    /* translate iso left tab to shift tab */
    if (keyval == GDK_ISO_Left_Tab) {
        keyval = GDK_Tab;
        state |= GDK_SHIFT_MASK;
    }

    if (len == 0 || len == 1) {
        for (int i = 0; i < LENGTH(special_keys); i++) {
            if (special_keys[i].keyval == keyval
                && (special_keys[i].state == 0 || state & special_keys[i].state)
            ) {
                state &= ~special_keys[i].state;
                string[0] = CSI;
                string[1] = special_keys[i].one;
                string[2] = special_keys[i].two;
                len = 3;
                break;
            }
        }
    }

    if (len == 0) {
        /* mark all unknown key events as unhandled to not break some gtk features
         * like <S-Einf> to copy clipboard content into inputbox */
        return false;
    }

    vb.state.processed_key = true;
    map_handle_keys(string, len, true);

    return vb.state.processed_key;
}

/**
 * Added the given key sequence to the key queue and process the mapping of
 * chars. The key sequence do not need to be NUL terminated.
 * Keylen of 0 signalized a key timeout.
 */
MapState map_handle_keys(const guchar *keys, int keylen, gboolean use_map)
{
    int ambiguous;
    Map *match = NULL;
    gboolean timeout = (keylen == 0); /* keylen 0 signalized timeout */
    static int showlen = 0;           /* track the number of keys in showcmd of status bar */

    /* if a previous timeout function was set remove this */
    if (map.timout_id) {
        g_source_remove(map.timout_id);
        map.timout_id = 0;
    }

    /* don't set the timeout function if the timeout is processed now */
    if (!timeout) {
        map.timout_id = g_timeout_add(vb.config.timeoutlen, (GSourceFunc)do_timeout, NULL);
    }

    /* copy the keys onto the end of queue */
    while (map.qlen < LENGTH(map.queue) && keylen > 0) {
        map.queue[map.qlen++] = *keys++;
        keylen--;
    }

    /* try to resolve keys against the map */
    while (true) {
        /* send any resolved key to the parser */
        while (map.resolved > 0) {
            int qk;

            /* skip csi indicator and the next 2 chars - if the csi sequence
             * isn't part of a mapped command we let gtk handle the key - this
             * is required allow to move cursor in inputbox with <Left> and
             * <Right> keys */
            if ((map.queue[0] & 0xff) == CSI && map.qlen >= 3) {
                /* get next 2 chars to build the termcap key */
                qk = TERMCAP2KEY(map.queue[1], map.queue[2]);

                map.resolved -= 3;
                map.qlen     -= 3;
                /* move all other queue entries three steps to the left */
                memmove(map.queue, map.queue + 3, map.qlen);
            } else {
                /* get first char of queue */
                qk = map.queue[0];

                map.resolved--;
                map.qlen--;

                /* move all other queue entries one step to the left */
                memmove(map.queue, map.queue + 1, map.qlen);
            }

            /* remove the no-map flag */
            vb.mode->flags &= ~FLAG_NOMAP;

            /* send the key to the parser */
            if (RESULT_MORE != mode_handle_key((int)qk)) {
                map_showcmd(0);
                showlen = 0;
            } else if (showlen > 0) {
                showlen--;
            } else {
                map_showcmd(qk);
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
        if (use_map && !(vb.mode->flags & FLAG_NOMAP)) {
            for (GSList *l = map.list; l != NULL; l = l->next) {
                Map *m = (Map*)l->data;
                /* ignore maps for other modes */
                if (m->mode != vb.mode->id) {
                    continue;
                }

                /* find ambiguous matches */
                if (!timeout && m->inlen > map.qlen && !strncmp(m->in, map.queue, map.qlen)) {
                    if (ambiguous == 0) {
                        /* show command chars for the ambiguous commands */
                        int i = map.qlen > SHOWCMD_LEN ? map.qlen - SHOWCMD_LEN : 0;
                        /* appen only those chars that are not already in showcmd */
                        i += showlen;
                        while (i < map.qlen) {
                            map_showcmd(map.queue[i++]);
                            showlen++;
                        }
                    }
                    ambiguous++;
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

            /* if there are ambiguous matches return MAP_KEY and flush queue
             * after a timeout if the user do not type more keys */
            if (ambiguous) {
                return MAP_AMBIGUOUS;
            }
        }

        /* replace the matched chars from queue by the cooked string that
         * is the result of the mapping */
        if (match) {
            int i, j;
            /* flush the show command to make room for possible mapped command
             * chars to show for example if :nmap foo 12g is use we want to
             * display the incomplete 12g command */
            map_showcmd(0);
            showlen = 0;

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

            /* without remap the mapped chars are resolved now */
            if (!match->remap) {
                map.resolved = match->mappedlen;
            } else if (match->inlen <= match->mappedlen
                && !strncmp(match->in, match->mapped, match->inlen)
            ) {
                map.resolved = match->inlen;
            }
        } else {
            /* first char is not mapped but resolved */
            map.resolved = 1;
        }
    }

    g_return_val_if_reached(MAP_DONE);
}

/**
 * Like map_handle_keys but use a null terminates string with untranslated
 * keys like <C-T> that are converted here before calling map_handle_keys.
 */
void map_handle_string(const char *str, gboolean use_map)
{
    int len;
    char *keys = convert_keys(str, strlen(str), &len);

    map_handle_keys((guchar*)keys, len, use_map);
}

void map_insert(const char *in, const char *mapped, char mode, gboolean remap)
{
    int inlen, mappedlen;
    char *lhs = convert_keys(in, strlen(in), &inlen);
    char *rhs = convert_keys(mapped, strlen(mapped), &mappedlen);

    /* if lhs was already mapped, remove this first */
    map_delete_by_lhs(lhs, inlen, mode);

    Map *new = g_slice_new(Map);
    new->in        = lhs;
    new->inlen     = inlen;
    new->mapped    = rhs;
    new->mappedlen = mappedlen;
    new->mode      = mode;
    new->remap     = remap;

    map.list = g_slist_prepend(map.list, new);
}

gboolean map_delete(const char *in, char mode)
{
    int len;
    char *lhs = convert_keys(in, strlen(in), &len);

    return map_delete_by_lhs(lhs, len, mode);
}

/**
 * Put the given char onto the show command buffer.
 */
void map_showcmd(int c)
{
    char *translated;
    int old, extra, overflow;

    if (c) {
        translated = transchar(c);
        old        = strlen(map.showcmd);
        extra      = strlen(translated);
        overflow   = old + extra - SHOWCMD_LEN;
        if (overflow > 0) {
            memmove(map.showcmd, map.showcmd + overflow, old - overflow + 1);
        }
        strcat(map.showcmd, translated);
    } else {
        map.showcmd[0] = '\0';
    }
#ifndef TESTLIB
    /* show the typed keys */
    gtk_label_set_text(GTK_LABEL(vb.gui.statusbar.cmd), map.showcmd);
#endif
}

/**
 * Translate a singe char into a readable representation to be show to the
 * user in status bar.
 */
static char *transchar(int c)
{
    static char trans[5];
    int i = 0;
    if (VB_IS_CTRL(c)) {
        trans[i++] = '^';
        trans[i++] = CTRL(c);
    } else if ((unsigned)c >= 0x80) {
        trans[i++] = '<';
        trans[i++] = NR2HEX((unsigned)c >> 4);
        trans[i++] = NR2HEX((unsigned)c);
        trans[i++] = '>';
    } else {
        trans[i++] = c;
    }
    trans[i++] = '\0';

    return trans;
}

static gboolean map_delete_by_lhs(const char *lhs, int len, char mode)
{
    for (GSList *l = map.list; l != NULL; l = l->next) {
        Map *m = (Map*)l->data;

        /* remove only if the map's lhs matches the given key sequence */
        if (m->mode == mode && m->inlen == len && !strcmp(m->in, lhs)) {
            /* remove the found list item */
            map.list = g_slist_delete_link(map.list, l);
            free_map(m);
            return true;
        }
    }
    return false;
}

/**
 * Translate a keyvalue to utf-8 encoded and null terminated string.
 * Given string must have room for 6 bytes.
 */
static int keyval_to_string(guint keyval, guint state, guchar *string)
{
    int len;
    guint32 uc;

    len = 1;
    switch (keyval) {
        case GDK_Tab:
        case GDK_KP_Tab:
        case GDK_ISO_Left_Tab:
            string[0] = KEY_TAB;
            break;

        case GDK_Linefeed:
            string[0] = KEY_NL;
            break;

        case GDK_Return:
        case GDK_ISO_Enter:
        case GDK_3270_Enter:
            string[0] = KEY_CR;
            break;

        case GDK_Escape:
            string[0] = KEY_ESC;
            break;

        case GDK_BackSpace:
            string[0] = KEY_BS;
            break;

        default:
            if ((uc = gdk_keyval_to_unicode(keyval))) {
                if ((state & GDK_CONTROL_MASK) && uc >= 0x20 && uc < 0x80) {
                    if (uc >= '@') {
                        string[0] = uc & 0x1f;
                    } else if (uc == '8') {
                        string[0] = KEY_BS;
                    } else {
                        string[0] = uc;
                    }
                } else {
                    /* translate a normal key to utf-8 */
                    len = utf_char2bytes((guint)uc, string);
                }
            } else {
                len = 0;
            }
            break;
    }

    return len;
}

static int utf_char2bytes(guint c, guchar *buf)
{
    if (c < 0x80) {
        buf[0] = c;
        return 1;
    }
    if (c < 0x800) {
        buf[0] = 0xc0 + (c >> 6);
        buf[1] = 0x80 + (c & 0x3f);
        return 2;
    }
    if (c < 0x10000) {
        buf[0] = 0xe0 + (c >> 12);
        buf[1] = 0x80 + ((c >> 6) & 0x3f);
        buf[2] = 0x80 + (c & 0x3f);
        return 3;
    }
    if (c < 0x200000) {
        buf[0] = 0xf0 + (c >> 18);
        buf[1] = 0x80 + ((c >> 12) & 0x3f);
        buf[2] = 0x80 + ((c >> 6) & 0x3f);
        buf[3] = 0x80 + (c & 0x3f);
        return 4;
    }
    if (c < 0x4000000) {
        buf[0] = 0xf8 + (c >> 24);
        buf[1] = 0x80 + ((c >> 18) & 0x3f);
        buf[2] = 0x80 + ((c >> 12) & 0x3f);
        buf[3] = 0x80 + ((c >> 6) & 0x3f);
        buf[4] = 0x80 + (c & 0x3f);
        return 5;
    }
    buf[0] = 0xfc + (c >> 30);
    buf[1] = 0x80 + ((c >> 24) & 0x3f);
    buf[2] = 0x80 + ((c >> 18) & 0x3f);
    buf[3] = 0x80 + ((c >> 12) & 0x3f);
    buf[4] = 0x80 + ((c >> 6) & 0x3f);
    buf[5] = 0x80 + (c & 0x3f);
    return 6;
}

/**
 * Converts a keysequence into a internal raw keysequence.
 * Returned keyseqence must be freed if not used anymore.
 */
static char *convert_keys(const char *in, int inlen, int *len)
{
    int symlen, rawlen;
    char *dest;
    char ch[1];
    const char *p, *raw;
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
                    if (VB_IS_UPPER(p[3])) {
                        ch[0]  = p[3] - 0x40;
                        raw    = ch;
                        rawlen = 1;
                    } else if (VB_IS_LOWER(p[3])) {
                        ch[0]  = p[3] - 0x60;
                        raw    = ch;
                        rawlen = 1;
                    }
                }
            }

            /* if we could not convert it jet - try to translate the label */
            if (!rawlen) {
                raw = convert_keylabel(p, symlen, &rawlen);
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
static char *convert_keylabel(const char *in, int inlen, int *len)
{
    for (int i = 0; i < LENGTH(key_labels); i++) {
        if (inlen == key_labels[i].len
            && !strncmp(key_labels[i].label, in, inlen)
        ) {
            *len = key_labels[i].chlen;
            return key_labels[i].ch;
        }
    }
    *len = 0;

    return NULL;
}

/**
 * Timeout function to signalize a key timeout to the map.
 */
static gboolean do_timeout(gpointer data)
{
    /* signalize the timeout to the key handler */
    map_handle_keys((guchar*)"", 0, true);

    /* we return true to not automatically remove the resource - this is
     * required to prevent critical error when we remove the source in
     * map_handle_keys where we don't know if the timeout was called or not */
    return true;
}

static void free_map(Map *map)
{
    g_free(map->in);
    g_free(map->mapped);
    g_slice_free(Map, map);
}
