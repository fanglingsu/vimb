---
title:  Vimb - The Vim like Browser
layout: default
meta:   vimb - the vim like browser is a fast, keyboard driven and lightweight webkit web-browser
nav_sort: 1
nav_title: home
---

# vimb - the vim like browser
[Vimb][vimb] is a fast and lightweight vim like web browser based on the
webkit web browser engine and the GTK toolkit. Vimb is modal like the great
vim editor and also easily configurable during runtime. Vimb is mostly
keyboard driven and does not distract you from your daily work.

If your are familiar with vim or have some experience with pentadactyl the use
of vimb would be a breeze, if not we missed our target.

## latest features
New setting `status-bar-show-settings`
: This allows to enable visualization of some runtime settings. Format of this
  and the settings to show are configured in `src/config.h` on compilation time.

New setting `notification`
: These new settings allow to control the website access to the notification
  API.

Add `$VIMB_SELECTION` when `:shellcmd` is run.
: This variable is set to the current selected text whenever a `:shellcmd` is
  executed.

[Vimb 3.7.0](https://github.com/fanglingsu/vimb/releases/tag/3.7.0) released

New option `--cmd, -C`
: Allow to give ex commands on startup that are give to new spawned child
  instances too.

More about latest changes can be found in the [CHANGELOG.md][]. 

## screenshots
There isn't really much to see for a browser that is controlled via keyboard.
But following images may give a impression of they way vimb works.

[![vimb hinting marks active element like links](media/vimb-hints.png "link hinting (684x465 : 13kB)"){:width="350"}](media/vimb-hints.png)
[![completion with scrallable completion menu](media/vimb-completion.png "completion of settings (686x467 : 13kB)"){:width="350"}](media/vimb-completion.png)

## features
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
- Vim like [autocmd][] - execute commands automatically after an event on specific
  URIs

## packages
- Arch Linux: [community/vimb][], [aur/vimb-git][], [aur/vimb-gtk2][]
- Fedora: [fedora/vimb][]
- Gentoo: [gentoo-git][], [gentoo][]
- openSUSE: [network/vimb][]
- pkgsrc: [pkgsrc/www/vimb][], [pkgsrc/wip/vimb-git][]
- Slackware: [slackbuild/vimb][]

## download
- You can get vimb from github by following command.

      git clone git://github.com/fanglingsu/vimb.git

- Or you can download actual source as [tar.gz][tgz] or as [zip][] or get
  one of the [releases][].

## dependencies

- gtk+-3.0
- webkit2gtk-4.1
- gst-libav, gst-plugins-good (optional, for media decoding among other things)

## install
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

Run the following commands to compile and install Vimb (if necessary, the last
one as root). `V=1` enables verbose output for those that are interested to
see full compiler option lines.

    make V=1
    make install

If you wish to install with other `PREFIX` note that this option must be
given for both steps the compile step as well as the install step.

    make PREFIX="/usr"
    make PREFIX="/usr" DESTDIR="/home/user" install

To run vimb without installation for testing it out use the 'runsandbox' make
target.

    make runsandbox

## contribute
If you find a misbehaviour or have feature requests use the
[issue tracker][bug] provided by github or via [mailing list][mail] ([list archive][mail-archive]).

## about
- [http://thedarnedestthing.com/vimb](http://thedarnedestthing.com/vimb)
- [https://blog.jeaye.com/2015/08/23/vimb/](https://blog.jeaye.com/2015/08/23/vimb/)

[aur/vimb-git]:        https://aur.archlinux.org/packages/vimb-git
[aur/vimb-gtk2]:       https://aur.archlinux.org/packages/vimb-gtk2/
[community/vimb]:      https://www.archlinux.org/packages/community/x86_64/vimb/
[fedora/vimb]:         https://src.fedoraproject.org/rpms/vimb
[gentoo-git]:          https://github.com/tharvik/overlay/tree/master/www-client/vimb
[gentoo]:              https://github.com/hsoft/portage-overlay/tree/master/www-client/vimb
[vimb]:                https://github.com/fanglingsu/vimb "vimb project sources"
[mail]:                https://lists.sourceforge.net/lists/listinfo/vimb-users "vimb vim like browser - mailing list"
[mail-archive]:        https://sourceforge.net/p/vimb/vimb/vimb-users/ "vimb - mailing list archive"
[bug]:                 https://github.com/fanglingsu/vimb/issues "vimb vim like browser - issues"
[zip]:                 https://github.com/fanglingsu/vimb/archive/master.zip "vimb download zip"
[tgz]:                 https://github.com/fanglingsu/vimb/archive/master.tar.gz "vimb download tar.gz"
[releases]:            https://github.com/fanglingsu/vimb/releases "vimb download releases"
[autocmd]:             https://fanglingsu.github.io/vimb/man.html#Autocmd "vim like autocmd"
[slackbuild/vimb]:     https://slackbuilds.org/repository/14.2/network/vimb/
[pkgsrc/wip/vimb-git]: http://pkgsrc.se/wip/vimb-git
[pkgsrc/www/vimb]:     http://pkgsrc.se/www/vimb
[network/vimb]:        https://build.opensuse.org/package/show/network/vimb
[CHANGELOG.md]:        https://github.com/fanglingsu/vimb/blob/master/CHANGELOG.md "Changelog of the vimb browser project"
*[feature]: "asdf asdf"
