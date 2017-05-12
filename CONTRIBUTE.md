# Contribute

This document contains guidelines for contributing to vimb, as well as useful
hints when doing so.

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

