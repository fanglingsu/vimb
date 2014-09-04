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

## latest features

Read HTML from `stdin`
: Vimb can be started with `-` as URI parameter to read HTML from stdin.
  ```
  markdown_py README.md | vimb -
  ```

Input [CTRL-O](keybindings.html#i_CTRL-O) command
: Allow to run normal mode command in input mode with `CTRL-O`. This allows to
  yank selected text within form fields by pressing `CTRL-O Y` or to zoom the
  page like `CTRL-O 2zi`

[CTRL-A](keybindings.html#n_CTRL-A) and [CTRL-X](keybindings.html#n_CTRL-X) commands
: These commands allow to increment the last number of the current opened URL.
  This allows really efficient access to pages with pagers. These commands can
  also be prefixed by a number to page in larger steps. 

Soup Webcaching
: Vimb supports no caching of some page data on disk, to make pages load
  faster. The maximum used cache size can be configured by the new setting
  `maximum-cache-size` which is by default set to 2000kB. To disable the
  caching this config should be set to 0.

Extended [:set-Syntax](commands.html#settings)
: Like vim, vimb supports the following `:set` flavors: `:set+=val`,
  `:set-=val` and `:set^=val`. These works for strings, numbers and even
  lists.  So you can now easily add or remove a customer header by something
  like `:set header+=DNT=1`, without retyping the headers you have defined
  before.

## screenshots

There isn't really much to see for a browser that is controlled via keyboard.
But following images may give a impression of they way vimb works.

[![link hinting](media/vimb-hints.png "link hinting (688x472 32kB)"){:width="350"}](media/vimb-hints.png)
[![setting completion of vimb](media/vimb-completion.png "completion of settings (690x472 10kB)"){:width="350"}](media/vimb-completion.png)

## features

- vim like usage and [keybindings][]
- follow links via keyboard [hints][]
- read it later [queue][] to collect URIs for later use
- page marks
- tagged bookmarks
- cookie support
- userscripts and user style sheet support
- completions for commands, url history, bookmarks, bookmark tags, variables
  and search queries
- [history][] for commands, url and search queries
- open textareas with configurable editor
- user defined url [shortcuts][] with up to 9 placeholders
- xembed - so vimb can be used together with [tabbed](faq.html#tabbed)
- kiosk mode without keybindings and context menu
- manipulate http request headers
- custom [protocol handlers][handlers]
- HSTS -- HTTP Strict Transport Security
- multiple yank/paste [registers][]

## packages

- [archlinux][]
- [NetBSD][]
- [FreeBSD][]

## download
- You can get vimb from github by following command.

      git clone git://github.com/fanglingsu/vimb.git vimb

- Or you can download actual source as [tar.gz][tgz] or as [zip][] or get
  one of the [releases][].

## contribute

If you find a misbehaviour or have feature requests use the
[issue tracker][bug] provided by github or via [mailing list][mail].

## alternatives

- [vimprobable][] this was the initial inspiration for the vimb browser and has
  a lot of features in common
- [surf][] a really minimalistic browser of the suckless project. No runtime
  configuration.

[FreeBSD]:     http://www.freshports.org/www/vimb/ "vimb - FreeBSD port"
[NetBSD]:      http://pkgsrc.se/wip/vimb "vimb - NetBSD package"
[archlinux]:   https://aur.archlinux.org/packages/vimb-git/ "vimb - archlinux package"
[bug]:         https://github.com/fanglingsu/vimb/issues "vimb vim like browser - issues"
[handlers]:    commands.html#handlers "vimb custom protocol handlers"
[hints]:       keybindings.html#hinting "vimb hinting"
[history]:     keybindings.html#history "vimb keybindings to access history"
[keybindings]: keybindings.html "vimb keybindings"
[mail]:        https://lists.sourceforge.net/lists/listinfo/vimb-users "vimb vim like browser - mailing list"
[queue]:       commands.html#queue "vimb read it later queue feature"
[registers]:   keybindings.html#registers "vimb yank/paste registers"
[releases]:    https://github.com/fanglingsu/vimb/releases "vimb download releases"
[shortcuts]:   commands.html#shortcuts "vimb shortcuts"
[surf]:        http://surf.suckless.org/
[tgz]:         https://github.com/fanglingsu/vimb/archive/master.tar.gz "vimb download tar.gz"
[vimb]:        https://github.com/fanglingsu/vimb "vimb project sources"
[vimprobable]: http://sourceforge.net/apps/trac/vimprobable/
[zip]:         https://github.com/fanglingsu/vimb/archive/master.zip "vimb download zip"
*[HSTS]:       HTTP Strict Transport Security
*[vimb]:       vim browser - the vim like browser
