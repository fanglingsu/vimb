# vimb
Vimb is a web browser that behaves like Vimprobable, but with some
paradigms from dwb and hopefully a cleaner code base. The goal of Vimb is to
build a completely keyboard-driven, efficient and pleasurable
browsing-experience with low memory and cpu usage.

More information and some screenshots of vimb browser in action can be found on
the [vimb project page][vimb].

## features
- vim-like modal
- vim-like keybindings
- nearly every configuration can be changed on runtime with `:set varname=value`
  - allow to inspect the current set values of variables `:set varname?`
  - allow to toggle boolean variables with `:set varname!`
- keybindings for each browser mode assignable
- history for
  - commands
  - search queries
  - urls
- completions for
  - commands
  - urls
  - bookmarked urls
  - variable names of settings
  - search-queries
- hinting - marks links, form fields and other clickable elements to be
  clicked, opened or inspected
- webinspector that opens ad the bottom of the browser window like in some
  other fat browsers
- ssl validation against ca-certificate file
- HTTP Strict Transport Security (HSTS)
- custom configuration files
- open input or textarea with configurable external editor
- user defined URL-shortcuts with placeholders
- run shell commands from inpubox
- read it later queue to collect URIs for later use

## packages

- [archlinux][]
- [NetBSD][]

## dependencies
- libwebkit >=1.5.0
- libgtk+-2.0
- libsoup-2.4

## install
Edit config.mk to match your local setup.

Edit config.h to match your personal preferences.

The default Makefile will not overwrite your customised `config.h` with the
contents of `config.def.h`, even if it was updated in the latest git pull.
Therefore, you should always compare your customised `config.h` with
`config.def.h` and make sure you include any changes to the latter in your
`config.h`.

Run following command to compile and install vimb (if necessary last one as
root).

    make clean
    make // or make GTK=3 to compile against gtk3
    make install

To build vimb against gtk3 you can use `make GTK=3`.

# license
Information about the license are found in the file LICENSE.

# mailing list
- feature requests, issues and patches can be discussed on the [mailing list][mail]

[vimb]:        http://fanglingsu.github.io/vimb/ "vimb - vim-like webkit browser project page"
[mail]:        https://lists.sourceforge.net/lists/listinfo/vimb-users "vimb - mailing list"
[archlinux]:   https://aur.archlinux.org/packages/vimb-git/ "vimb - archlinux package"
[NetBSD]:      http://pkgsrc.se/wip/vimb "vimb - NetBSD package"
