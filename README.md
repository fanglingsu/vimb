# vimb
Vimb is a web browser that behaves like the Vimprobable but with some
paradigms from dwb and hopefully a cleaner code base. The goal of Vimb is to
build a completely keyboard-driven, efficient and pleasurable
browsing-experience with low memory and cpu usage.

## Features
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
- hinting - marks links, form fields and other clickable elements to be
  clicked, opened or inspected
- webinspector that opens ad the bottom of the browser window like in some
  other fat browsers
- ssl validation against ca-certificate file
- custom configuration files
- tagged bookmarks
- open input or teaxteaeas with configurable external editor

## Dependencies
- libwebkit-1.0
- libgtk+-2.0
- libsoup-2.4

# License
Information about the license are found in the file LICENSE.
