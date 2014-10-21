# vimb - the vim like browser

Vimb is vim like webbrowser that is inspired by pentadactyl and vimprobable.
The goal of Vimb is to build a completely keyboard-driven, efficient and
pleasurable browsing-experience with low memory and cpu usage that is
intuitive to use for vim users.

More information and some screenshots of vimb browser in action can be found on
the project page of [vimb][].

## features

- it's modal like vim
- vim like [keybindings][] - assignable for each browser mode
- nearly every configuration can be changed on runtime with vim like [set syntax][set]
- [history][] for ex commands, search queries, urls
- completions for, commands, urls, bookmarked urls, variable names of settings, search-queries
- [hinting][hints] - marks links, form fields and other clickable elements to
  be clicked, opened or inspected
- ssl validation against ca-certificate file
- HTTP Strict Transport Security (HSTS)
- open input or textarea with configurable external editor
- user defined URL-shortcuts with placeholders
- custom [protocol handlers][handlers]
- read it later [queue][] to collect URIs for later use
- multiple yank/paste [registers][]
- vim like [autocmd][]

## packages

- archlinux [vimb-git][arch-git], [vimb][arch]
- [NetBSD][]
- [FreeBSD][]

## dependencies

- libwebkit >=1.5.0
- libgtk+-2.0
- libsoup >=2.38

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

[vimb]:        http://fanglingsu.github.io/vimb/ "vimb - vim like browser project page"
[keybindings]: http://fanglingsu.github.io/vimb/keybindings.html "vimb keybindings"
[hints]:       http://fanglingsu.github.io/vimb/keybindings.html#hinting "vimb hinting"
[queue]:       http://fanglingsu.github.io/vimb/commands.html#queue "vimb read it later queue feature"
[history]:     http://fanglingsu.github.io/vimb/keybindings.html#history "vimb keybindings to access history"
[handlers]:    http://fanglingsu.github.io/vimb/commands.html#handlers "vimb custom protocol handlers"
[registers]:   http://fanglingsu.github.io/vimb/keybindings.html#registers "vimb yank/paste registers"
[mail]:        https://lists.sourceforge.net/lists/listinfo/vimb-users "vimb - mailing list"
[NetBSD]:      http://pkgsrc.se/wip/vimb "vimb - NetBSD package"
[autocmd]:     http://fanglingsu.github.io/vimb/commands.html#autocmd "vim like autocmd and augroup feature"
[set]:         http://fanglingsu.github.io/vimb/commands.html#settings "vim like set syntax"
[arch-git]:    https://aur.archlinux.org/packages/vimb-git/ "vimb - archlinux package"
[arch]:        https://aur.archlinux.org/packages/vimb/ "vimb - archlinux package"
[FreeBSD]:     http://www.freshports.org/www/vimb/ "vimb - FreeBSD port"
