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

#ifndef _ASCII_H
#define _ASCII_H

/* CSI (control sequence introducer) is the first byte of a control sequence
 * and is always followed by two bytes. */
#define CSI         0x9b
#define CSI_STR     "\233"

#define IS_SPECIAL(c)       (c < 0)

#define TERMCAP2KEY(a, b)   (-((a) + ((int)(b) << 8)))
#define KEY2TERMCAP0(x)     ((-(x)) & 0xff)
#define KEY2TERMCAP1(x)     (((unsigned)(-(x)) >> 8) & 0xff)

#define KEY_TAB         '\x09'
#define KEY_NL          '\x15'
#define KEY_CR          '\x0a'
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
