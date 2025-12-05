# Vimb - the Vim-like browser

[![Build Status](https://travis-ci.com/fanglingsu/vimb.svg?branch=master)](https://travis-ci.com/fanglingsu/vimb)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Latest Release](https://img.shields.io/github/release/fanglingsu/vimb.svg?style=flat)](https://github.com/fanglingsu/vimb/releases/latest)

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

- Arch Linux: [extra/vimb][], [aur/vimb-git][], [aur/vimb-gtk2][]
- Debian: [trixie/vimb][], [sid/vimb][]
- Fedora: [fedora/vimb][],
- Gentoo: [tharvik overlay][], [jjakob overlay][]
- openSUSE: [network/vimb][]
- pkgsrc: [pkgsrc/www/vimb][], [pkgsrc/wip/vimb-git][]
- Slackware: [slackbuild/vimb][]

## dependencies

- gtk4
- webkitgtk-6.0
- gst-libav, gst-plugins-good (optional, for media decoding among other things)

**Note:** Vimb has been migrated from GTK3/WebKit2GTK-4.1 to GTK4/WebKitGTK-6.0.
This is a major version change that requires GTK4 and WebKitGTK 6.0 or later.

### Native Tab Support

Vimb now has **native tab support** using GTK4's `GtkNotebook`. All tabs share a 
single window with a shared command input at the bottom.

**Tab Commands:**
- `:tabopen [uri]` or `:tabo [uri]` - Open URI in new tab
- `:tabclose` - Close current tab
- `:tabnext` or `:tabn` - Switch to next tab  
- `:tabprev` or `:tabp` - Switch to previous tab
- `:tabfirst` - Go to first tab
- `:tablast` - Go to last tab

**Keyboard Shortcuts (Normal Mode):**
- `gt` - Go to next tab
- `gT` - Go to previous tab
- `g0` - Go to first tab
- `g$` - Go to last tab

**Note:** GTK4 has removed XEmbed support (GtkPlug/GtkSocket). The `-e, --embed` 
flag is no longer functional. Native tabs replace the need for external tools 
like `tabbed`.

## Install

Edit `config.mk` to match your local setup. You might need to do this if
you use another compiler, like tcc. Most people, however, will almost never
need to do this on systems like Ubuntu or Debian.

Edit `src/config.h` to match your personal preferences, like changing the
characters used in the loading bar, or the font.

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

- feature requests, issues and patches can be discussed on the [mailing list][mail] ([list archive][mail-archive])

## Similar projects

- [luakit](https://luakit.github.io/)
- [qutebrowser](https://www.qutebrowser.org/)
- [surf](https://surf.suckless.org/)
- [uzbl](https://www.uzbl.org/)
- [wyeb](https://github.com/jun7/wyeb)

## license

Information about the license are found in the file LICENSE.

## about

- https://en.wikipedia.org/wiki/Vimb
- http://thedarnedestthing.com/vimb
- https://blog.jeaye.com/2015/08/23/vimb/

[aur/vimb-git]:        https://aur.archlinux.org/packages/vimb-git
[aur/vimb-gtk2]:       https://aur.archlinux.org/packages/vimb-gtk2/
[trixie/vimb]:         https://packages.debian.org/trixie/vimb
[sid/vimb]:            https://packages.debian.org/sid/vimb
[extra/vimb]:          https://www.archlinux.org/packages/extra/x86_64/vimb/
[fedora/vimb]:         https://src.fedoraproject.org/rpms/vimb
[tharvik overlay]:     https://github.com/tharvik/overlay/tree/master/www-client/vimb
[jjakob overlay]:      https://github.com/jjakob/gentoo-overlay/tree/master/www-client/vimb
[mail-archive]:        https://sourceforge.net/p/vimb/vimb/vimb-users/ "vimb - mailing list archive"
[mail]:                https://lists.sourceforge.net/lists/listinfo/vimb-users "vimb - mailing list"
[network/vimb]:        https://build.opensuse.org/package/show/network/vimb
[pkgsrc/wip/vimb-git]: http://pkgsrc.se/wip/vimb-git
[pkgsrc/www/vimb]:     http://pkgsrc.se/www/vimb
[slackbuild/vimb]:     https://slackbuilds.org/repository/14.2/network/vimb/
[vimb]:                https://fanglingsu.github.io/vimb/ "Vimb - Vim like browser project page"
