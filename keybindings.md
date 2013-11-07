---
title:  vimb - keybindings
layout: default
meta:   vimb browser keybindings for normal and command mode
active: keybindings
---

# vimb keybindings

Some of the Normal Model Commands can have a numeric count to multiply the
effect of the command. If a command supports the count this is shown as `[N]`.

## normal mode

### general

\:
: Start Command Mode and print ':' to the input box.

g-i
: Set cursor to the first editable element in the page and switch to Input
  Mode.

CTRL-Z
: Switch vimb into Pass-Through Mode.

g-f
: Toggle show html source of current page.

g-F
: Opend the Web Inspector for current page.

CTRL-V
: Pass the next key press directly to GTK.

CTRL-Q
: Quit the browser.

### navigation

o
: Start Command Mode and print ':open ' to input box.

O
: Start Command Mode and print ':open URI' to input box.

t
: Start Command Mode and print ':tabopen ' to input box.

T
: Start Command Mode and print ':tabopen URI' to input box.

g-h
: Open the configured home-page.

g-H
: Open the configured home-page into new window.

u
: Open the last closed page.

U
: Open the last closed page into a new window.

CTRL-P
: Open the oldest entry from read it later queue in current browser window, if
: vimb has been compiled with QUEUE feature.

p
: Open the url out of the clipboard.

P
: Open the url out of the clipboard into new window.

[N]UP
: Go back N steps in the browser history.

[N]DOWN
: Go forward N steps in the browser history.

[N]g-u
: Go to the Nth descendent directory of the current opened URL.

g-U
: Go to the domain of the current opened page.

r
: Reload the website.

R
: Reload the website without using caches.

CTRL-C
: Stop loading the current page.

### motion

[N]CTRL-F
: Scroll N pages down.

[N]CTRL-B
: Scroll N pages up.

[N]CTRL-D
: Scroll N half pages down.

[N]CTRL-U
: Scroll N half pages up.

[N]g-g
: Scroll to the top of the current page. Or if N is given to N% of the page.

[N]G
: Scroll to the bottom of the current page. Or if N is given to N% of the page.

[N]0
: Scroll N steps to the left of current page.

[N]$
: Scroll N steps to the right of current page.

[N]h
: Scroll N steps to the left of page.

[N]l
: Scroll N steps to the right of page.

[N]j
: Scroll page N steps down.

[N]k
: Scroll page N steps up.

[N]]-]
: Follow the last Nth link matching `nextpattern`.

[N][-[
: Follow the last Nth link matching `previouspattern`.

### hinting
{:#hinting}

The hinting is the way to do what you would do with the mouse in common
mouse-driven browsers. Open url, yank uri, save page and so on. If the hinting
is started, the relevant elements on the page will be marked by numbered
labels. Hints can be selected by using `<Tab>`, `<C-I>` or `<C-Tab>`, `<C-O>`,
by typing the number of the label, or filtering the elements by some text that
is part of the hinted element (like url, link text, button label) and any
combination of this methods. If <enter> is pressed, the current active hint
will be fired. If only one possible hint remains, this will be fired
automatically.

Syntax: *;{mode}{hint}*

Start hint mode. Different elements depending on mode are highlighted and
numbered. Elements can be selected either by typing their number, or by typing
part of their text (hint) to narrow down the result. When an element has been
selected, it is automatically clicked or used (depending on mode) and hint mode
ends. Following keys have special meanings in Hints mode:

\<CR\>
: Selects the first highlighted element, or the current focused.

\<Tab\>
: Moves the focus to the next hint element.

\<S-Tab\>
: Moves the focus to the previous hint element.

\<Esc\>, CTRL-C, CTRL-[
: Exits Hints mode without selecting an element

f
: Is an alias for the ;o hint mode.

F
: Is an alias for the ;t hint mode.

;-o
: Open hint's location in the current window.

;-t
: Open hint's location in a new window.

;-s
: Saves the hint's destination under the configured `download-path`.

;-O
: Generate an ':open ' prompt with hint's URL.

;-T
: Generate an ':tabopen ' prompt with hint's URL.

;-e
: Open the configured editor (`editor-command`) with the hinted form element's
  content. If the file in editor is saved and the editor is closed, the file
  content will be put back in the form field.

;-i
: Open hinted image into current window.

;-I
: Open hinted image into new window.

;-p
: Push the hint's URL to the end of the read it later queue like the ':qpush'
  command. This is only available if vimb was compiled with QUEUE feature.

;-P
: Push the hint's URL to the end of the read it later queue like the ':qpush'
  command. This is only available if vimb was compiled with QUEUE feature.

;-y
: Yank hint's destination location into primary and secondary clipboard.

### searching

/QUERY, ?QUERY
: Start searching for QUERY in the current page. '/' start search forward, '?'
  in backward direction.

\*, \#
: Start searching for the current selected text, or if no text is selected for
  the content of the primary or secondary clipboard. `*` start the search in
  forward direction and `#` in backward direction.

  Note that this commands will yank the text selection into the clipboard and
  may remove other content from there!

[N]n
: Search for Nth next search result depending on current search direction.

[N]N
: Search for Nth previous search result depending on current search
  direction.

### zooming

[N]z-i
: Zoom-In the text of the page by N steps.

[N]z-o
: Zoom-Out the text of the page by N steps.

[N]z-I
: Full-Content Zoom-In the page by N steps.

[N]z-O
: Full-Content Zoom-Out the page by N steps.

z-z
: Reset Zoom.

### yank

y
: Yank the URI or current page into clipboard.

Y
: Yank the current selection into clipboard.

## command mode

### command line editing

\<Esc\>, CTRL-[, CTRL-C
: Ignore all typed content and switch back to normal mode.

\<CR\>
: Submit the entered ex command or search query to run it.

CTRL-H
: Deletes the char before the cursor.

CTRL-W
: Deletes the last word before the cursor.

CTRL-U
: Remove everything between cursor and prompt.

CTRL-B
: Moves the cursor direct behind the prompt ':'.

CTRL-E
: Moves the cursor after the char in inputbox.

CTRL-V
: Pass the next key press directly to GTK.

### command line history
{:#history}

\<Tab\>
: Start completion of the content in inputbox in forward direction.

\<S-Tab\>
: Start completion of the content in inputbox in backward direction.

\<Up\>
: Step backward in the command history.

\<Down\>
: Step forward in the command history.

## input mode

\<Esc\>, CTRL-[
: Switch back to normal mode.

CTRL-T
: Open configured editor with content of current form field.

CTRL-Z
: Enter the pass-through mode.
