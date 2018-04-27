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

#ifndef _ASCII_H
#define _ASCII_H

#define VB_UPPER  0x01
#define VB_LOWER  0x02
#define VB_DIGIT  0x04
#define VB_SPACE  0x08
#define VB_PUNKT  0x10
#define VB_CTRL   0x20

#define U  VB_UPPER
#define L  VB_LOWER
#define D  VB_DIGIT
#define P  VB_PUNKT
#define S  VB_SPACE
#define C  VB_CTRL
#define SC VB_SPACE|VB_CTRL
static const unsigned char chartable[256] = {
/* 0x00-0x0f */  C,  C,  C,  C,  C,  C,  C,  C,  C, SC, SC,  C, SC, SC,  C,  C,
/* 0x10-0x1f */  C,  C,  C,  C,  C,  C,  C,  C,  C,  C,  C,  C,  C,  C,  C,  C,
/* 0x20-0x2f */  S,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,
/* 0x30-0x3f */  D,  D,  D,  D,  D,  D,  D,  D,  D,  D,  P,  P,  P,  P,  P,  P,
/* 0x40-0x4f */  P,  U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  U,
/* 0x50-0x5f */  U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  P,  P,  P,  P,  P,
/* 0x60-0x6f */  P,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,
/* 0x70-0x7f */  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  P,  P,  P,  P,  C,
/* 0x80-0x8f */  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,
/* 0x90-0x9f */  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,
/* 0xa0-0xaf */  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,
/* 0xb0-0xbf */  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,
/* 0xc0-0xcf */  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,
/* 0xd0-0xdf */  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,
/* 0xe0-0xef */  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,
/* 0xf0-0xff */  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P,  P
};
#undef U
#undef L
#undef D
#undef P
#undef S
#undef C
#undef SC

#define VB_IS_UPPER(c)     ((chartable[(unsigned char)c] & VB_UPPER) != 0)
#define VB_IS_LOWER(c)     ((chartable[(unsigned char)c] & VB_LOWER) != 0)
#define VB_IS_DIGIT(c)     ((chartable[(unsigned char)c] & VB_DIGIT) != 0)
#define VB_IS_PUNKT(c)     ((chartable[(unsigned char)c] & VB_PUNKT) != 0)
#define VB_IS_SPACE(c)     ((chartable[(unsigned char)c] & VB_SPACE) != 0)
#define VB_IS_CTRL(c)      ((chartable[(unsigned char)c] & VB_CTRL) != 0)
#define VB_IS_SEPARATOR(c) (VB_IS_SPACE(c) || c == '"' || c == '\'')
#define VB_IS_ALPHA(c)     (VB_IS_LOWER(c) || VB_IS_UPPER(c))
#define VB_IS_ALNUM(c)     (VB_IS_ALPHA(c) || VB_IS_DIGIT(c))
#define VB_IS_IDENT(c)     (VB_IS_ALNUM(c) || c == '_')

/* CSI (control sequence introducer) is the first byte of a control sequence
 * and is always followed by two bytes. */
#define CSI                 0x80
#define CSI_STR             "\x80"

/* get internal representation for conrol character ^C */
#define CTRL(c)             ((c) ^ 0x40)
#define UNCTRL(c)           (((c) ^ 0x40) + 'a' - 'A')

#define IS_SPECIAL(c)       (c < 0)

#define TERMCAP2KEY(a, b)   (-((a) + ((int)(b) << 8)))
#define KEY2TERMCAP0(x)     ((-(x)) & 0xff)
#define KEY2TERMCAP1(x)     (((unsigned)(-(x)) >> 8) & 0xff)

#define KEY_TAB         '\x09'
#define KEY_NL          '\x15'
#define KEY_CR          '\x0d'
#define KEY_ESC         '\x1b'
#define KEY_BS          '\x08'
#define KEY_SHIFT_TAB   TERMCAP2KEY('k', 'B')
#define KEY_UP          TERMCAP2KEY('k', 'u')
#define KEY_DOWN        TERMCAP2KEY('k', 'd')
#define KEY_LEFT        TERMCAP2KEY('k', 'l')
#define KEY_RIGHT       TERMCAP2KEY('k', 'r')
#define KEY_F1          TERMCAP2KEY('k', '1')
#define KEY_F2          TERMCAP2KEY('k', '2')
#define KEY_F3          TERMCAP2KEY('k', '3')
#define KEY_F4          TERMCAP2KEY('k', '4')
#define KEY_F5          TERMCAP2KEY('k', '5')
#define KEY_F6          TERMCAP2KEY('k', '6')
#define KEY_F7          TERMCAP2KEY('k', '7')
#define KEY_F8          TERMCAP2KEY('k', '8')
#define KEY_F9          TERMCAP2KEY('k', '9')
#define KEY_F10         TERMCAP2KEY('k', ';')
#define KEY_F11         TERMCAP2KEY('F', '1')
#define KEY_F12         TERMCAP2KEY('F', '2')

#endif /* end of include guard: _ASCII_H */
