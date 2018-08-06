---
title:  Vimb - manual page
layout: default
meta:   The whole manual page
active: man
---

# vimb - manual page

* toc
{:toc}

## SYNOPSIS
{:#SYNOPSIS}

**vimb** `[OPTIONS] [URI|file|-]`

## DESCRIPTION
{:#DESCRIPTION}

Vimb is a WebKit based web browser that behaves like the Vimperator plugin for
Firefox and has usage paradigms from the great editor, Vim. The goal of Vimb
is to build a completely keyboard-driven, efficient and pleasurable
browsing-experience.

## OPTIONS
{:#OPTIONS}

If no *URI* or *file* is given vimb will open the configured home-page. If
*URI* is `-`, vimb reads the html to display from stdin.

Mandatory arguments to long options are mandatory for short options too.

−c, −−config *FILE*
: Use custom configuration given as *CONFIG-FILE*. This will also be applied
  on new spawned instances.

−e, −−embed *WINID*
: *WINID* of an XEmbed-aware application, that vimb will use as its parent.

−h, −−help
: Show help options.

−p, −−profile *PROFILE-NAME*
: Create or open specified configuration profile. Configuration data for the
  profile is stored in a directory named *PROFILE-NAME* under default
  directory for configuration data.

−v, −−version
: Print build and version information and then quit.

−−bug-info
: Prints information about used libraries for bug reports and then quit.

## MODES
{:#MODES}

Vimb is modal an has following main modes:

Normal Mode
: The default mode. Pressing Escape always enter normal mode.

Input Mode
: Used for editing text elements in a webpage.

Command Mode
: Execute ex-commands from the builtin inputbox (commandline).

Pass-Through Mode
: In Pass-Through mode only the \<Esc\> and \<C-[\> keybindings are
  interpreted by vimb, all other keystrokes are given to the webview to handle
  them. This allows to use websites that uses keybindings itself, that might
  be swallowed by vimb else.

## NORMAL MODE
{:#NORMAL_MODE}

Some of the Normal Model Commands can have a numeric count to multiply the
effect of the command. If a command supports the count this is shown as
[*N*].

### General
{:#General}

\:
: Start Command Mode and print `:` to the input box.

gi
: Set cursor to the first editable element in the page and switch to Input Mode.

CTRL−Z
: Switch vimb into Pass-Through Mode.

gF
: Open the Web Inspector for current page.

CTRL−V
: Pass the next key press directly to webkit or gtk.

CTRL−Q
: Quit the browser if there are no running downloads.

### Navigation
{:#Navigation}

o
: Start Command Mode and print `:open ` to input box.

O
: Start Command Mode and print `:open URI` to input box.

t
: Start Command Mode and print `:tabopen ` to input box.

T
: Start Command Mode and print `:tabopen URI` to input box.

gh
: Open the configured home-page.

gH
: Open the configured home-page into new window.

u
: Open the last closed page.

U
: Open the last closed page into a new window.

CTRL−P
: Open the oldest entry from read it later queue in current browser window, if
  vimb has been compiled with QUEUE feature.

[*"x*]p
: Open the URI out of the register *x* or if not given from clipboard.

[*"x*]P
: Open the URI out of the register *x* or if not given from clipboard into new
  window.

[*N*]CTRL−O
: Go back *N* steps in the browser history.

[*N*]CTRL−I
: Go forward *N* steps in the browser history.

[*N*]gu
: Go to the *N*th descendent directory of the current opened URI.

gU
: Go to the domain of the current opened page.

r
: Reload the website.

R
: Reload the website without using caches.

CTRL−C
: Stop loading the current page.

### Motion
{:#Motion}

[*N*]CTRL−F
: Scroll *N* pages down.

[*N*]CTRL−B
: Scroll *N* pages up.

[*N*]CTRL−D
: Scroll *N* half pages down.

[*N*]CTRL−U
: Scroll *N* half pages up.

[*N*]gg
: Scroll to the top of the current page. Or if *N* is given to *N*% of the page.

[*N*]G
: Scroll to the bottom of the current page. Or if *N* is given to *N*% of the
  page.

0, ^
: Scroll to the absolute left of the document. Unlike in Vim, 0 and ^ work
  exactly the same way.

$
: Scroll to the absolute right of the document.

[*N*]h
: Scroll *N* steps to the left of page.

[*N*]l
: Scroll *N* steps to the right of page.

[*N*]j
: Scroll page *N* steps down.

[*N*]k
: Scroll page *N* steps up.

### Hinting
{:#Hinting}

The hinting is the way to do what you would do with the mouse in common mouse-
driven browsers. Open URI, yank URI, save page and so on. If the hinting is
started, the relevant elements on the page will be marked by labels generated
from configured 'hintkeys'. Hints can be selected by using \<Tab\>, \<C-I\> or
\<C-Tab\>, \<C-O\>, by typing the chars of the label, or filtering the elements by
some text that is part of the hinted element (like URI, link text, button
label) and any combination of this methods. If \<enter\> is pressed, the current
active hint will be fired. If only one possible hint remains, this will be
fired automatically.

**Syntax**: `;{mode}{hint}`

Start hint mode. Different elements depending on *mode* are highlighted and
'numbered'. Elements can be selected either by typing their label, or by
typing part of their text (*hint*) to narrow down the result. When an
element has been selected, it is automatically clicked or used (depending on
*mode*) and hint mode ends.

The filtering of hints by text splits the query at ' ' and use the single
parts as separate queries to filter the hints. This is useful for hints that
have a lot of filterable chars in common and it needs many chars to make a
distinct selection. For example `;over tw` will easily select the second
hint out of `{'very long link text one', 'very long link text two'}`.
Following keys have special meanings in Hints mode:

\<CR\>
: Selects the first highlighted element, or the current focused.

\<Tab\>
: Moves the focus to the next hint element.

\<S-Tab\>
: Moves the focus to the previous hint element.

\<Esc\>, CTRL−C, CTRL−[
: Exits Hints mode without selecting an element

#### Hint modes:
{:#Hint_modes}

f
: Is an alias for the `;o` hint mode.

F
: Is an alias for the `;t` hint mode.

;o
: Open hint's location in the current window.

;t
: Open hint's location in a new window.

;s
: Saves the hint's destination under the configured 'download-path'.

;O
: Generate an `:open` prompt with hint's URI.

;T
: Generate an `:tabopen` prompt with hint's URI.

;e
: Open the configured editor ('editor-command') with the hinted form element's
  content. If the file in editor is saved and the editor is closed, the file
  content will be put back in the form field.

;i
: Open hinted image into current window.

;I
: Open hinted image into new window.

;p
: Push the hint's URI to the end of the read it later queue like the `:qpush`
  command. This is only available if vimb was compiled with QUEUE feature.

;P
: Push the hint's URI to the beginning of the read it later queue like the
  `:qunshift` command. This is only available if vimb was compiled with QUEUE
  feature.

;x
: Hints like ;o, but instead of opening the hinted URI, the 'x-hint-command' is
  run in vimb.

[*"x*];y
: Yank hint's destination location into primary and secondary clipboard and into
  the register *x*.

[*"x*];Y
: Yank hint's text description or form text into primary and secondary clipboard
  and into the register *x*.

**Syntax**: `g;{mode}{hint}`

Start an extended hints mode and stay there until \<Esc\> is pressed. Like the
normal hinting except that after a hint is selected, hints remain visible so
that another one can be selected with the same action as the first. Note that
the extended hint mode can only be combined with the following hint modes
*I p P s t y Y*.

#### Motion
{:#hint_mode_motion}

Motions commands are like those for normal mode except that CTRL is used as
modifier. But they can not be used together with a count.

CTRL-F 
: Scroll one page down.

CTRL-B
: Scroll one page up.

CTRL-D
: Scroll half page down.

CTRL-U
: Scroll half page up.

CTRL-J
: Scroll one step down.

CTRL-K
: Scroll one step up.

### Searching
{:#Searching}

/QUERY, ?QUERY
: Start searching for `QUERY` in the current page. `/` start search forward,
  `?` in backward direction.

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

\<CR\>
: Perform a click on element containing the current highlighted search result.

### Zooming
{:#Zooming}

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

### Yank
{:#Yank}

[*"x*]y
: Yank the URI or current page into register *x* and clipboard.

[*"x*]Y
: Yank the current selection into register *x* and clipboard.

## COMMAND MODE
{:#COMMAND_MODE}

Commands that are listed below are ex-commands like in vim, that are typed
into the inputbox (the command line of vimb). The commands may vary in their
syntax or in the parts they allow, but in general they follow a simple syntax.

**Syntax:** `:[:| ][N]cmd[name][!][ lhs][ rhs]`

Where *lhs* (left hand side) must not contain any unescaped space. The syntax
of the rhs (right hand side) if this is available depends on the command. At
the moment the count parts [N] of commands is parsed, but actual there does
not exists any command that uses the count.
Commands that are typed interactive (from inputbox or from socket) are
normally recorded into command history and register. To avoid this, the
commands can be prefixed by one or more additional `:` or whitespace.

Multiple commands, separated by a `|` can be given in a single command line
and will be executed consecutively. The pipe can be included as an argument to
a command by escaping it with a backslash.
Following commands process the entire command-line string literally. These
commands will include any `|` as part of their argument string and so can not
be followed by another command.

- autocmd
- cmap, cnoremap, imap, inoremap, nmap, nnoremap
- eval
- normal
- open, tabopen
- shellcmd

### Command Line Editing
{:#Command_Line_Editing}

\<Esc\>, CTRL−[, CTRL-C
: Ignore all typed content and switch back to normal mode.

\<CR\>
: Submit the entered ex command or search query to run it.

CTRL−H
: Deletes the char before the cursor.

CTRL−W
: Deletes the last word before the cursor.

CTRL−U
: Remove everything between cursor and prompt.

CTRL−B
: Moves the cursor direct behind the prompt `:`.

CTRL−E
: Moves the cursor after the char in inputbox.

CTRL−V
: Pass the next key press directly to gtk.

CTRL−R {a-z"%:/;}
: Insert the content of given register at cursor position. See also section
  REGISTERS.

### Command Line History
{:#Command_Line_History}

\<Tab\>
: Start completion of the content in inputbox in forward direction.

\<S-Tab\>
: Start completion of the content in inputbox in backward direction.

\<Up\>
: Step backward in the command history.

\<Down\>
: Step forward in the command history.

### Open
{:#Open}

\:o[pen] [*URI*]
: Open the give *URI* into current window. If *URI* is empty the configured
  'home-page' is opened.

\:t[abopen] [*URI*]
: Open the give *URI* into a new window. If *URI* is empty the configured
  'home-page' is opened.

### Key Mapping
{:#Key_Mapping}

Key mappings allow to alter actions of key presses. Each key mapping is
associated with a mode and only has effect when the mode is active. Following
commands allow the user to substitute one sequence of key presses by another.

**Syntax:** `:{m}map {lhs} {rhs}`

Note that the {lhs} ends with the first found space. If you want to use space
also in the {lhs} you have to escape this with a single ``\`` like shown in the
examples.
The {rhs} starts with the first none space char. If you want a {rhs} that starts
with a space, you have to use `<Space>`.

Standard key mapping commands are provided for these modes *m*:

- **n** Normal mode: When browsing normally.
- **i** Insert mode: When interacting with text fields on a website.
- **c** Command Line mode: When typing into the command line of vimb

Most keys in key sequences are represented simply by the character that you see
on the screen when you type them. However, as a number of these characters have
special meanings, and a number of keys have no visual representation, a special
notation is required.

As special key names have the format `<...>`. Following special keys can be
used `<Left>`, `<Up>`, `<Right>`, `<Down>` for the cursor keys, `<Tab>`,
`<Esc>`, `<CR>`, `<Space>`, `<F1>`-`<F12>` and `<C-A>`-`<C-Z>`.

\:nm[ap] {*lhs*} {*rhs*}
\:im[ap] {*lhs*} {*rhs*}
\:cm[ap] {*lhs*} {*rhs*}
: Map the key sequence *lhs* to *rhs* for the modes where the map command applies.
  The result, including *rhs*, is then further scanned for mappings. This allows
  for nested and recursive use of mappings.

: - `:cmap <C-G>h /home/user/downloads/`
    Adds a keybind to insert a file path into the input box. This could be
    useful for the `:save` command that could be used as `:save ^Gh`.
  - `:nmap <F1> :set scripts=on<CR>:open !glib<Tab><CR>`
    This will enable scripts and lookup the first bookmarked URI with the tag
    'glib' and open it immediately if F1 key is pressed.
  - `:nmap \ \  50G;o`
    Example which mappes two spaces to go to 50% of the page, start hinting
    mode.

\:nn[oremap] {*lhs*} {*rhs*}
\:ino[remap] {*lhs*} {*rhs*}
\:cno[remap] {*lhs*} {*rhs*}
: Map the key sequence *lhs* to *rhs* for the mode where the map command applies.
  Disallow mapping of *rhs*, to avoid nested and recursive mappings. Often used
  to redefine a command.

\:nu[nmap] {*lhs*}
\:iu[nmap] {*lhs*}
\:cu[nmap] {*lhs*}
: Remove the mapping of *lhs* for the applicable mode.

### Bookmarks
{:#Bookmarks}
\:bma [*tags*]
: Save the current opened URI with *tags* to the bookmark file.

\:bmr [*URI*]
: Removes all bookmarks for given *URI* or, if not given, the current opened
  page.

### Handlers
{:#Handlers}

\:handler-add *handler*=*cmd*
: Adds a handler to direct *handler* links to the external *cmd*. The *cmd* can
  contain one placeholder %s that will be filled by the full URI given when the
  command is called.
: Examples:

: - `:handler-add mailto=urxvt -e mutt %s`
    to start email client for mailto links.
  - `:handler-add magnet=xdg-open %s`
    to open magnet links with xdg-open.
  - `:handler-add ftp=urxvt -e wget %s -P ~/ftp-downloads`
    to handle ftp downloads via wget.

\:handler-remove *handler*
: Remove the handler for the given URI *handler*.

### Shortcuts
{:#Shortcuts}

Shortcuts allows to open URL build up from a named template with additional
parameters. If a shortcut named 'dd' is defined, you can use it with
`:open dd list of parameters` to open the generated URL.

Shortcuts are a good to use with search engines where the URL is nearly the
same but a single parameter is user defined.

\:shortcut-add *shortcut=URI*
: Adds a shortcut with the *shortcut* and URI template. The URI can contain
  multiple placeholders `$0-$9` that will be filled by the parameters given when
  the shortcut is called. The parameters given when the shortcut is called
  will be split into as many parameters like the highest used placeholder.

: - `:shortcut-add dl=https://duckduckgo.com/lite/?q=$0`
    to setup a search engine. Can be called by `:open dl my search phrase`.
  - `:shortcut-add gh=https://github.com/$0/$1`
    to build url from given parameters. Can be called `:open gh fanglingsu vimb`.
  - `:shortcut-add map=https://maps.google.com/maps?saddr=$0&daddr=$1`
	to search for a route, all but the last parameter must be quoted if they
	contain spaces like `:open map "city hall, London" railway station, London`

\:shortcut-remove *shortcut*
: Remove the search engine to the given *shortcut*.

\:shortcut-default *shortcut*
: Set the shortcut for given shortcut as the default, that is the *shortcut* to
  be used if no shortcut is given and the string to open is not an URI. It
  doesn't matter if the shortcut is already in use or not to be able to set it.

### Settings
{:#Settings}

\:se[t] *var=value*
: Set configuration values named by *var*. To set boolean variable you should
  use 'on', 'off' or 'true' and 'false'. Colors are given as hexadecimal value
  like '#f57700'.

\:se[t] *var+=value*
: Add the *value* to a number option, or apend the *value* to a string option.
  When the option is a comma separated list, a comma is added, unless the
  value was empty.

\:set[t] *var^=value*
: Multiply the *value* to a number option, or prepend the *value* to a string
  option. When the option is a comma separated list, a comma is added, unless
  the value was empty.

\:se[t] *var-=value*
: Subtract the *value* from a number option, or remove the *value* from a
  string option, if it is there. When the option is a comma separated list, a
  comma is deleted, unless the option becomes empty.

\:se[t] *var*?
: Show the current set value of variable *var*.

\:se[t] *var*!
: Toggle the value of boolean variable *var* and display the new set value.

### Queue
{:#Queue}

The queue allows to mark URLs for later reading (something like a read it later
list). This list is shared between the single instances of vimb. Only available
if vimb has been compiled with QUEUE feature.

\:qpu[sh] [*URI*]
: Push URI or if not given current *URI* to the end of the queue.

\:qu[nshift] [*URI*]
: Push URI or if not given current *URI* to the beginning of the queue.

\:qp[op]
: Open the oldest queue entry in current browser window and remove it from the
  queue.

\:qc[lear]
: Removes all entries from queue.

### Automatic commands
{:#Autocmd}
An autocommand is a command that is executed automatically in response to some
event, such as a URI being opened. Autocommands are very powerful. Use them
with care and they will help you avoid typing many commands.

Autocommands are built with following properties.

*group*
: When the _group_ argument is not given, Vimb uses the current
  group as defined with ':augroup', otherwise, Vimb uses the group defined
  with _group_. Groups are useful to remove multiple grouped autocommands.

*event*
: You can specify a comma separated list of event names. No white space can be
  used in this list.
: Events:
: - `LoadStarted` Fired if a new page is going to opened. No data has been
	received yet, the load may still fail for transport issues. Out of this
    reason this event has no associated URL to match.
  - `LoadCommitted` Fired if first data chunk has arrived, meaning that the
    necessary transport requirements are established, and the load is being
    performed. This is the right event to toggle content related setting like
    'scripts', 'plugins' and such things.
  - `LoadFinished` Fires when everything that was required to display on the
    page has been loaded.
  - `DownloadStart` Fired right before a download is started. This is fired
    for vimb downloads as well as external downloads if
    'down‐load-use-external' is enabled.
  - `DownloadFinished` Fired if a vimb managed download is finished. For
    external download this event is not available.
  - `DownloadFailed` Fired if a vimb managed download failed. For external
    download this event is not available.

*pat*
: Comma separated list of patterns, matches in order to check if a autocommand
  applies to the URI associated to an event. To use '`,`' within the single
  patterns this must be escaped as '`\,`'.
: Patterns:
: - `\*` Matches any sequence of characters. This includes also `/` in contrast
    to shell patterns.
  - `?` Matches any single character except of `/`.
  - `{one,two}` Matches 'one' or 'two'. Any `{`, `,` and `}` within this
    pattern must be escaped by a '`\`'. `*` and `?` have no special meaning
    within the curly braces.
  - `\` Use backslash to escape the special meaning of '`?*{},`' in the pattern
    or pattern list.

*cmd*
: Any ex command vimb understands. The leading `:` is not required. Multiple
  commands can be separated by `|`.

\:au[tocmd] [*group*] {*event*} {*pat*} {*cmd*}
: Add cmd to the list of commands that vimb will execute automatically on event
  for a URI matching pat autocmd-patterns. Vimb always adds the cmd after
  existing autocommands, so that the autocommands are executed in the order in
  which they were given.

\:au[tocmd]! [*group*] {*event*} {*pat*} {*cmd*}
: Remove all autocommands associated with *event* and which pattern match
  *pat*, and add the command *cmd*. Note that the pattern is not matches
  literally to find autocommands to remove, like vim does. Vimb matches
  the autocommand pattern with *pat*.

\:au[tocmd]! [*group*] {*event*} {*pat*}
: Remove all autocommands associated with *event* and which pattern matches
  *pat*.

\:au[tocmd]! [*group*] * {*pat*}
: Remove all autocommands with patterns matching *pat* for all events.

\:au[tocmd]! [*group*] {*event*}
: Remove all autocommands for *event*.

\:au[tocmd]! [*group*]
: Remove all autocommands.

\:aug[roup] {*name*}
: Define the autocmd group *name* for the following `:autocmd` commands. The
  name "end" selects the default group.

\:aug[roup]! {*name*}
: Delete the autocmd group *name*.

Example:

    :aug mygroup
    :  au LoadCommitted * set scripts=off|set cookie-accept=never
    :  au LoadCommitted http{s,}://github.com/*.https://maps.google.de/* set scripts=on
    :  au LoadFinished https://maps.google.de/* set useragent=foo
    :aug end

### Misc
{:#Misc}

\:cl[earcache]
: Clears all resources currently cached by webkit. Note that this effects all
  running instances of vimb.

\:sh[ellcmd] *cmd*
: Runs given shell *cmd* synchronous and print the output into inputbox. The
  *CMD* can contain multiple `%` chars that are expanded to the current opened
  uri. Also the `~/` to home directory expansion is available.
: Runs given shell *cmd* syncronous and print the output into inputbox.
  Follwing pattern in *cmd* are expanded, `~username`, `~/`, `$VAR` and
  `${VAR}`. A '``\``' before these patterns disables the expansion.
: Following environment variables are set for called shell commands.
: - `VIMB_URI` This variable is set by vimb everytime a new page is opened to
    the URI of the page.
  - `VIMB_TITLE` Contains the title of the current opened page.
  - `VIMB_PID` Contains the pid of the running vimb instance.
  - `VIMB_XID` Holds the X-Window id of the vim window or of the embedding
    window if vimb is started with -e option.
: Example: `:sh ls -la $HOME`

:sh[ellcmd]! *cmd*
: Like `:shellcmd` but runs given shell *cmd* asyncron.
: Example: ``:sh! /bin/sh -c 'echo "`date` $VIMB_URI" >> myhistory.txt'``

\:s[ave] [*path*]
: Download current opened page into configured download directory. If *path* is
  given, download under this file name or path. *Path* is expanded and can
  therefore contain `~/`, `${ENV}` and `~user` pattern.

\:so[ource] [*file*]
: Read ex commands from *file*.

\:q[uit]
: Close the browser. This will be refused if there are running downloads.

\:q[uit]!
: Close the browser independent from an running download.

\:reg[ister]
: Display the contents of all registers.
: Register:
: - `"a` -- `"z` 26 named registers. Vimb fills these registers only when you
    say so.
  - `":` Last executed ex command.
  - `""` Last yanked content.
  - `"%` Curent opened URI.
  - `"/` Last search phrase.
  - `";` Last hinted URL. This can be used in `x-hint-command` to get the URL
    of the hint.

\:e[val] *javascript*
: Runs the given *javascript* in the current page and display the evaluated
  value.
: This comman cannot be followed by antoher command, since any `|` is
  considered part of the command.
: Example: `:eval document.cookie`

\:e[val]! *javascript*
: Like `:e[val]`, but there is nothing print to the input box.

\:no[rmal] [*cmds*]
: Execute normal mode commands *cmds*. This makes it possible to execute normal
  mode commands typed on the input box.
: *cmds* cannot start with a space. Put a count of 1 (one) before it, "1 " is one space.
  This comman cannot be followed by antoher command, since any `|` is
  considered part of the command.
: Example: `:set scripts!|no! R` toggle scripts and reload the page.

\:no[rmal]! [*cmds*]
: Like `:no[rmal]`, but no mapping is applied to *cmds*.

\:ha[rdcopy]
: Print current document. Open a GUI dialog where you can select the printer,
  number of copies, orientation, etc.

## INPUT MODE
{:#INPUT_MODE}

\<Esc\>, CTRL−[
: Switch back to normal mode.

CTRL−O
: Executes the next command as normal mode command and return to input mode.

CTRL−T
: Open configured editor with content of current form field.

CTRL−V
: Pass the next key press directly to gtk.

CTRL−Z
: Enter the pass-through mode.

## COMPLETIONS
{:#COMPLETIONS}

The completions are triggered by pressing \<Tab\> or \<S-Tab\> in the
activated inputbox. Depending of the current inserted content different
completions are started. The completion takes additional typed chars to filter
the completion list that is shown.

commands
: The completion for commands are started when at least `:` is shown in the
  inputbox. If there are given some sore chars the completion will lookup
  those commands that starts with the given chars.

settings
: The setting name completion is started if at least `:set ` is shown in
  inputbox and does also match settings that begins with already typed setting
  prefix.

history
: The history of URIs is shown for the `:open ` and `:tabopen ` commands. This
  completion looks up for every given word in the history URI and titles. Only
  those history items are shown, where the title or URI contains all tags.

: `:open foo bar<Tab>` will complete only URIs that contain the words foo and
  bar.

bookmarks
: The bookmark completion is similar to the history completion, but does match
  only the tags of the bookmarks. The bookmark completion ist started by
  `:open !`, `:tabopen !` or `:bmr ` and does a prefix search for all given
  words in the bookmark tags.

: `:open !foo ba` will match all bookmark that have the tags "foo" or "foot"
  and tags starting with "ba" like "ball".
: `:bmr foo bar` to complete existing bookmark matching the tags "foo" or "bar"

boomark tags
: The boomark tag completion allows to insert already used bookmarks for the
  `:bma ` commands.

search
: The search completion allow to get a filtered list of already done searches.
  This completion starts by `/` or `?` in inputbox and performs a prefix
  comparison for further typed chars.

## SETTINGS
{:#SETTINGS}

All settings listed below can be set with the `:set` command.

accelerated-2d-canvas (bool)
: Enable or disable accelerated 2D canvas. When accelerated 2D canvas is
  enabled, WebKit may render some 2D canvas content using hardware accelerated
  drawing operations.

allow-file-access-from-file-urls (bool)
: Indicates whether file access is allowed from file URLs. By default, when
  something is loaded using a file URI, cross origin requests to other file
  resources are not allowed.

allow-universal-access-from-file-urls (bool)
: Indicates whether or not JavaScript running in the context of a file scheme
  URL should be allowed to access content from any origin. By default, when
  something is loaded in a using a file scheme URL, access to the local file
  system and arbitrary local storage is not allowed.

caret (bool)
: Whether to enable accessibility enhanced keyboard navigation.

cookie-accept (string)
: Cookie accept policy {'always', 'never', 'origin' (accept all non-third-party
  cookies)}.

closed-max-items (int)
: Maximum number of stored last closed URLs. If closed-max-items is set to 0,
  closed URLs will not be stored.

completion-css (string)
: CSS style applied to the inputbox completion list items.

completion-hover-css (string)
: CSS style applied to the inputbox completion list item that is currently
  hovered by the mouse.

completion-selected-css (string)
: CSS style applied to the inputbox completion list item that is currently
  selected.

cursiv-font (string)
: The font family used as the default for content using cursive font.

default-charset (string)
: The default text charset used when interpreting content with an unspecified
  charset.

default-font (string)
: The font family to use as the default for content that does not specify a
  font.

default-zoom (int)
: Default Full-Content zoom level in percent. Default is 100.

dns-prefetching (bool)
: Indicates if Vimb prefetches domain names.

download-path (string)
: Path to the default download directory. If no download directory is set,
  download will be written into current directory. The following pattern will
  be expanded if the download is started `~/`, `~user`, `$VAR` and `${VAR}`.

editor-command (string)
: Command with placeholder '%s' called if form field is opened with $EDITOR to
  spawn the editor-like `x-terminal-emulator -e vim %s`. To use Gvim as the
  editor, it's necessary to call it with `-f' to run it in the foreground.

font-size (int)
: The default font size used to display text.

frame-flattening (bool)
: Whether to enable the Frame Flattening. With this setting each subframe is
  expanded to its contents, which will flatten all the frames to become one
  scrollable page.

fullscreen (bool)
: Show the current window full-screen.

hardware-acceleration-policy (string)
: This setting decides how to enable and disable hardware acceleration.
: - 'ondemand' enables the hardware acceleration when the web contents request
    it, disabling it again when no longer needed.
  - 'always' enforce hardware acceleration to be enabled.
  - 'never' disables it completely. Note that disabling hardware acceleration
    might cause some websites to not render correctly or consume more CPU.

header (list)
: Comma separated list of headers that replaces default header sent by WebKit
  or new headers. The format for the header list elements is `name[=[value]]`.

: Note that these headers will replace already existing headers. If there is no
  `=` after the header name, then the complete header will be removed from the
  request, if the `=` is present means that the header value is set to empty
  value.

: To use `=` within a header value the value must be quoted like shown in
  Example for the Cookie header.

: Example:
  `:set header=DNT=1,User-Agent,Cookie='name=value'` Send the 'Do Not Track'
  header with each request and remove the User-Agent Header completely from
  request.

hint-follow-last (bool)
: If on, vimb automatically follows the last remaining hint on the page. If off
  hints are fired only if enter is pressed.

hint-keys-same-length (bool)
: If on, all hint numbers will have the same length, so no hints will be
  ambiguous.

hint-timeout (int)
: Timeout before automatically following a non-unique numerical hint. To
  disable auto fire of hints, set this value to 0.

hint-keys (string)
: The keys used to label and select hints. With its default value, each hint
  has a unique number which can be typed to select it, while all other
  characters are used to filter hints based on their text. With a value such
  as `asdfg;lkjh`, each hint is 'numbered' based on the characters of the home
  row. Note that the hint matching by label built of hint-keys is case sen‐
  sitive. In this vimb differs from some other browsers that show hint labels
  in upper case, but match them lowercase. To have upper case hint labels,
  it's possible to add following css to the 'style.css' file in vimb's
  configuration directory.

: `._hintLabel {text-transform: uppercase !important;}`

history-max-items (int)
: Maximum number of unique items stored in search-, command or URI history. If
  history-max-items is set to 0, the history file will not be changed.

home-page (string)
: Homepage that vimb opens if started without a URI.

html5-database (bool)
: Whether to enable HTML5 client-side SQL database support. Client-side SQL
  database allows web pages to store structured data and be able to use SQL to
  manipulate that data asynchronously.

html5-local-storage (bool)
: Whether to enable HTML5 localStorage support. LocalStorage provides simple
  synchronous storage access.

hyperlink-auditing (bool)
: Enable or disable support for \<a ping\>.

images (bool)
: Determines whether images should be automatically loaded or not.

incsearch (bool)
: While typing a search command, show where the pattern typed so far matches.

input-autohide (bool)
: If enabled the inputbox will be hidden whenever it contains no text.

input-css (string)
: CSS style applied to the inputbox in normal state.

input-error-css (string)
: CSS style applied to the inputbox in case of displayed error.

javascript-can-access-clipboard (bool)
: Whether JavaScript can access the clipboard.

javascript-can-open-windows-automatically (bool)
: Whether JavaScript can open popup windows automatically without user
  interaction.

media-playback-allows-inline (bool)
: Whether media playback is full-screen only or inline playback is allowed.
  Setting it to false allows specifying that media playback should be always
  fullscreen.

media-playback-requires-user-gesture (bool)
: Whether a user gesture (such as clicking the play button) would be required
  to start media playback or load media. Setting it on requires a gesture by
  the user to start playback, or to load the media.

media-stream (bool)
: Enable or disable support for MediaSource on pages. MediaSource is an
  experimental proposal which extends HTMLMediaElement to allow JavaScript to
  generate media streams for playback.

mediasource (bool)
: Enable or disable support for MediaSource on pages. MediaSource is an
  experimental proposal which extends HTMLMediaElement to allow JavaScript to
  generate media streams for playback.

minimum-font-size (int)
: The minimum font size used to display text.

monospace-font (string)
: The font family used as the default for content using monospace font.

monospace-font-size (int)
: Default font size for the monospace font.

offline-cache (bool)
: Whether to enable HTML5 offline web application cache support. Offline web
  application cache allows web applications to run even when the user is not
  connected to the network.

print-backgrounds (bool)
: Whether background images should be drawn during printing.

private-browsing (bool)
: Whether to enable private browsing mode. This suppresses printing of
  messages into JavaScript Console. At the time this is the only way to force
  WebKit to not allow a page to store data in the windows sessionStorage.

plugins (bool)
: Determines whether or not plugins on the page are enabled.

sans-serif-font (string)
: The font family used as the default for content using sans-serif font.

scripts (bool)
: Determines whether or not JavaScript executes within a page.

scroll-step (int)
: Number of pixel vimb scrolls if 'j' or 'k' is used.

serif-font (string)
: The font family used as the default for content using serif font.

show-titlebar (bool)
: Determines whether the titlebar is shown (on systems that provide window
  decoration). Defaults to true.

site-specific-quirks (bool)
: Enables the site-specific compatibility workarounds.

smooth-scrolling (bool)
: Enable or disable support for smooth scrolling.

spacial-navigation (bool)
: Whether to enable the Spatial Navigation. This feature consists in the
  ability to navigate between focusable elements in a Web page, such as
  hyperlinks and form controls, by using Left, Right, Up and Down arrow keys.
  For example, if a user presses the Right key, heuristics determine whether
  there is an element they might be trying to reach towards the right, and if
  there are multiple elements, which element they probably want.

spell-checking (bool)
: Enable or disable the spell checking feature.

spell-checking-languages (string)
: Set comma separated list of spell checking languages to be used for spell
  checking. The locale string typically is in the form lang_COUNTRY, where
  lang is an ISO-639 language code, and COUNTRY is an ISO-3166 country code.
  For instance, sv_FI for Swedish as written in Finland or pt_BR for
  Portuguese as written in Brazil.

status-bar (bool)
: Indicates if the status bar should be shown.

status-css (string)
: CSS style applied to the status bar on none https pages.

status-ssl-css (string)
: CSS style applied to the status bar on https pages with trusted certificate.

status-ssl-invalid-css (string)
: CSS style applied to the status bar on https pages with untrusted
  certificate.

strict-ssl (bool)
: If 'on', vimb will not load a untrusted https site.

stylesheet (bool)
: If 'on' the user defined styles-sheet is used.

tabs-to-links (bool)
: Whether the Tab key cycles through elements on the page.

: If true, pressing the Tab key will focus the next element in the web view.
  Otherwise, the web view will interpret Tab key presses as normal key
  presses.  If the selected element is editable, the Tab key will cause the
  insertion of a Tab character.

timeoutlen (int)
: The time in milliseconds that is waited for a key code or mapped key
  sequence to complete.

user-agent (string)
: The user-agent string used by WebKit.

user-scripts (bool)
: Indicates if user style sheet file $XDG_CONFIG_HOME/vimb/style.css is
  sourced.

webaudio (bool)
: Enable or disable support for WebAudio on pages. WebAudio is an experimental
  proposal for allowing web pages to generate Audio WAVE data from JavaScript.

webgl (bool)
: Enable or disable support for WebGL on pages.

webinspector (bool)
: Determines whether or not developer tools, such as the Web Inspector, are
  enabled.

x-hint-command (string)
: Command used if hint mode `;x` is fired. The command can be any vimb command
  string. Note that the command is run through the mapping mechanism of vimb
  so it might change the behaviour by adding or changing mappings.

: `:set x-hint-command=:sh! curl -e <C-R>% <C-R>;`
: This fills the inputbox with the prefilled download command and replaces
  `<C-R>%' with the current URI and `<C-R>;' with the URI of the hinted
  element.

xss-auditor (bool)
: Whether to enable the XSS auditor. This feature filters some kinds of
  reflective XSS attacks on vulnerable web sites.

## FILES
{:#FILES}

*$XDG\_CONFIG\_HOME/vimb[/PROFILE]*
: Directory for configuration data. If executed with -p PROFILE parameter,
  configuration is read from this subdirectory. config Configuration file to
  set WebKit setting, some GUI styles and keybindings.
: **cookies.db** Sqlite cookie storage.
: **closed** Holds the URIs of last closed browser windows.
: **history** This file holds the history of unique opened URIs.
: **command** This file holds the history of commands and search queries
  performed via input box.
: **queue** Holds the read it later queue filled by `qpush'.
: **search** This file holds the history of search queries.
: **scripts.js** This file can be used to run user scripts, that are injected
  into every paged that is opened.
: **style.css** File for userdefined CSS styles. These file is used if the
  config variable `stylesheet' is enabled.

There are also some sample scripts installed together with Vimb under
_PREFIX/share/vimb/examples_.

## ENVIRONMENT
{:#ENVIRONMENT}

http_proxy
: If this variable is set to an none empty value, and the configuration option
  'proxy' is enabled, this will be used as http proxy. If the proxy URL has no
  scheme set, http is assumed.
