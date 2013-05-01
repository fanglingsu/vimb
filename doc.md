---
title:  vimb - documentation
layout: default
meta:   vimb documentation of the features
---

# vimb documentation

## add and remove keybindings

There are already keybindings defined in vimb, but it's easy to add new
keybindings and to remove not used bindings.

To bind a command to a key sequence use the `{n,i,c}map` command. To map a
key-sequence to a command, use this format `:nmap {[modkey]key}={command}[
params]`.

The modkey is a single simple char like "g". The key can also contain special
keys and modifiers and is given in format like `<ctrl-o>`, `<tab>`,
`<shift-tab>`, `<up>`, `<right>` or also a simple char like "G".

If a keybinding is added, for the same key-sequence like another keybinding,
the later added have precedence. If a keybinding is removed only that with the
highest precedence will be removed and all other will still be active.

n(un)map
: Add (Remove) a keybinding used in Normal Mode.

i(un)map
: Add (Remove) a keybinding used in Input Mode.

c(un)map
: Add (Remove) a keybinding used in Command Mode.

## default keybindings
{:#default-keys}

### normal mode

 :
: Start command mode and print `:' to the input box.

/
: Start command mode and print `/' to inputbox to start searching forward.

?
: Start command mode and print `?' to inputbox to start searching backward.

o
: Start command mode and print `:open ' to input box.

O
: Start command mode and print `:open CURRENT_URI' to input box.

t
: Start command mode and print `:tabopen ' to input box.

T
: Start command mode and print `:tabopen CURRENT_URI' to input box.

---

g-h
: Opend the configured home-page.

g-H
: Opend the configured home-page into new window.

u
: Open the last closed page.

U
: Open the last closed page into a new window.

d
: Quit the browser.

[N]ctrl-o
: Go back N steps in the browser history.

[N]ctrl-i
: Go forward N steps in the browser history.

r
: Reload the website.

R
: Reload the website witout using caches.

C
: Stop loading the current page.

---

[N]ctrl-f
: Scroll N pages down.

[N]ctrl-b
: Scroll N pages up.

[N]ctrl-d
: Scroll N half pages down.

[N]ctrl-u
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

---

f
: Start hinting for links.

F
: Start hinting for links to open them in new window.

;-o
: Start hinting to build :open command with hinted source.

;-t
: Start hinting to build :tabopen command with hinted source.

;-y
: Start hinting to yank hinted element URI into clipboard.

;-i
: Start hinting to open images.

;-I
: Start hinting to open images into new window.

;-e
: Start hinting to open editable form fileds with external editor.

---

y
: Yank the URI or current page into clipboard.

Y
: Yank the current selection into clipboard.

p
: Open the url out of the clipboard.

P
: Open the url out of the clipboard into new window.

---

[N]z-i
: Zoom-In the text of the page by N steps.

[N]z-o
: Zoom-Out the text of the page by N steps.

[N]z-I
: Fullcontent Zoom-In the page by N steps.

[N]z-O
: Fullcontent Zoom-Out the page by N steps.

---

z-z
: Reset Zoom.

[N]n
: Search for Nnth next search result.

[N]N
: Search for Nnth previous search result.

---

ctrl-n
: Hint to the next element.

ctrl-p
: Hint the previous element.

---

g-f
: Toggle show html source of current page.

g-F
: Opend the Web Inspector for current page.

### command mode

tab
: Complete different sources in the inputbox.

shift-tab
: Complete backward different sources in the inputbox.

---
up
: Step through history backward.

down
: Step through history forward.

### insert mode

ctrl-t
: If the current active form element is an inputbox or textarea, the content is
  copied to temporary file and the file openen with the configured external
  editor.
