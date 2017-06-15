# Contribute

This document contains guidelines for contributing to vimb, as well as useful
hints when doing so.

## Goals

Getting a light, fast and keyboard-driven browser that is easy to use for
those users familiar with vim.

- Provide powerful knobs allowing the user to tweak vimb to fit the own needs
  and usecases.
- Add only knobs/features that do not do what other knobs do. In this point
  vimb is in contrast to vim.
- If there are two colliding features we should pick the mightier one, or that
  which need less code or resources.

## Find something to work on

If you are interested to contribute to vimb it's a good idea to check if
someone else is already working on. I would be a shame when you work was for
nothing.

If you have some ideas how to improve vimb by new features or to simplify it.
Write an issue so that other contributors can comment/vote on it or help you
with it.

If you do not want to write code you are pretty welcome to update
[documentation][issue-doc] or to argue and vote for features and [request for
comments][issue-rfc].

## Communication

If you want to discuss some feature or provide some suggestion that can't be
done very well with the github issues. You should use the [mailing list][mail]
for this purpose. Else it's a good decision to use the features provided by
github for that.

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
        ├── scripts         JavaScripts and CSS that are compiled in for various purposes
        └── webextension    Source files for the webextension

## compile and run

To inform vimb during compile time where the webextension should be loaded
from, the `RUNPREFIX` option can be set to a full qualified path to the
directory where the extension should be stored in.

To run vimb without installation you could run as a sandbox like this

    make runsandbox

This will compile and install vimb into the local _sandbox_ folder in the
project directory.

[mail]:        https://lists.sourceforge.net/lists/listinfo/vimb-users "vimb - mailing list"
[issue-doc]:   https://github.com/fanglingsu/vimb/labels/component%3A%20docu
[issue-rfc]:   https://github.com/fanglingsu/vimb/labels/rfc
