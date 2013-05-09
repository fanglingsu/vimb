---
title:  vimb - commands
layout: default
meta:   list of commands available in vimb browser
---

# vimb commands
Commands are a central part in vimb. They are used for nearly all things that
could be done with this browser. Commands allow to set config variables, to
assign keybindings and much more. Also the keybindings are only shortcut for
the commands itself.

Commands can be called in Insert Mode from the inputbox in the way like
`:[count]command[ param[=value]]`. But some commands are not available in
Command Mode and therefore not callable directly. To use them too, they must be
bound to the keybinding.

## open
open [URI]
: Open the give URI into current window. If URI is empty the configured
  `home-page` is opened.

tabopen [URI]
: Open the give URI into a new window. If URI is empty the configured
  `home-page` is opened.

(tab)open-closed
: Open the last closed page.

(tab)open-clipboard
: Open the url from clipboard.

## input

Switches the browser into Command Mode and prefill the inputbox on th bottom of
the browser with various prefilled content.

input [TEXT]
: Writes TEXT into to inputbox and switch to command mode. If no TEXT is given,
  print `:` into the inputbox.

inputuri [TEXT]
: Writes TEXT{URI} into to inputbox where {URL} is the current used url. This
  can be used to add keybindings to generate the tabopen command with current uri
  prefilled. If TEXT is not given use `:` instead.

## navigate

Following commands are used to navigate within the browser history.

[N]back
: Move N steps back in browser history.

[N]forward
: Move N steps back in browser history.

reload
: Reload the current viewed url.

reload!
: Discard any caches and reload the current viewed url.

stop
: Stop loading the current url.

## scroll

Following commands are used to scroll ad jump within the current view.

[N]jumpleft
: Scrolls the current view N steps to the left. If N is given jump to N% of the
  page from the left.

[N]jumpright
: Scrolls the current view to the right. If N is given jump to N% of the page
  from the left.

[N]jumptop
: Scrolls the current view to the top of page. If N is given, scroll to N% of
  the page.

[N]jumpbottom
: Scrolls the current view to the end of page. If N is given, scroll to N% of
  the page.

[N]pageup
: Scroll up the page N screens.

[N]pagedown
: Scroll down the page N screens.

[N]halfpageup
: Scroll up the page N half screens.

[N]halfpagedown
: Scroll down the page N half screens.

[N]scrollleft
: Scroll the page N times the `scrollstep` to the left.

[N]scrollright
: Scroll the page N times the `scrollstep` to the right.

[N]scrollup
4 Scroll the page N times the `scrollstep` to the top.

[N]scrolldown
: Scroll the page N times the `scrollstep` to the end.

## keybinding

nmap
: Add a keybinding used in Normal Mode.

imap
: Add a keybinding used in Input Mode.

cmap
: Add a keybinding used in Command Mode.

nunmap
: Remove a Normal Mode keybinding.

iunmap
: Remove a Input Mode keybinding.

cunmap
: Remove a Command Mode keybinding.

More about the [keybindings][].

## hints

hint-link [PREFIX]
hint-link-new [PREFIX]
: Start hinting to open link into current or new window. If PREFIX is given,
  print this into the inputbox, default `.` and `,`.

hint-input-open [PREFIX]
hint-input-tabopen [PREFIX]
: Start hinting to fill the inputbox with `:open {hintedLinkUrl}` or `:tabopen
  {hintedLinkUrl}`. If PREFIX is given, print this into the inputbox, default
  `;o` and `;t`.

hint-yank [PREFIX]
: Start hinting to yank the hinted link url into the primary and secondary
  clipboard. If PREFIX is given, print this into the inputbox, default `;y`.

hint-image-open [PREFIX]
hint-image-tabopen [PREFIX]
: Start hinting to open images into current or new window. If PREFIX is given,
  print this into the inputbox, default `;i` and `;I`.

hint-editor [PREFIX]
: Start hinting to open inputboxes or textareas with external editor. If PREFIX
  is given, print this into the inputbox, default `;e`.

hint-focus-nex
hint-focus-prev
: Focus next or previous hint.

## yank

yank-uri
: Yank the current url to the primary and secondary clipboard.

yank-selection
: Yank the selected text into the primary and secondary clipboard.

## shortcuts

Shortcuts allows to open URL build up from a named template with additional
parameters. If a shortcut named `dd` is defined, you can use it with `:open dd
list of parameters` to open the generated URL.

Shortcuts are a good to use with search engines where the URL is nearly the
same but a single parameter is user defined.

shortcut-add SHORTCUT=URI
: Adds a shortcut with the SHORTCUT and URI template. The URI can contain
  multiple placeholders $0-$9 that will be filled by the parameters given when
  the shortcut is called. The parameters given when the shortcut is
  called will be split into as many parameters like the highest used placeholder.

  Example 1: `shortcut-add dl=https://duckduckgo.com/lite/?q=$0` to setup a search
  engine. Can be called by `:open dl my search phrase`.

  Example 2: `shortcut-add gh=https://github.com/$0/$1` to build urls from given
  parameters. Can be called `:open gh fanglingsu vimb`.

shortcut-remove SHORTCUT
: Remove the search engine to the given SHORTCUT.

shortcut-default SHORTCUT
: Set the shortcut for given SHORTCUT as the default. It doesn't matter if the
  SHORTCUT is already in use or not to be able to set it.

## configuration

set VAR=VALUE
: Set configuration values named by VAR. To set boolean variable you should use
  `on`, `off` or `true` and `false`. Colors are given as hexadecimal value like
  `#f57700`.

set VAR?
: Show the current set value of variable VAR.

set VAR!
: Toggle the value of boolean variable VAR and display the new set value.

## zoom

[N]zoomin
[N]zoomout
: Zoom N steps in or out of the current page - effects only the text.

[N]zoominfull
[N]zoomoutfull
: Zoom N steps in or out of the current page - effecting all elements.

zoomreset
: Reset the zoomlevel to the default value.

## history

hist-prev
hist-next
: Prints the previous or next cammand or search query from history into
  inputbox. If there is already text in the input box this will be used to get
  history items. A command is not a internal command, but every string entered
  into inputbox that begins with [:/?]. So the history contains real commands and
  search queries.

## bookmark

bookmark-add [TAGS]
: Save the current opened uri with TAGS to the bookmark file.

## misc
run [COMMAND LIST]
: Run is a command, that was introduced to have the ability to run multiple
  other commands with a single call. Everything after the `run` is interpreted as
  a `|` seperated list of commands and parameters. The run command
  allows to use fancy keybindings that set several config settings with only on keypress.

  Format: `:run [count]command[ param[=value]]|[count]command[ param[=value]]|...`

  Example: `:run set input-bg-normal=#ff0 | set input-fg-normal=#f0f | 5pagedown`

[N]search-forward
[N]search-backward
: Search in current page forward or backward.

inspect
: Toggles the webinspector for current page. This is only available if the
  config `webinspector` is enabled.

quit
: Close the browser.

source
: Toggle between normal view and source view for the current page.

eval JAVASCRIPT
: Runs the given JAVASCRIPT in the current page and display the evaluated value.

  Example: `:eval document.cookie`


[keybindings]: keybindings.html
