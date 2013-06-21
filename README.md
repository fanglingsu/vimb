# vimb
Vimb is a web browser that behaves like the Vimprobable but with some
paradigms from dwb and hopefully a cleaner code base. The goal of Vimb is to
build a completely keyboard-driven, efficient and pleasurable
browsing-experience with low memory and cpu usage.

More informatio and some screenshots of vimb browser in action can be found on
the [vimb project page][vimb].

## features
- vim-like modal
- vim-like keybindings
- nearly every configuration can be changed on runtime with `:set varname=value`
  - allow to inspect the current set values of variables `:set varname?`
  - allow to toggle boolean variables with `:set varname!`
- keybindings for each browser mode assignable
- the center of `vimb` are the commands that can be called from inputbox or
  via keybinding
- history for
  - commands
  - search queries
  - urls
- completions for
  - commands
  - urls
  - variable names of settings
  - search-queries
- hinting - marks links, form fields and other clickable elements to be
  clicked, opened or inspected
- webinspector that opens ad the bottom of the browser window like in some
  other fat browsers
- ssl validation against ca-certificate file
- custom configuration files
- tagged bookmarks
- open input or teaxteaeas with configurable external editor
- user defined URL-shortcuts with placeholders

## dependencies
- libwebkit >=1.3.10
- libgtk+-2.0
- libsoup-2.4

# license
Information about the license are found in the file LICENSE.

[vimb]: http://fanglingsu.github.io/vimb/ "vimb - vim-like webkit browser project page"
