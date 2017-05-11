# Vimb - the Vim-like browser

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

## Packages

- Gentoo [gentoo-git][], [gentoo][]

## dependencies

- webkit2gtk-4.0 >= 2.16.x

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

    make
    make install

## Mailing list

- feature requests, issues and patches can be discussed on the [mailing list][mail]

## Patching and Coding style

### File Layout

- Comment with LICENSE and possibly short explanation of file/tool
- Headers
- Macros
- Types
- Function declarations
  - Include variable names
  - For short files these can be left out
  - Group/order in logical manner
- Global variables
- Function definitions in same order as declarations
- main

### C Features

- Do not mix declarations and code
- Do not use for loop initial declarations
- Use `/* */` for comments, not `//`

### Headers

- Place system/libc headers first in alphabetical order
  - If headers must be included in a specific order comment to explain
- Place local headers after an empty line

### Variables

- Global variables not used outside translation unit should be declared static
- In declaration of pointers the `*` is adjacent to variable name, not type

### Indentation

- the code is indented by 4 spaces - if you use vim to code you can set
  `:set expandtab ts=4 sts=4 sw=4`
- it's a good advice to orientate on the already available code
- if you are using `indent`, following options describe best the code style
  - `--k-and-r-style`
  - `--case-indentation4`
  - `--dont-break-function-decl-args`
  - `--dont-break-procedure-type`
  - `--dont-line-up-parentheses`
  - `--no-tabs`

## directories

    ├── doc                 documentation like manual page
    └── src                 all sources to build vimb
        ├── scripts         JavaScripts that are compiled in for various purposes
        └── webextension    Source files for the webextension

## compile and run

To inform vimb during compile time where the webextension should be loaded
from, the `RUNPREFIX` option can be set to a full qualified path to the
directory where the extension should be stored in.

To run vimb without installation you could run as a sandbox like this

    make runsandbox

This will compile and install vimb into the local _sandbox_ folder in the
project directory.

## license

Information about the license are found in the file LICENSE.

[gentoo-git]:  https://github.com/tharvik/overlay/tree/master/www-client/vimb
[gentoo]:      https://github.com/hsoft/portage-overlay/tree/master/www-client/vimb
[vimb]:        https://fanglingsu.github.io/vimb/ "Vimb - Vim like browser project page"
[mail]:        https://lists.sourceforge.net/lists/listinfo/vimb-users "vimb - mailing list"
