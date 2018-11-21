# Vimb - the Vim-like browser

[![Build Status](https://api.travis-ci.com/fanglingsu/vimb.svg?branch=master)](https://travis-ci.com/fanglingsu/vimb)

Vimb is a Vim-like web browser that is inspired by Pentadactyl and Vimprobable.
The goal of Vimb is to build a completely keyboard-driven, efficient and
pleasurable browsing-experience with low memory and CPU usage that is
intuitive to use for Vim users.

More information and some screenshots of Vimb browser in action can be found on
the project page of [Vimb][].

## Features

- it's modal like Vim
- Vim like keybindings - assignable for each browser mode
- nearly every configuration can be changed at runtime with Vim like set syntax
- history for `ex` commands, search queries, URLs
- completions for: commands, URLs, bookmarked URLs, variable names of settings, search-queries
- hinting - marks links, form fields and other clickable elements to
  be clicked, opened or inspected
- SSL validation against ca-certificate file
- user defined URL-shortcuts with placeholders
- read it later queue to collect URIs for later use
- multiple yank/paste registers
- Vim like autocmd - execute commands automatically after an event on specific URIs

## Packages

- Arch Linux: [aur/vimb][], [aur/vimb-git][]
- Gentoo: [gentoo-git][], [gentoo][]
- Slackware: [slackbuild/vimb][]

## dependencies

- gtk+-3.0
- webkit2gtk-4.0 >= 2.8.x

## Install

Edit `config.mk` to match your local setup.

Edit `src/config.h` to match your personal preferences.

The default `Makefile` will not overwrite your customised `config.h` with the
contents of `config.def.h`, even if it was updated in the latest git pull.
Therefore, you should always compare your customised `config.h` with
`config.def.h` and make sure you include any changes to the latter in your
`config.h`.

Run the following commands to compile and install Vimb (if necessary, the last one as
root). If you want to change the `PREFIX`, note that it's required to give it on both stages, build and install.

    make PREFIX=/usr
    make PREFIX=/usr install

To run vimb without installation for testing it out use the 'runsandbox' make
target.

    make runsandbox

## Mailing list

- feature requests, issues and patches can be discussed on the [mailing list][mail] ([mail-archive][list archive])

## Similar projects

- [luakit](https://luakit.github.io/)
- [qutebrowser](https://www.qutebrowser.org/)
- [surf](https://surf.suckless.org/)
- [uzbl](https://www.uzbl.org/)

## license

Information about the license are found in the file LICENSE.

## about

- https://en.wikipedia.org/wiki/Vimb
- http://thedarnedestthing.com/vimb
- https://blog.jeaye.com/2015/08/23/vimb/

[aur/vimb]:          https://aur.archlinux.org/packages/vimb
[aur/vimb-git]:      https://aur.archlinux.org/packages/vimb-git
[gentoo-git]:        https://github.com/tharvik/overlay/tree/master/www-client/vimb
[gentoo]:            https://github.com/hsoft/portage-overlay/tree/master/www-client/vimb
[vimb]:              https://fanglingsu.github.io/vimb/ "Vimb - Vim like browser project page"
[mail]:              https://lists.sourceforge.net/lists/listinfo/vimb-users "vimb - mailing list"
[mail-archive]:      https://sourceforge.net/p/vimb/vimb/vimb-users/ "vimb - mailing list archive"
[slackbuild/vimb]:   https://slackbuilds.org/repository/14.2/network/vimb/
