# Vimb - the Vim-like browser

Vimb is a Vim-like web browser that is inspired by Pentadactyl and Vimprobable.
The goal of Vimb is to build a completely keyboard-driven, efficient and
pleasurable browsing-experience with low memory and CPU usage that is
intuitive to use for Vim users.

More information and some screenshots of Vimb browser in action can be found on
the project page of [Vimb][].

## Features

- it's modal like Vim
- Vim like [keybindings][] - assignable for each browser mode
- nearly every configuration can be changed at runtime with Vim like [set syntax][set]
- [history][] for `ex` commands, search queries, URLs
- completions for: commands, URLs, bookmarked URLs, variable names of settings, search-queries
- [hinting][hints] - marks links, form fields and other clickable elements to
  be clicked, opened or inspected
- SSL validation against ca-certificate file
- HTTP Strict Transport Security (HSTS)
- open input or textarea with configurable external editor
- user defined URL-shortcuts with placeholders
- custom [protocol handlers][handlers]
- read it later [queue][] to collect URIs for later use
- multiple yank/paste [registers][]
- Vim like [autocmd][]

## Packages

- Arch Linux [vimb-git][arch-git], [vimb][arch]
- [OpenBSD][]
- [NetBSD][]
- [FreeBSD][]
- [Void Linux][]

## Dependencies

- libwebkit >=1.5.0
- libgtk+-2.0
- libsoup >=2.38

On Ubuntu these dependencies can be installed by
`sudo apt-get install libsoup2.4-dev libwebkit-dev libgtk-3-dev libwebkitgtk-3.0-dev`.

## Install

Edit `config.mk` to match your local setup.

Edit `src/config.h` to match your personal preferences.

The default `Makefile` will not overwrite your customised `config.h` with the
contents of `config.def.h`, even if it was updated in the latest git pull.
Therefore, you should always compare your customised `config.h` with
`config.def.h` and make sure you include any changes to the latter in your
`config.h`.

Run the following commands to compile and install Vimb (if necessary, the last one as
root).

    make clean
    make // or make GTK=3 to compile against gtk3
    make install

To build Vimb against GTK3 you can use `make GTK=3`.

# License

Information about the license is found in the file: LICENSE.

# Mailing list

- feature requests, issues and patches can be discussed on the [mailing list][mail]

[vimb]:        http://fanglingsu.github.io/vimb/ "Vimb - Vim like browser project page"
[keybindings]: http://fanglingsu.github.io/vimb/keybindings.html "vimb keybindings"
[hints]:       http://fanglingsu.github.io/vimb/keybindings.html#hinting "vimb hinting"
[queue]:       http://fanglingsu.github.io/vimb/commands.html#queue "vimb read it later queue feature"
[history]:     http://fanglingsu.github.io/vimb/keybindings.html#history "vimb keybindings to access history"
[handlers]:    http://fanglingsu.github.io/vimb/commands.html#handlers "vimb custom protocol handlers"
[registers]:   http://fanglingsu.github.io/vimb/keybindings.html#registers "vimb yank/paste registers"
[mail]:        https://lists.sourceforge.net/lists/listinfo/vimb-users "vimb - mailing list"
[OpenBSD]:     http://openports.se/www/vimb "vimb - OpenBSD port"
[NetBSD]:      http://pkgsrc.se/www/vimb "vimb - NetBSD package"
[autocmd]:     http://fanglingsu.github.io/vimb/commands.html#autocmd "Vim like autocmd and augroup feature"
[set]:         http://fanglingsu.github.io/vimb/commands.html#settings "Vim like set syntax"
[Arch-git]:    https://aur.archlinux.org/packages/vimb-git/ "vimb - archlinux package"
[Arch]:        https://aur.archlinux.org/packages/vimb/ "vimb - archlinux package"
[FreeBSD]:     http://www.freshports.org/www/vimb/ "vimb - FreeBSD port"
[Void Linux]:  https://github.com/voidlinux/void-packages/blob/master/srcpkgs/vimb/template "vimb - Void Linux package"
