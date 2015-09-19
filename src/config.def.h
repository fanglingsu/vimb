/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2015 Daniel Carl
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

/* features */
/* should the history indicator [+-] be shown in status bar after url */
#define FEATURE_HISTORY_INDICATOR
/* show wget style progressbar in status bar */
#define FEATURE_WGET_PROGRESS_BAR
#ifdef FEATURE_WGET_PROGRESS_BAR
/* chars to use for the progressbar */
#define PROGRESS_BAR                "=> "
#define PROGRESS_BAR_LEN            20
#endif
/* show page title in url completions */
#define FEATURE_TITLE_IN_COMPLETION

/* time in seconds after that message will be removed from inputbox if the
 * message where only temporary */
#define MESSAGE_TIMEOUT             5

/* number of chars to be shown in statusbar for ambiguous commands */
#define SHOWCMD_LEN                 10
/* css applied to the gui elements of the borwser window */
#define GUI_STYLE                   "GtkBox#statusbar{color:#fff;background-color:#000;font:monospace bold 10;} \
GtkBox#statusbar.secure{background-color:#95e454;color:#000;} \
GtkBox#statusbar.insecure{background-color:#f77;color:#000;} \
GtkTextView{background-color:#fff;color:#000;font:monospace 10;} \
GtkTextView.error{background-color:#f77;font-weight:bold;} \
GtkTreeView{color:#fff;background-color:#656565;font:monospace;} \
GtkTreeView:hover{background-color:#777;} \
GtkTreeView:selected{color:#f6f3e8;background-color:#888;}"

/* default font size for fonts in webview */
#define SETTING_DEFAULT_FONT_SIZE   10
#define SETTING_HOME_PAGE           "about:blank"
