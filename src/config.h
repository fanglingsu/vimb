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

#ifndef CONFIG_H
#define CONFIG_H

#define START_PAGE      "http://sourceforge.net/apps/trac/vimprobable"

#define STATUS_BG_COLOR "#000000"           /* background color for status bar */
#define STATUS_FG_COLOR "#ffffff"           /* foreground color for status bar */
#define STATUS_BAR_FONT "monospace bold 8"

                                    /* normal                error */
static const char *inputbox_font[] = { "monospace normal 8", "monospace bold 8"};
static const char *inputbox_fg[]   = { "#000000",            "#000000" };
static const char *inputbox_bg[]   = { "#ffffff",            "#ff0000" };


#define SETTING_DEFAUL_FONT_SIZE    12
#define SETTING_USER_AGENT          PROJECT "/" VERSION " (X11; Linux i686) AppleWebKit/535.22+ Compatible (Safari)"
#define SETTING_MAX_CONNS           25
#define SETTING_MAX_CONNS_PER_HOST  5

#define SCROLLSTEP (40) /* cursor difference in pixel for scrolling */

#endif /* end of include guard: CONFIG_H */
