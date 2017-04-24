/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2016 Daniel Carl
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
/* show wget style progressbar in status bar */
#define FEATURE_WGET_PROGRESS_BAR
/* show load progress in window title */
#define FEATURE_TITLE_PROGRESS
/* show page title in url completions */
#define FEATURE_TITLE_IN_COMPLETION
/* support gui style settings compatible with vimb2 */
/* #define FEATURE_GUI_STYLE_VIMB2_COMPAT */
/* enable the read it later queue */
#define FEATURE_QUEUE

#ifdef FEATURE_WGET_PROGRESS_BAR
/* chars to use for the progressbar */
#define PROGRESS_BAR             "=> "
#define PROGRESS_BAR_LEN            20
#endif

/* time in seconds after that message will be removed from inputbox if the
 * message where only temporary */
#define MESSAGE_TIMEOUT             5

/* number of chars to be shown in statusbar for ambiguous commands */
#define SHOWCMD_LEN                 10
/* css applied to the gui elements regardless of user's settings */
#define GUI_STYLE_CSS_BASE          "#input text{background-color:inherit;color:inherit;caret-color:@color;font:inherit;}"

/* default font size for fonts in webview */
#define SETTING_DEFAULT_FONT_SIZE             16
#define SETTING_DEFAULT_MONOSPACE_FONT_SIZE   13
#define SETTING_GUI_FONT_NORMAL               "10pt monospace"
#define SETTING_GUI_FONT_EMPH                 "bold 10pt monospace"
#define SETTING_HOME_PAGE                     "about:blank"

/* CSS style use on creating hints. This might also be averrules by css out of
 * $XDG_CONFIG_HOME/vimb/style.css file. */
#define HINT_CSS "#_hintContainer{\
position:static\
}\
._hintLabel{\
-webkit-transform:translate(-4px,-4px);\
position:absolute;\
z-index:100000;\
font:bold .8em monospace;\
color:#000;\
background-color:#fff;\
margin:0;\
padding:0px 1px;\
border:1px solid #444;\
opacity:0.7\
}\
._hintElem{\
background-color:#ff0 !important;\
color:#000 !important;\
transition:all 0 !important;\
transition-delay:all 0 !important\
}\
._hintElem._hintFocus{\
background-color:#8f0 !important\
}\
._hintLabel._hintFocus{\
z-index:100001;\
opacity:1\
}"

