# Changes in vimb

## [unreleased]

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
  hintkeys         - hint-keys
  cursivfont       - cursiv-font
  defaultfont      - default-font
  fontsize         - font-size
  minimumfontsize  - minimum-font-size
  monofont         - monospace-font
  monofontsize     - monospace-font-size
  offlinecache     - offline-cache
  sansfont         - sans-serif-font
  seriffont        - serif-font
  xssauditor       - xss-auditor
  userscripts      - user-scripts
  scrollstep       - scroll-step
  statusbar        - status-bar
  ```

### Removed

* There where many features removed during the webkit2 migration.
  TODO list removed features and if they will be added again

---

## [2.12] - 2017-04-11

### Added

* Queueing of key events - fixes swalled chars in case of some imap bindings
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

[unreleased]: https://github.com/fanglingsu/vimb/compare/2.12...webkit2
[2.12]: https://github.com/fanglingsu/vimb/compare/2.11...2.12
[2.11]: https://github.com/fanglingsu/vimb/compare/2.10...2.11
