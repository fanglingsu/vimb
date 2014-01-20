---
title:  Vimb - The Vim like Browser
layout: default
meta:   vimb - the vim like browser is a fast, keyboard driven and lightweight web-browser
active: home
---

# vimb - the vim like browser

[Vimb][vimb] is a fast and lightweight vim like web browser based on the
webkit web browser engine and the GTK toolkit. Vimb is modal like the great
vim editor and also easily configurable during runtime. Vimb is mostly
keyboard driven and does not detract you from your daily work.

If your are familiar with vim or have some experience with pentadactyl the use
of vimb would be a breeze, if not we missed our target.

## screenshots

There isn't really much to see for a browser that is controlled via keyboard.
But following images may give a impression of they way vimb works.

[![link hinting](media/vimb-hints.png "link hinting (688x472 32kB)"){:width="350"}](media/vimb-hints.png)
[![setting completion of vimb](media/vimb-completion.png "completion of settings (690x472 10kB)"){:width="350"}](media/vimb-completion.png)

## features

- modal like vim
- vim oriented [keybindings][]
- follow links via keyboard [hints][]
- read it later [queue][] to collect URIs for later use
- page marks
- tagged bookmarks
- cookie support
- userscripts support
- customer stylesheet support
- completions for commands, url history, bookmarks, bookmark tags, variables
  and search queries
- [history][] for commands, url and search queries
- open textareas with configurable editor
- user defined url [shortcuts][] with up to 9 placeholders
- xembed - so vimb can be used together with [tabbed][]
- run shell commands from inputbox
- manipulate http request headers

## packages

- [archlinux][]
- [NetBSD][]

## install

1. Download the sources

   - You can get vimb from github by following command.

         git clone git://github.com/fanglingsu/vimb.git vimb

   - Or you can download actual source as [tar.gz][tgz] or as [zip][] or get
     one of the [releases][].

2. Edit `config.mk` to match your local setup.

3. Edit `config.h` to match you personal preferences.

   The default Makefile will not overwrite your customised `config.h` with the
   contents of `config.def.h`, even if it was updated in the latest git pull.
   Therefore, you should always compare your customised `config.h` with
   `config.def.h` and make sure you include any changes to the latter in your
   `config.h`.

4. Run following commands to compile and install vimb (if necessary last one as
   root).

       make clean
       make             // or make GTK=3 to build agains gtk3 libs
       make install

## dependencies

- libwebkit >= 1.5.0
- libgtk+-2.0
- libsoup-2.4

## contribute

If you find a misbehaviour or have feature requests use the
[issue tracker][bug] provided by github or via [mailing list][mail].

## alternatives

- [vimprobable][] this was the initial inspiration for the vimb browser and has
  a lot of features in common
- [surf][] a really minimalistic browser of the suckless project. No runtime
  configuration.

[zip]:         https://github.com/fanglingsu/vimb/archive/master.zip "vimb download zip"
[tgz]:         https://github.com/fanglingsu/vimb/archive/master.tar.gz "vimb download tar.gz"
[releases]:    https://github.com/fanglingsu/vimb/releases "vimb download releases"
[bug]:         https://github.com/fanglingsu/vimb/issues "vimb vim like browser - issues"
[surf]:        http://surf.suckless.org/
[vimb]:        https://github.com/fanglingsu/vimb "vimb vim like browser sources"
[vimprobable]: http://sourceforge.net/apps/trac/vimprobable/
[tabbed]:      http://tools.suckless.org/tabbed/
[keybindings]: keybindings.html "vimb keybindings"
[hints]:       keybindings.html#hinting "vimb hinting"
[history]:     keybindings.html#history "vimb keybindings to access history"
[queue]:       commands.html#queue "vimb read it later queue feature"
[shortcuts]:   commands.html#shortcuts "vimb shortcuts"
[mail]:        https://lists.sourceforge.net/lists/listinfo/vimb-users "vimb vim like browser - mailing list"
[archlinux]:   https://aur.archlinux.org/packages/vimb-git/ "vimb - archlinux package"
[NetBSD]:      http://pkgsrc.se/wip/vimb "vimb - NetBSD package"
