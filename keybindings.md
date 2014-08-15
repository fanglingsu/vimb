---
title:  vimb - keybindings
layout: default
meta:   vimb browser keybindings for normal and command mode
active: keybindings
---

# vimb keybindings

Some of the Normal Model Commands can have a numeric count to multiply the
effect of the command. If a command supports the count this is shown as [*N*].

## normal mode

### general

\:
: Start Command Mode and print ':' to the input box.

gi
: Set cursor to the first editable element in the page and switch to Input
  Mode.

CTRL-Z
: Switch vimb into Pass-Through Mode.

gf
: Toggle show html source of current page.

gF
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

gh
: Open the configured home-page.

gH
: Open the configured home-page into new window.

u
: Open the last closed page.

U
: Open the last closed page into a new window.

CTRL-P
: Open the oldest entry from read it later queue in current browser window, if
: vimb has been compiled with QUEUE feature.

[*"x*]p
: Open the URI out of the register *x* or if not given from clipboard.

[*"x*]P
: Open the URI out of the register *x* or if not given from clipboard into new
  window.

[*N*]UP
: Go back *N* steps in the browser history.

[*N*]DOWN
: Go forward *N* steps in the browser history.

[*N*]gu
: Go to the *N*th descendent directory of the current opened URL.

gU
: Go to the domain of the current opened page.

[*N*]CTRL-A
: Increments the last number in URL by 1, or by *N* if given.

[*N*]CTRL-X
: Decrements the last number in URL by 1, or by *N* if given. Negative numbers
  are not supported as trailing numbers in URLs are often preceded by hyphens.

r
: Reload the website.

R
: Reload the website without using caches.

CTRL-C
: Stop loading the current page.

### motion

[*N*]CTRL-F
: Scroll *N* pages down.

[*N*]CTRL-B
: Scroll *N* pages up.

[*N*]CTRL-D
: Scroll *N* half pages down.

[*N*]CTRL-U
: Scroll *N* half pages up.

[*N*]gg
: Scroll to the top of the current page. Or if N is given to *N*% of the page.

[*N*]G
: Scroll to the bottom of the current page. Or if *N* is given to *N*% of the page.

[*N*]0
: Scroll *N* steps to the left of current page.

[*N*]$
: Scroll *N* steps to the right of current page.

[*N*]h
: Scroll *N* steps to the left of page.

[*N*]l
: Scroll *N* steps to the right of page.

[*N*]j
: Scroll page *N* steps down.

[*N*]k
: Scroll page *N* steps up.

[*N*]]]
: Follow the last *N*th link matching `nextpattern`.

[*N*][[
: Follow the last *N*th link matching `previouspattern`.

m{a-z}
: Set a page mark `{a-z}` at current possition on page. Such set marks are only
  available on the current page, if the page is left, all marks will be
  removed.

'{a-z}
: Jump to the mark `{a-z}` on current page.

''
: Jumps to the position before the latest jump, or where the last `m'` command
  was given.

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

;o
: Open hint's location in the current window.

;t
: Open hint's location in a new window.

;s
: Saves the hint's destination under the configured `download-path`.

;O
: Generate an ':open ' prompt with hint's URL.

;T
: Generate an ':tabopen ' prompt with hint's URL.

;e
: Open the configured editor (`editor-command`) with the hinted form element's
  content. If the file in editor is saved and the editor is closed, the file
  content will be put back in the form field.

;i
: Open hinted image into current window.

;I
: Open hinted image into new window.

;p
: Push the hint's URL to the end of the read it later queue like the ':qpush'
  command. This is only available if vimb was compiled with QUEUE feature.

;P
: Push the hint's URL to the end of the read it later queue like the ':qpush'
  command. This is only available if vimb was compiled with QUEUE feature.

;x
: Hints like `;o` but instead of opening the hinted URI, the configured
  `x-hint-command` is run in vimb.

;y
: Yank hint's destination location into primary and secondary clipboard.

;Y
: Yank hint's text description or form text into primary and secondary
  clipboard.

Syntax: *g;{mode}{hint}*

Start an extended hints mode and stay there until `<Esc>` is pressed. Like the
normal hinting except that after a hint is selected, hints remain visible so
that another one can be selected with the same action as the first. Note that
the extended hint mode can only be combined with the following hint modes `I`
`p` `P` `s` `t` `y` `Y`.

### searching

/QUERY, ?QUERY
: Start searching for `QUERY` in the current page. '/' start search forward,
  '?' in backward direction.

\*, \#
: Start searching for the current selected text, or if no text is selected for
  the content of the primary or secondary clipboard. `*` start the search in
  forward direction and `#` in backward direction.

  Note that this commands will yank the text selection into the clipboard and
  may remove other content from there!

[*N*]n
: Search for *N*th next search result depending on current search direction.

[*N*]N
: Search for *N*th previous search result depending on current search
  direction.

### zooming

[*N*]zi
: Zoom-In the text of the page by *N* steps.

[*N*]zo
: Zoom-Out the text of the page by *N* steps.

[*N*]zI
: Full-Content Zoom-In the page by *N* steps.

[*N*]zO
: Full-Content Zoom-Out the page by *N* steps.

zz
: Reset Zoom.

### yank

[*"x*]y
: Yank the URI or current page into register *x* and clipboard.

[*"x*]Y
: Yank the current selection into register *x* and clipboard.

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

CTRL-R {a-z%:/;}
: Insert the content of given register at cursor position. See also section
  [registers](#registers)

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

CTRL-O
: Executes the next command as normal mode command and return to input mode.

CTRL-T
: Open configured editor with content of current form field.

CTRL-V
: Pass the next key press directly to GTK.

CTRL-Z
: Enter the pass-through mode.

## registers
{:#registers}

There are different types of registers.

"a - "z
: 26 named registers "a to "z. Vimb fills these registers only when you say
  so.

"%
: Contains the curent opened URI.

":
: Contains the most recent executed ex command.

"/
: Contains the most recent search-pattern.

";
: Contains the last hinted URL. This can be used in `x-hint-command` to get
  the URL of the hint.
