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

#ifndef _CONFIG_H
#define _CONFIG_H


/* features */
/* enable cookie support */
#define FEATURE_COOKIE
/* highlight search results */
#define FEATURE_SEARCH_HIGHLIGHT
/* disable scrollbars */
#define FEATURE_NO_SCROLLBARS
/* show page title in url completions */
#define FEATURE_TITLE_IN_COMPLETION
/* enable the read it later queue */
#define FEATURE_QUEUE
/* show load progress in window title */
#define FEATURE_TITLE_PROGRESS
/* should the history indicator [+-] be shown in status bar after url */
#define FEATURE_HISTORY_INDICATOR
/* show wget style progressbar in status bar */
#define FEATURE_WGET_PROGRESS_BAR
#ifdef HAS_GTK3
/* enables workaround for hight dpi displays */
/* eventually the environment variable GDK_DPI_SCALE=2.0 must be set */
/* to get the hack working */
/* #define FEATURE_HIGH_DPI */
#endif
/* enable HTTP Strict-Transport-Security*/
#define FEATURE_HSTS
/* enable soup caching - size can be configure by maximum-cache-size setting */
#define FEATURE_SOUP_CACHE

/* time in seconds after that message will be removed from inputbox if the
 * message where only temporary */
#define MESSAGE_TIMEOUT              5

/* number of chars to be shown for ambiguous commands */
#define SHOWCMD_LEN                 10

#define SETTING_MAX_CONNS           25
#define SETTING_MAX_CONNS_PER_HOST   5
/* default font size for fonts in webview */
#define SETTING_DEFAULT_FONT_SIZE   10
#define SETTING_GUI_FONT_NORMAL     "monospace normal 10"
#define SETTING_GUI_FONT_EMPH       "monospace bold 10"
#define SETTING_HOME_PAGE           "http://fanglingsu.github.io/vimb/"

#define MAXIMUM_HINTS              500

#define WIN_WIDTH                  800
#define WIN_HEIGHT                 600

#ifdef FEATURE_WGET_PROGRESS_BAR
/* chars to use for the progressbar */
#define PROGRESS_BAR             "=> "
#define PROGRESS_BAR_LEN            20
#endif

#endif /* end of include guard: _CONFIG_H */
