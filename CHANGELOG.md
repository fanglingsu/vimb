# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [3.6.0] - 2020-01-02
### Added
* `:cleardata [listOfDataTypes] [timeSpan]` command to clear various types of
  stored website data modified in the last _timeSpan_.
* Setting `hint-match-element` to allow to disable the hinting to filter hints
  by the elements text content. This is useful if 'hint-keys' are set the
  chars instead of numbers.
* New autocmd event `LoadStarting` to run auto commands before the first page
  content is loaded (Thanks to Patrick Steinhardt).
* Setting `geolocation` with values ('ask', 'always' and 'never') to allow the
  user to permit or disable geolcation requests by default (Thanks to Alva).
* Setting `dark-mode` to switch the webview into dark mode, which might be
  picked up by pages media query to setup dark styling (Thanks to Alva).
* Option `--cmd, -C` to run ex commands on startup.
### Changed
### Fixed
### Removed
* `:clearcache` was removed in favor of more advanced `:cleardata` command.
  The previous behaviour of `:clearcache` could be replaces by
  `:cleardata memory-cache,disk-cache`.

## [3.5.0] - 2019-07-29
### Added
* Add external download command #543 #348.
* Added ephemeral mode by new option `--incognito` #562.
### Changed
* Hinting shows the current focused elements URI in the statusbar.
* Show error if printing with `:hardcopy` fails #564.
### Fixed
* Fixed compilation if source is not in a git repo (Thanks to Patrick Steinhardt).
* Fixed partial hidden hint labels on top of screen.
* Fix segfault on open in new tabe from context menu #556.
* Fix "... (null)" shown in title during url sanitization.
### Removed
* Setting `private-browsing` was removed in favor of `--incognito` option.

## [3.4.0] - 2019-03-26
### Added
* Allow to show video in fullscreen, without statusbar and inputbox, if requested.
* Added option `--no-maximize` to no start with maximized window #483.
* New setting `prevent-newwindow` to enforce opening links into same window
  even if they are crafted by `target="_blank"` or using `window.open(...)` #544.
### Changed
* Increased min required webkit version to 2.20.x.
* Use man page date instead of build date ot make reproducible builds.
* URLs shown on statusbar and title are now shown as punicode if they contain
  homographs.
### Fixed
* Fix out-of-bounds buffer access in parse_command (Thanks to Sören Tempel) #529.
* Fixed none shown hint labels by Content-Security-Policy headers #531.
* Fixed segfault on JavaScript `window.close()` call #537.
* Fixed no char inserted in input mode after timeout and imap/inoremap
  candidate #546.

## [3.3.0] - 2018-11-06

### Added
* Allow to change following webkit settings during runtime
  * allow-file-access-from-file-urls
  * allow-universal-access-from-file-urls
* Added `#define CHECK_WEBEXTENSION_ON_STARTUP 1` to config.def.h to enable
  checks during runtime if the webextension file could be found. Hope that
  this helps user to fix compile/installation issues easier.
* Re-Added support for page marks to jump around within long single pages by
  using names marks.
  Set a marks by `m{a-z}` in normal mode. Jump to marks by `'{a-z}`.
* Re-Added `gf` to show page source (Thanks to Leonardo Taccari) #361.
  Webkit2 does not allow to show the page in the source view mode so the `gf`
  writes the HTML to a temporary files and opens it in the editor configured
  by `:set editor-command=...`
### Changed
* New created files in `$XDG_CONFIG_HOME/vimb` are generated with `0600`
  permission to prevent cookies be observed on multi users systems. Existing
  files are not affected by this change. It's a good advice to change the
  permission of all the files in `$XDG_CONFIG_HOME/vimb` to `0600` by
  hand.
### Fixed
* Fixed missing dependency in Makefile which possibly caused broken builds
  (Thanks to Patrick Steinhardt).
* Fixed weird scroll position values shown in scroll indicator on some pages #501.
* Fixed wrong hint label position on xkcd.com #506.
* Fixed wrong hint label position in case of hints within iframes.

## [3.2.0] - 2018-06-16

### Added
* Allow basic motion commands for hinting too.
* Show the numbers of search matches in status bar.
* Show dialog if the page makes a permission request e.g. geolocation to allow
  the user to make a decision.
* new Setting `show-titlebar` to toggle window decorations.

### Changed
* Use sqlite as cookie storage #470 to prevent cookies lost on running many
  vimb instances.
* Start vimb with maximized window #483.
* Hints are now styled based on the vimbhint attributes. The old additional set
  classes are not set anymore to the hints. So customized css for the hints have
  to be adapted to this.
* Element ID is stored in case the editor was spawned. So it's now possible to
  start the editor, load another page, come back and paste the editor contents
  (thanks to Sven Speckmaier).

### Fixed
* Fixed none cleaned webextension object files on `make clean`.
* Remove none used gui styling for completion.

### Removed
* Removed webkit1 combat code.

## [3.1.0] - 2017-12-18

### Added

* Added completion of bookmarked URIs for `:bmr` to allow to easily remove
  bookmarks without loading the page first.
* Refresh hints after scrolling the page or resizing the window which makes
  extended hint mode more comfortable.
* Reintroduce the automatic commands from vimb2. An automatic command is
  executed automatically in response to some event, such as a URI being opened.

### Changed

* Number of webprocesses in no longer limited to one.
* Treat hint label generation depending on the first hint-key char.
  If first char is '0' generate numeric style labels else the labels start with
  the first char (thanks to Yoann Blein).
  * `hint-keys=0123` -> `1 2 3 10 11 12 13`
  * `hint-keys=asdf` -> `a s d f aa as ad af`
* Show versions of used libs on `vimb --bug-info` and the extension directory
  for easier issue investigation.
* During hinting JavaScript is enabled and reset to it's previous setting after
  hinting is done might be security relevant.
* Allow extended hints mode also for open `g;o` to allow the user to toggle
  checkboxes and radiobuttons of forms.
* Rename `hint-number-same-length` into `hint-keys-same-length` for consistency.
* Search is restarted on pressing `n` or `N` with previous search query if no
  one was given (thanks to Yoann Blein).

### Fixed

* Deduced min required webkit version 2.16.x -> 2.8.x to compile vimb also on
  older systems.
* Fixed undeleted desktop file on `make uninstall`.
* Fixed window not redrawn properly in case vimb was run within tabbed.
* Fixed cursor appearing in empty inputbox on searching in case a normal mode
  command was used that switches vimb into command mode like 'T' or ':'.
* Fixed hint labels never started by the first char of the 'hint-keys'.
* Fixed items where added to history even when `history-max-items` is set to 0
  (thanks to Patrick Steinhardt).
* Fixed hinting caused dbus timeout on attempt to open URI with location hash.
* Fixed wrong scroll position shown in the right of the statusbar on some pages.
* Fixed vimb keeping in normal mode when HTTP Authentication dialog is shown.
* Fixed password show in title bar and beeing written to hisotry in case the
  pssword was given by URI like https://user:password@host.tdl.

## [3.0-alpha] - 2017-05-27

### Changed

* completely rebuild of vimb on webkit2 api.
* Syntax for the font related gui settings has be changed.
  Fonts have to be given as `[ font-style | font-variant | font-weight | font-stretch ]? font-size font-family`
  Example `set input-font-normal=bold 10pt "DejaVu Sans Mono"` instead of
  previous `set input-fg-normal=DejaVu Sans Mono Bold 10`
* Renames some settings to consequently use dashed setting names. Following
  settings where changed.
  ```
  previous setting - new setting name
  --------------------------------------
  cursivfont       - cursiv-font
  defaultfont      - default-font
  fontsize         - font-size
  hintkeys         - hint-keys
  minimumfontsize  - minimum-font-size
  monofont         - monospace-font
  monofontsize     - monospace-font-size
  offlinecache     - offline-cache
  useragent        - user-agent
  sansfont         - sans-serif-font
  scrollstep       - scroll-step
  seriffont        - serif-font
  statusbar        - status-bar
  userscripts      - user-scripts
  xssauditor       - xss-auditor
  ```

### Removed

* There where many features removed during the webkit2 migration. That will
  hopefully be added again soon.
  * auto-response-headers
  * autocommands and augroups
  * external downloader
  * HSTS
  * kiosk mode
  * multiple ex commands on startup via `--cmd, -C`
  * page marks
  * prevnext
  * showing page source via `gF` this viewtype is not supported by webkit
    anymore.
  * socket support

---

## [2.12] - 2017-04-11

### Added

* Queueing of key events - fixes swallowed chars in case of some imap bindings
  #258 (thanks to Michael Mackus)
* Allow to disable xembed by `FEATURE_NO_XEMBED` to compile on wayland only
  platforms (thanks to Patrick Steinhardt)
* Custom default_zoom setting disables HIGH_DPI logic (thanks to Robert Timm)
* Allow link activation from search result via `<CR>` #131

### Changed

* Allow shortcuts without parameters #329
* Write soup cache to disk after each page load to allow other instances to
  pick this up.
* Use the beginning position of links for hinting (thanks to Yutao Yuan)

### Fixed

* Fix path expansion to accept only valid POSIX.1-2008 usernames (thanks to
  Manzur Mukhitdinov)
* Fix default previouspattern (thanks to Nicolas Porcel)

## [2.11] - 2015-12-17

### Added

* Added hint-number-same-length option
* VERBOSE flag to Makefile to toggle verbose make on
* `<Esc>` removes selections in normal mode
* Support for multiple configuration profiles. New parameter `-p` or
  `--profile`
* Adds support for contenteditable attribute as input mode trigger
  [#237](https://github.com/fanglingsu/vimb/issues/237)
* Added `^` as normal mode alias of `0`
  [#236](https://github.com/fanglingsu/vimb/issues/236)
* Added :source command to source a config file
* Added path completion for :save command too
* Added closed-max-items option to allow to store more than one closed page

### Changed

* Set only required CFLAGS
* Replaced `-Wpedantic` with `-pedantic` CFLAGS for older gcc versions
* Check for focused editable element as soon as possible
* Do not blur the focused element after alt-tabbing
* Show typed text as last completion entry to easily change it
  [#253](https://github.com/fanglingsu/vimb/issues/253)

### Fixed

* Fixed [#224](https://github.com/fanglingsu/vimb/issues/224): Wrong URL and
  titles shown in case one or more pages could not be loaded.
* Fixed Makefile install target using -D
* Fixed [#232](https://github.com/fanglingsu/vimb/issues/232): Fixed misplaced
  hint labels on some sites
* Fixed [#235](https://github.com/fanglingsu/vimb/issues/235): Randomly reset
  cookie file
* Fixed none POSIX `echo -n` call

[Unreleased]: https://github.com/fanglingsu/vimb/compare/3.6.0...master
[3.6.0]: https://github.com/fanglingsu/vimb/compare/3.5.0...3.6.0
[3.5.0]: https://github.com/fanglingsu/vimb/compare/3.4.0...3.5.0
[3.4.0]: https://github.com/fanglingsu/vimb/compare/3.3.0...3.4.0
[3.3.0]: https://github.com/fanglingsu/vimb/compare/3.2.0...3.3.0
[3.2.0]: https://github.com/fanglingsu/vimb/compare/3.1.0...3.2.0
[3.1.0]: https://github.com/fanglingsu/vimb/compare/3.0-alpha...3.1.0
[3.0-alpha]: https://github.com/fanglingsu/vimb/compare/2.12...3.0-alpha
[2.12]: https://github.com/fanglingsu/vimb/compare/2.11...2.12
[2.11]: https://github.com/fanglingsu/vimb/compare/2.10...2.11
