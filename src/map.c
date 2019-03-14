/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2019 Daniel Carl
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

#include <gdk/gdkkeysyms-compat.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>

#include "ascii.h"
#include "config.h"
#include "main.h"
#include "map.h"
#include "util.h"

static char *convert_keylabel(const char *in, int inlen, int *len);
static char *convert_keys(const char *in, int inlen, int *len);
static gboolean do_timeout(Client *c);
static void free_map(Map *map);
static int keyval_to_string(guint keyval, guint state, guchar *string);
static gboolean map_delete_by_lhs(Client *c, const char *lhs, int len, char mode);
static void showcmd(Client *c, int ch);
static char *transchar(int c);
static int utf_char2bytes(guint c, guchar *buf);
static void queue_event(GdkEventKey *e);
static void process_events(void);
static void free_events(void);
static void process_event(GdkEventKey* event);

extern struct Vimb vb;

static struct {
    guint state;
    guint keyval;
    char one;
    char two;
} special_keys[] = {
    /* TODO: In GTK3, keysyms changed to have a KEY_ prefix.
     * See gdkkeysyms.h and gdkkeysyms-compat.h
     */
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
    {"<Space>", 7, "\x20",       1},
    {"<BS>",    4, "\x08",       1},
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

/* this is only to queue GDK key events, in order to later send them if the map didn't match */
static struct {
    GSList   *list;       /* queue holding submitted events */
    gboolean processing; /* whether or not events are processing */
} events = {0};

void map_init(Client *c)
{
    c->map.queue = g_string_sized_new(50);
    /* TODO move this to settings */
    c->map.timeoutlen = 1000;
}

void map_cleanup(Client *c)
{
    if (c->map.list) {
        g_slist_free_full(c->map.list, (GDestroyNotify)free_map);
    }
    if (c->map.queue) {
        g_string_free(c->map.queue, TRUE);
    }
}

/**
 * Added the given key sequence to the key queue and process the mapping of
 * chars. The key sequence do not need to be NUL terminated.
 * Keylen of 0 signalized a key timeout.
 */
MapState map_handle_keys(Client *c, const guchar *keys, int keylen, gboolean use_map)
{
    int ambiguous;
    Map *match = NULL;
    gboolean timeout = (keylen == 0); /* keylen 0 signalized timeout */
    static int showlen = 0;           /* track the number of keys in showcmd of status bar */

    /* if a previous timeout function was set remove this */
    if (c->map.timout_id) {
        g_source_remove(c->map.timout_id);
        c->map.timout_id = 0;
    }

    /* don't set the timeout function if the timeout is processed now */
    if (!timeout) {
        c->map.timout_id = g_timeout_add(c->map.timeoutlen, (GSourceFunc)do_timeout, c);
    }

    /* copy the keys onto the end of queue */
    if (keylen > 0) {
        g_string_overwrite_len(c->map.queue, c->map.qlen, (char*)keys, keylen);
        c->map.qlen += keylen;
    }

    /* try to resolve keys against the map */
    while (TRUE) {
        /* send any resolved key to the parser */
        while (c->map.resolved > 0) {
            int qk;

            /* skip csi indicator and the next 2 chars - if the csi sequence
             * isn't part of a mapped command we let gtk handle the key - this
             * is required allow to move cursor in inputbox with <Left> and
             * <Right> keys */
            if ((c->map.queue->str[0] & 0xff) == CSI && c->map.qlen >= 3) {
                /* get next 2 chars to build the termcap key */
                qk = TERMCAP2KEY(c->map.queue->str[1], c->map.queue->str[2]);

                c->map.resolved -= 3;
                c->map.qlen     -= 3;
                /* move all other queue entries three steps to the left */
                memmove(c->map.queue->str, c->map.queue->str + 3, c->map.qlen);
            } else {
                /* get first char of queue */
                qk = c->map.queue->str[0];

                c->map.resolved--;
                c->map.qlen--;

                /* move all other queue entries one step to the left */
                memmove(c->map.queue->str, c->map.queue->str + 1, c->map.qlen);
            }

            /* remove the no-map flag */
            c->mode->flags &= ~FLAG_NOMAP;

            /* send the key to the parser */
            if (RESULT_MORE != vb_mode_handle_key(c, (int)qk)) {
                showcmd(c, 0);
                showlen = 0;
            } else if (showlen > 0) {
                showlen--;
            } else {
                showcmd(c, qk);
            }
        }

        /* if all keys where processed return MAP_DONE */
        if (c->map.qlen == 0) {
            c->map.resolved = 0;
            return match ? MAP_DONE : MAP_NOMATCH;
        }

        /* try to find matching maps */
        match     = NULL;
        ambiguous = 0;
        if (use_map && !(c->mode->flags & FLAG_NOMAP)) {
            for (GSList *l = c->map.list; l != NULL; l = l->next) {
                Map *m = (Map*)l->data;
                /* ignore maps for other modes */
                if (m->mode != c->mode->id) {
                    continue;
                }

                /* find ambiguous matches */
                if (!timeout && m->inlen > c->map.qlen && !strncmp(m->in, c->map.queue->str, c->map.qlen)) {
                    if (ambiguous == 0) {
                        /* show command chars for the ambiguous commands */
                        int i = c->map.qlen > SHOWCMD_LEN ? c->map.qlen - SHOWCMD_LEN : 0;
                        /* appen only those chars that are not already in showcmd */
                        i += showlen;
                        while (i < c->map.qlen) {
                            showcmd(c, c->map.queue->str[i++]);
                            showlen++;
                        }
                    }
                    ambiguous++;
                }
                /* complete match or better/longer match than previous found */
                if (m->inlen <= c->map.qlen
                    && !strncmp(m->in, c->map.queue->str, m->inlen)
                    && (!match || match->inlen < m->inlen)
                ) {
                    /* backup this found possible match */
                    match = m;
                }
            }

            /* if there are ambiguous matches return MAP_AMBIGUOUS and flush queue
             * after a timeout if the user do not type more keys */
            if (ambiguous) {
                return MAP_AMBIGUOUS;
            }
        }

        /* Replace the matched chars from queue by the cooked string that
         * is the result of the mapping. */
        if (match) {
            /* Flush the show command to make room for possible mapped command
             * chars to show. For example if :nmap foo 12g is use we want to
             * display the incomplete 12g command. */
            showcmd(c, 0);
            showlen = 0;

            /* Replace the matching input chars by the mapped chars. */
            if (match->inlen == match->mappedlen) {
                /* If inlen and mappedlen are the same - replace the inlin
                 * chars with the mapped chars. This case could also be
                 * handled by the later string erase and prepend, but handling
                 * it special avoids unneded function call. */
                g_string_overwrite_len(c->map.queue, 0, match->mapped, match->mappedlen);
            } else {
                /* Remove all the chars that where matched and prepend the
                 * mapped chars to the queue. */
                g_string_erase(c->map.queue, 0, match->inlen);
                g_string_prepend_len(c->map.queue, match->mapped, match->mappedlen);
            }
            c->map.qlen += match->mappedlen - match->inlen;

            /* without remap the mapped chars are resolved now */
            if (!match->remap) {
                c->map.resolved = match->mappedlen;
            } else if (match->inlen <= match->mappedlen
                && !strncmp(match->in, match->mapped, match->inlen)
            ) {
                c->map.resolved = match->inlen;
            }
            /* Unset the typed flag - if there where keys replaced by a
             * mapping the resulting key string is considered as not typed by
             * the user. */
            c->state.typed = FALSE;
        } else {
            /* first char is not mapped but resolved */
            c->map.resolved = 1;
        }
    }

    g_return_val_if_reached(MAP_DONE);
}

/**
 * Like map_handle_keys but use a null terminates string with untranslated
 * keys like <C-T> that are converted here before calling map_handle_keys.
 */
void map_handle_string(Client *c, const char *str, gboolean use_map)
{
    int len;
    char *keys = convert_keys(str, strlen(str), &len);
    map_handle_keys(c, (guchar*)keys, len, use_map);
}

void map_insert(Client *c, const char *in, const char *mapped, char mode, gboolean remap)
{
    int inlen, mappedlen;
    char *lhs = convert_keys(in, strlen(in), &inlen);
    char *rhs = convert_keys(mapped, strlen(mapped), &mappedlen);

    /* if lhs was already mapped, remove this first */
    map_delete_by_lhs(c, lhs, inlen, mode);

    Map *new = g_slice_new(Map);
    new->in        = lhs;
    new->inlen     = inlen;
    new->mapped    = rhs;
    new->mappedlen = mappedlen;
    new->mode      = mode;
    new->remap     = remap;

    c->map.list = g_slist_prepend(c->map.list, new);
}

gboolean map_delete(Client *c, const char *in, char mode)
{
    int len;
    char *lhs = convert_keys(in, strlen(in), &len);

    return map_delete_by_lhs(c, lhs, len, mode);
}

/**
 * Handle all key events, convert the key event into the internal used ASCII
 * representation and put this into the key queue to be mapped.
 */
gboolean on_map_keypress(GtkWidget *widget, GdkEventKey* event, Client *c)
{
    if (events.processing) {
        /* events are processing, pass all keys unmodified */
        return FALSE;
    }

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
        return FALSE;
    }

    /* set flag to notify that the key was typed by the user */
    c->state.typed         = TRUE;
    c->state.processed_key = TRUE;

    queue_event(event);

    MapState res = map_handle_keys(c, string, len, true);

    if (res != MAP_AMBIGUOUS) {
        if (!c->state.processed_key) {
            /* events ready to be consumed */
            process_events();
        } else {
            /* no ambiguous - key processed elsewhere */
            free_events();
        }
    }

    /* reset the typed flag */
    c->state.typed = FALSE;

    /* prevent input from going to GDK - input is sent via process_events(); */
    return TRUE;
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
    g_string_free(str, FALSE);

    return dest;
}

/**
 * Timeout function to signalize a key timeout to the map.
 */
static gboolean do_timeout(Client *c)
{
    /* signalize the timeout to the key handler */
    map_handle_keys(c, (guchar*)"", 0, TRUE);

    /* consume any unprocessed events */
    process_events();

    /* we return TRUE to not automatically remove the resource - this is
     * required to prevent critical error when we remove the source in
     * map_handle_keys where we don't know if the timeout was called or not */
    return TRUE;
}

static void free_map(Map *map)
{
    g_free(map->in);
    g_free(map->mapped);
    g_slice_free(Map, map);
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

static gboolean map_delete_by_lhs(Client *c, const char *lhs, int len, char mode)
{
    for (GSList *l = c->map.list; l != NULL; l = l->next) {
        Map *m = (Map*)l->data;

        /* remove only if the map's lhs matches the given key sequence */
        if (m->mode == mode && m->inlen == len && !strcmp(m->in, lhs)) {
            /* remove the found list item */
            c->map.list = g_slist_delete_link(c->map.list, l);
            free_map(m);
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * Put the given char onto the show command buffer.
 */
static void showcmd(Client *c, int ch)
{
    char *translated;
    int old, extra, overflow;

    if (ch) {
        translated = transchar(ch);
        old        = strlen(c->map.showcmd);
        extra      = strlen(translated);
        overflow   = old + extra - SHOWCMD_LEN;
        if (overflow > 0) {
            memmove(c->map.showcmd, c->map.showcmd + overflow, old - overflow + 1);
        }
        strcat(c->map.showcmd, translated);
    } else {
        c->map.showcmd[0] = '\0';
    }
#ifndef TESTLIB
    /* show the typed keys */
    gtk_label_set_text(GTK_LABEL(c->statusbar.cmd), c->map.showcmd);
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
/* convert the lower 4 bits of byte n to its hex character */
#define NR2HEX(n)   (n & 0xf) <= 9 ? (n & 0xf) + '0' : (c & 0xf) - 10 + 'a'
        trans[i++] = '<';
        trans[i++] = NR2HEX((unsigned)c >> 4);
        trans[i++] = NR2HEX((unsigned)c);
        trans[i++] = '>';
#undef NR2HEX
    } else {
        trans[i++] = c;
    }
    trans[i++] = '\0';

    return trans;
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
 * Append an event into the queue.
 */
static void queue_event(GdkEventKey *e)
{
    events.list = g_slist_append(events.list, gdk_event_copy((GdkEvent*)e));
}

/**
 * Process events in the queue, sending the key events to GDK.
 */
static void process_events(void)
{
    for (GSList *l = events.list; l != NULL; l = l->next) {
        process_event((GdkEventKey*)l->data);
        /* TODO take into account qk mapped key? */
    }
    free_events();
}

/**
 * Clear the events list and free the allocated memory.
 */
static void free_events(void)
{
    if (events.list) {
        g_slist_free_full(events.list, (GDestroyNotify)gdk_event_free);
        events.list = NULL;
    }
}

static void process_event(GdkEventKey* event)
{
    if (event) {
        /* signal not to queue other events */
        events.processing = TRUE;
        gtk_main_do_event((GdkEvent*)event);
        events.processing = FALSE;
    }
}
