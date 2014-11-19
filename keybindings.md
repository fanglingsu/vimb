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
{:#n_:}

gi
: Set cursor to the first editable element in the page and switch to Input
  Mode.
{:#n_gi}

CTRL-Z
: Switch vimb into Pass-Through Mode.
{:#n_CTRL-Z}

gf
: Toggle show html source of current page.
{:#n_gf}

gF
: Opend the Web Inspector for current page.
{:#n_gF}

CTRL-V
: Pass the next key press directly to GTK.
{:#n_CTRL-V}

CTRL-Q
: Quit the browser.
{:#n_CTRL-Q}

### navigation

o
: Start Command Mode and print ':open ' to input box.
{:#n_o}

O
: Start Command Mode and print ':open URI' to input box.
{:#n_O}

t
: Start Command Mode and print ':tabopen ' to input box.
{:#n_t}

T
: Start Command Mode and print ':tabopen URI' to input box.
{:#n_T}

gh
: Open the configured home-page.
{:#n_gh}

gH
: Open the configured home-page into new window.
{:#n_gH}

u
: Open the last closed page.
{:#n_u}

U
: Open the last closed page into a new window.
{:#n_U}

CTRL-P
: Open the oldest entry from read it later queue in current browser window, if
  vimb has been compiled with QUEUE feature.
{:#n_CTRL-P}

[*"x*]p
: Open the URI out of the register *x* or if not given from clipboard.
{:#n_p}

[*"x*]P
: Open the URI out of the register *x* or if not given from clipboard into new
  window.
{:#n_P}

[*N*]UP
: Go back *N* steps in the browser history.
{:#n_UP}

[*N*]DOWN
: Go forward *N* steps in the browser history.
{:#n_DOWN}

[*N*]gu
: Go to the *N*th descendent directory of the current opened URL.
{:#n_gu}

gU
: Go to the domain of the current opened page.
{:#n_gU}

[*N*]CTRL-A
: Increments the last number in URL by 1, or by *N* if
  given.
{:#n_CTRL-A}

[*N*]CTRL-X
: Decrements the last number in URL by 1, or by *N* if given. Negative numbers
  are not supported as trailing numbers in URLs are often preceded by hyphens.
{:#n_CTRL-X}

r
: Reload the website.
{:#n_r}

R
: Reload the website without using caches.
{:#n_R}

CTRL-C
: Stop loading the current page.
{:#n_CTRL-C}

### motion

[*N*]CTRL-F
: Scroll *N* pages down.
{:#n_CTRL-F}

[*N*]CTRL-B
: Scroll *N* pages up.
{:#n_CTRL-B}

[*N*]CTRL-D
: Scroll *N* half pages down.
{:#n_CTRL-D}

[*N*]CTRL-U
: Scroll *N* half pages up.
{:#n_CTRL-U}

[*N*]gg
: Scroll to the top of the current page. Or if N is given to *N*% of the page.
{:#n_gg}

[*N*]G
: Scroll to the bottom of the current page. Or if *N* is given to *N*% of the page.
{:#n_G}

[*N*]0
: Scroll *N* steps to the left of current page.
{:#n_0}

[*N*]$
: Scroll *N* steps to the right of current page.
{:#n_dollar}

[*N*]h
: Scroll *N* steps to the left of page.
{:#n_h}

[*N*]l
: Scroll *N* steps to the right of page.
{:#n_l}

[*N*]j
: Scroll page *N* steps down.
{:#n_j}

[*N*]k
: Scroll page *N* steps up.
{:#n_k}

[*N*]]]
: Follow the last *N*th link matching `nextpattern`.
{:#n_]]}

[*N*][[
: Follow the last *N*th link matching `previouspattern`.
{:#n_[[}

m{a-z}
: Set a page mark `{a-z}` at current possition on page. Such set marks are only
  available on the current page, if the page is left, all marks will be
  removed.
{:#n_m}

'{a-z}
: Jump to the mark `{a-z}` on current page.
{:#n_quote}

''
: Jumps to the position before the latest jump, or where the last `m'` command
  was given.
{:#n_doublequote}

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

["x];y
: Yank hint's destination location into primary and secondary clipboard and
  into register `x`.

["x];Y
: Yank hint's text description or form text into primary and secondary
  clipboard and into register `x`.

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
{:#i_ESC}

CTRL-O
: Executes the next command as normal mode command and return to input mode.
{:#i_CTRL-O}

CTRL-T
: Open configured editor with content of current form field.
{:#i_CTRL-T}

CTRL-V
: Pass the next key press directly to GTK.
{:#i_CTRL-V}

CTRL-Z
: Enter the pass-through mode.
{:#i_CTRL-Z}

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
