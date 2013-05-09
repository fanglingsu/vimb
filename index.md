---
title:  Vimb - Vim-Like Webkit Browser
layout: default
meta:   vimb browser is a fast, keyboard driven and lightweight browser
---

# vimb - a vim-like webkit browser

vimb is a fast and lightweight web browser based on the webkit web browser
engine and the gtk toolkit. vimb is modal like the great vim editor and also
easy configurable during runtime. vimb is mostly keyboard driven and does not
detract you from your daily work.

## features
- modal like vim
- vim oriented [keybindings][]
- follow links via keyboard hints
- bookmarks
- cookie support
- userscripts support
- customer stylesheet support
- completions for
  - commands
  - urls from history
  - bookmarked urls
  - variables
  - search queries from history
- history for
  - commands
  - urls
  - search queries
- open textareas with configurable editor
- user defined url shortcuts with up to 9 placeholders
- xembed - so vimb can be used together with [tabbed][]

## download

- You can get [vimb][] from github with following command.

      git clone git://github.com/fanglingsu/vimb.git vimb

- Or you can download the latest releases as [tar.gz][tgz] or as [zip][].

## dependencies

- libwebkit-1.0
- libgtk+-2.0
- libsoup-2.4

## bugs

If you find a misbehaviour or have feature requests use the
[issue tracker][bug] provided by github.

## alternatives

- [vimprobable][] this was the initial inspiration for the vimb browser and has
  a lot of features in common
- [surf][] a really minimalistic browser of the suckless project. No runtime
  configuration.

[zip]:  https://github.com/fanglingsu/vimb/archive/master.zip "vim browser download zip"
[tgz]:  https://github.com/fanglingsu/vimb/archive/master.tar.gz "vim browser download tar.gz"
[bug]:  https://github.com/fanglingsu/vimb/issues "vimb browser - issue tracker"
[surf]: http://surf.suckless.org/
[vimb]: https://github.com/fanglingsu/vimb "vimb browser project"
[vimprobable]: http://sourceforge.net/apps/trac/vimprobable/
[tabbed]:      http://tools.suckless.org/tabbed/
[keybindings]: keybindings.html#default-keys
