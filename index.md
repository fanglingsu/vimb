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

Remote-Control via socket (previously a fifo)
: If vimb is started with `-s` or `--socket` option, vimb creates a unix
  domain socket named `$XDG_CONFIG_HOME/vimb/socket/{pid}`.
: All commands written to the socket are executed in the same way like the
  right hand side of the `map` commands. This allow to perform normal mode
  commands as well as ex commands.
: Example:

      sh -c "vimb -s -d > ~/socket" &
      SOCKET=$(< ~/socket)
      echo ':o http://fanglingsu.github.io/vimb/<CR>' | socat - UNIX-CONNECT:$SOCKET
      echo ':o !<Tab><Tab><CR>' | socat - UNIX-CONNECT:$SOCKET
      echo '<C-Q>' | socat - UNIX-CONNECT:$SOCKET
      # or start an interactive remote control session
      socat READLINE UNIX-CONNECT:$(< ~/socket)

Hinting by Letters
: Vimb allow now to set the chars used to built the hint labels. To have a
  more vimium like behaviour you can `:set hintkeys=sdfghjkla`. This allows
  fast filtering without moving you hands from the home row of the keyboard.

Auto-Response-Header
: Prepend HTTP-Header to responses received from server, based on pattern matching. The purpose of this setting is to enforce some security setting in the client. For example, you could set [Content-Security-Policy](http://www.w3.org/TR/CSP/) to implement a whitelist policy, or set Strict-Transport-Security for server that don't provide this header whereas they propose https website.
: Note that this setting will not remplace existing headers, but add a new one. If multiple patterns match a request uri, the last matched rule will be applied. You could also specified differents headers for same pattern.
: The format is a list of `pattern header-list`. If `header-list` has not than one element, enclosing with QUOTE is mandatory: `"pattern header-list"`. The header-list format is the same as `header` setting.
: Example:

      :set auto-response-header=* Content-security-policy=default-src 'self' 'unsafe-inline' 'unsafe-eval'; script-src 'none'
      :set auto-response-header+=https://example.com/* Content-security-policy=default-src 'self' https://*.example.com/
      :set auto-response-header+=https://example.com/* Strict-Transport-Security=max-age=31536000
      :set auto-response-header+="https://*.example.org/sub/* Content-security-policy,X-Test=ok"


[Auto commands](commands.html#autocmd)
: Vimb provides another nice feature of vim to run command on various events and
  pattern matches URIs, `:autocmd` and `:augroup`.

Read HTML from `stdin`
: Vimb can be started with `-` as URI parameter to read HTML from stdin.
  ```
  markdown_py README.md | vimb -
  ```

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
- multiple yank/paste registers

## packages

- archlinux [vimb-git][arch-git], [vimb][arch]
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
[NetBSD]:      http://pkgsrc.se/wip/vimb "vimb - NetBSD package"
[arch-git]:    https://aur.archlinux.org/packages/vimb-git/ "vimb - archlinux package"
[arch]:        https://aur.archlinux.org/packages/vimb/ "vimb - archlinux package"
[bug]:         https://github.com/fanglingsu/vimb/issues "vimb vim like browser - issues"
[handlers]:    commands.html#handlers "vimb custom protocol handlers"
[hints]:       keybindings.html#hinting "vimb hinting"
[history]:     keybindings.html#history "vimb keybindings to access history"
[keybindings]: keybindings.html "vimb keybindings"
[mail]:        https://lists.sourceforge.net/lists/listinfo/vimb-users "vimb vim like browser - mailing list"
[queue]:       commands.html#queue "vimb read it later queue feature"
[releases]:    https://github.com/fanglingsu/vimb/releases "vimb download releases"
[shortcuts]:   commands.html#shortcuts "vimb shortcuts"
[surf]:        http://surf.suckless.org/
[tgz]:         https://github.com/fanglingsu/vimb/archive/master.tar.gz "vimb download tar.gz"
[vimb]:        https://github.com/fanglingsu/vimb "vimb project sources"
[vimprobable]: http://sourceforge.net/apps/trac/vimprobable/
[zip]:         https://github.com/fanglingsu/vimb/archive/master.zip "vimb download zip"
[Void Linux]:  https://github.com/voidlinux/void-packages/blob/master/srcpkgs/vimb/template "vimb - Void Linux package"
*[HSTS]:       HTTP Strict Transport Security
*[vimb]:       vim browser - the vim like browser
