# vimb - the vim like browser

This is the development branch for the new webkit2 port of vimb. This branch
does not work and lags a lot of features of the webkit1 version of vimb. So
this is only meant to be the playground for the developers at the moment.

If you like to have a working vimb, please use the master branch.

## Patching and Coding style

- the code is indented by 4 spaces - if you use vim to code you can set
  `:set expandtab ts=4 sts=4 sw=4`
- the functions are sorted alphabetically within the c files
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
        ├── scripts         JavaScripts that are compiled in to various purposes
        └── webextension    Source files for the webextension

## dependencies

- webkit2gtk-4.0 >= 2.3.5

## compile and run

To inform vimb during compile time where the webextension should be loaded
from, the `RUNPREFIX` option can be set to a full qualified path to the
directory where the extension should be sotred in.

To run vimb wihtout installation you could run as a sandbox like this

    make runsandbox

This will compile and install vimb into the local _sandbox_ folder in the
project directory.

## Tasks

1. general infrastructure and built
  - [x] write make file
    - [x] allow to built as sandbox for local testing
    - [x] add a way to specify the target of the webextension shared objects
          this is now available with the `RUNPREFIX` oprion for the make
  - [x] establish communication channel between the vimb instance and the now
        required webextension (dbus)
2. migrate as many of the features of the webkit1 vimb
  - [x] starting with custom config file `--config,-c` option
  - [ ] running multiple ex-commands during startup `--cmd,-C` options
  - [ ] starting with a named profile `--profile,-p` option
  - [ ] xembed `--embed,-e` option
  - [ ] socket support `--socket,-s` and `--dump,-d` option to print the actual
        used socket path to stdout
  - [ ] kiosk-mode `--kiosk,-k`
  - [ ] allow to start vimb reading html from `stdin` by `vimb -`
  - [ ] browser modes normal, input, command, pass-through and hintmode
  - [ ] download support
  - [ ] editor command
  - [ ] external downloader
  - [ ] hinting
  - [x] searching and matching of search results
  - [x] navigation j, k, h, l, ...
  - [ ] history and history lookup
  - [ ] completion
  - [ ] HSTS
  - [ ] auto-response-header
  - [x] cookies support
  - [x] register support and `:register` command
  - [ ] read it later queue
  - [ ] show scroll indicator in statusbar as top, x%, bttom or all
        how can we get this information from webview easily?
  - [x] find a way to disable the scrollbars on the main frame
        Can be achieved by `document.documentElement.style.overflow =
        'hidden';` in _scripts.js_
  - [ ] page marks - maybe we change make them global (shared between
        instances and to work also if the page was reloaded or changed like
        the marks in vim)
  - [x] zooming
  - [x] default zoom
  - [x] yanking
  - [x] keymapping
  - [ ] URL handler
  - [x] shortcuts
  - [ ] autocommands and augroups
3. documentation
4. testing
  - [ ] write automatic test to the essential main features
  - [ ] adapt the manual test cases and add some more to avoid regressions
        before a release
  - [ ] write new test cases for essential things like mode switching of vimb
        when clicking form fields or tabbing over them.
5. new features and changed behaviour
  - [ ] try to use the webkit2 feature of running multiple pages with related
        view instance `webkit_web_view_new_with_related_view`
  - [ ] allow setting of different scopes, global and instance (new feature)
  - [ ] remove the settings related to the gui like `status-color-bg` this was
        only a hack and is not recommended for new gtk3 applications. the
        color and font settings should be setup by css instead.
  - [ ] webkit2 does not provide the view mode `source` so maybe this is going
        to be removed together with the `gf` keybinding or we find a simple
        workaround for this

# license

Information about the license are found in the file LICENSE.
