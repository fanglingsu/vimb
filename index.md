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

New setting 'closed-max-items'
: Maximum number of stored last closed browser windows. If closed-max-items is
  set to 0, closed browser windows will not be stored. By default this is 10.

New option [-p, --profile](man.html#OPTIONS)
: This allows to create and run vimb with a seperate named config directory,
  with own history and bookmarks.

New setting 'hint-follow-last'
: If on, vimb automatically follows the last remaining hint on the page
  (default behaviour). If off hints are fired only if enter is pressed.

Remote-Control via socket (previously a fifo)
: If vimb is started with `-s` or `--socket` option, vimb creates a unix
  domain socket named `$XDG_RUNTIME_DIR/vimb/socket/{pid}`.
: All commands written to the socket are executed in the same way like the
  right hand side of the `map` commands. This allow to perform normal mode
  commands as well as ex commands.
: Example:

  ```shell
  sh -c "vimb -s -d > ~/socket" &
  SOCKET=$(< ~/socket)
  echo ':o http://fanglingsu.github.io/vimb/<CR>' | socat - UNIX-CONNECT:$SOCKET
  echo ':o !<Tab><Tab><CR>' | socat - UNIX-CONNECT:$SOCKET
  echo '<C-Q>' | socat - UNIX-CONNECT:$SOCKET
  # or start an interactive remote control session
  socat READLINE UNIX-CONNECT:$(< ~/socket)
  ```


## screenshots

There isn't really much to see for a browser that is controlled via keyboard.
But following images may give a impression of they way vimb works.

[![vimb hinting marks active element like links](media/vimb-hints.png "link hinting (688x472 32kB)"){:width="350"}](media/vimb-hints.png)
[![completion with scrallable completion menu](media/vimb-completion.png "completion of settings (690x472 10kB)"){:width="350"}](media/vimb-completion.png)

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
- multiple yank/paste registers

## packages

- archlinux [vimb-git][arch-git], [vimb][arch]
- [OpenBSD][]
- [NetBSD][]
- [FreeBSD][]
- [Void Linux][]

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
[OpenBSD]:     http://openports.se/www/vimb "vimb - OpenBSD port"
[NetBSD]:      http://pkgsrc.se/www/vimb  "vimb - NetBSD package"
[arch-git]:    https://aur.archlinux.org/packages/vimb-git/ "vimb - archlinux package"
[arch]:        https://aur.archlinux.org/packages/vimb/ "vimb - archlinux package"
[bug]:         https://github.com/fanglingsu/vimb/issues "vimb vim like browser - issues"
[handlers]:    man.html#Handlers "vimb custom protocol handlers"
[hints]:       man.html#Hinting "vimb hinting"
[history]:     man.html#Command_Line_History "vimb keybindings to access history"
[keybindings]: man.html#Key_Mapping "vimb keybindings"
[mail]:        https://lists.sourceforge.net/lists/listinfo/vimb-users "vimb vim like browser - mailing list"
[queue]:       man.html#Queue "vimb read it later queue feature"
[releases]:    https://github.com/fanglingsu/vimb/releases "vimb download releases"
[shortcuts]:   man.html#Shortcuts "vimb shortcuts"
[surf]:        http://surf.suckless.org/
[tgz]:         https://github.com/fanglingsu/vimb/archive/master.tar.gz "vimb download tar.gz"
[vimb]:        https://github.com/fanglingsu/vimb "vimb project sources"
[vimprobable]: http://sourceforge.net/apps/trac/vimprobable/
[zip]:         https://github.com/fanglingsu/vimb/archive/master.zip "vimb download zip"
[Void Linux]:  https://github.com/voidlinux/void-packages/blob/master/srcpkgs/vimb/template "vimb - Void Linux package"
*[HSTS]:       HTTP Strict Transport Security
*[vimb]:       vim browser - the vim like browser
