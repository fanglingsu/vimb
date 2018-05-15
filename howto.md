---
title:  Vimb - user scripts
layout: default
meta:   Tipp, Tricks and scripts to make vimb even more productive
active: howto
---

# howto

* toc
{:toc}

## reading mode via simplyread

If you like to read web pages without clutter you can use some user script
like [simplyread][] to add reading mode to vimb by:

1. downloading the sources of simplyread and copy simplyread.js as
  `$XDG_CONFIG_HOME/vimb/scripts.js`
2. adding some nice keybinding to toggle the reading mode on and off like

       :nnoremap <C-R> :e! simplyread();<CR>

## editor opens without the contents of textarea

There are two known cases of the `editor-command` setting that do not work.

1. If gvim or vim -g is used without the `-f` option. So to use gvim as editor
   you should use

       :set editor-command=gvim -f %s

2. If an editor ist started in `urxvtc` terminal as client for the `urxvtd`
   daemon. This does not work. So if you intend to use vim or another cli
   editor together with `urxvt` make sure you use `urxvt` and not `urxvtc`
   even if you still have the daemon running.

       :set editor-command=urxvt -e vim %s

## how can i have tabs?
{:#tabbed}

Vimb does not support tabs. Every new page is opened in a new browser instance
with own settings which makes things easier and secure. But vimb can be
plugged into another xembed aware window that allows tabbing like [tabbed][].

    tabbed -c vimb -e

To manage the tabs from within the vim like browser, you can use [xdotool][]
to run keyevents on tabbed and call the xdotool from within vimb.

Following keybindings simulate a little bit the vim behaviour.

    nnoremap gt :sh! xdotool key --window $VIMB_XID ctrl+shift+l<CR><Esc>
    nnoremap gT :sh! xdotool key --window $VIMB_XID ctrl+shift+h<CR><Esc>
    nnoremap 1gt :sh! xdotool key --window $VIMB_XID ctrl+1<CR><Esc>
    ...
    nnoremap 9gt :sh! xdotool key --window $VIMB_XID ctrl+9<CR><Esc>


## commented sample config

_~/.config/vimb/config_

```
# Homepage that vimb opens if started without a URI.
set home-page=about:blank

# Path to the default download directory. If no download directory is set,
# download will be written into current directory. The following pattern will
# be expanded if the download is started '~/', '~user', '$VAR' and '${VAR}'.
set download-path=~/tmp/

# Command with placeholder '%s' called if form field is opened with $EDITOR to
# spawn the editor-like `x-terminal-emulator -e vim %s'. To use Gvim as the
# editor, it's necessary to call it with `-f' to run it in the foreground.
set editor-command=termite -e "nvim %s"

# If enabled the inputbox will be hidden whenever it contains no text.
set input-autohide=true

# Enable or disable the spell checking feature.
set spell-checking=true

# Set comma separated list of spell checking languages to be used for
# spell checking.
set spell-checking-languages=en,de

# Enable or disable support for WebGL on pages.
set webgl=true

# While typing a search command, show where the pattern typed so far matches.
set incsearch=true

# The font family to use as the default for content that does not specify a
# font.
set default-font=DejaVu Sans

# The font family used as the default for content using monospace font.
set monospace-font=DejaVu Sans Mono

# The font family used as the default for content using sans-serif font.
set sans-serif-font=DejaVu Sans

# The font family used as the default for content using serif font.
set serif-font=DejaVu Serif

# The default font size used to display text.
set font-size=16

# Default font size for the monospace font.
set monospace-font-size=13

# Default Full-Content zoom level in percent. Default is 100.
set default-zoom=120

# Shortcuts allow the opening of an URI built up from a named template with
# additional parameters.
shortcut-add duck=https://duckduckgo.com/?q=$0
shortcut-add d=http://dict.cc/?s=$0
shortcut-add g=https://encrypted.google.com/search?q=$0
shortcut-add y=http://www.youtube.com/results?search_query=$0
shortcut-add s=https://www.startpage.com/do/dsearch?query=$0

# Set the shortcut as the default, that is the shortcut to be used if no
# shortcut is given and the string to open is not an URI.
shortcut-default duck

# Map page zoom in normal mode to keys commonly used across applications
# + (zoom in), - (zoom out), = (zoom reset)
nmap + zI
nmap - zO
nmap = zz

# GUI color settings
# Color scheme: Base16 Eighties (https://github.com/chriskempson/base16)
set completion-css=color:#d3d0c8;background-color:#393939;font:12pt DejaVu Sans Mono;
set completion-hover-css=color:#d3d0c8;background-color:#393939;font:12pt DejaVu Sans Mono;
set completion-selected-css=color:#d3d0c8;background-color:#515151;font:12pt DejaVu Sans Mono;
set input-css=color:#d3d0c8;background-color:#393939;font:12pt DejaVu Sans Mono;
set input-error-css=color:#f2777a;background-color:#393939;font:12pt DejaVu Sans Mono;
set status-css=color:#ffcc66;background-color:#393939;font:12pt DejaVu Sans Mono;
set status-ssl-css=color:#99cc99;background-color:#393939;font:12pt DejaVu Sans Mono;
set status-ssl-invalid-css=color:#f2777a;background-color:#393939;font:12pt DejaVu Sans Mono;
```

## dark theme

_~/.config/vimb/style.css_

```css
*,div,pre,textarea,body,input,td,tr,p {
    background-color: #303030 !important;
    background-image: none !important;
    color: #bbbbbb !important;
}
h1,h2,h3,h4 {
    background-color: #303030 !important;
    color: #b8ddea !important;
}
a {
    color: #70e070 !important;
}
a:hover,a:focus {
    color: #7070e0 !important;
}
a:visited {
    color: #e07070 !important;
}
img {
    opacity: .5;
}
img:hover {
    opacity: 1;
}

/* Hint mode color styling
 * Color scheme: Base16 Eighties (https://github.com/chriskempson/base16)
 *
 * The precedence of the user style is lower than that of the website so you
 * have to mark your style definition to have higher priority.
 */
._hintLabel {
    background-color: #f2f0ec !important;
    border: 1px solid #2d2d2d !important;
    color: #2d2d2d !important;
    font: bold 10pt monospace !important;
    opacity: 1 !important;
    padding: 0.1em !important;
    padding-left: 0.4em !important;
    padding-right: 0.4em !important;
    text-transform: uppercase !important;
}
._hintLabel._hintFocus {
    font: bold 13pt monospace !important;
}
._hintElem {
    background-color: #ffcc66 !important;
    color: #2d2d2d !important;
}
._hintElem._hintFocus {
    background-color: #6699cc !important;
    color: #2d2d2d !important;
}
```

## disable scrollbars
{:#no-scrollbar}

The webkit scrollbars in the main view can be disabled by user style sheet
_~/.config/vimb/style.css_
```css
body::-webkit-scrollbar {
    display: none;
}
```
[jsqsa]:    http://mdn.beonex.com/en/DOM/document.querySelectorAll.html
[tabbed]:   http://tools.suckless.org/tabbed/
[xdotool]:  http://www.semicomplete.com/projects/xdotool/
[simplyread]: https://njw.name/simplyread/
