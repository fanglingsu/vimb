# Changes in vimb

## [unreleased]

### Changed

* Syntax for the font related gui settings has be changed.
  Fonts have to be given as `[ font-style | font-variant | font-weight | font-stretch ]? font-size font-family`
  Example `set input-font-normal=bold 10pt "DejaVu Sans Mono"` instead of
  previous `set input-fg-normal=DejaVu Sans Mono Bold 10` 

### Removed

* `FEATURE_COOKIE` precompiler flag was removed bcause compiling without cookie
  support does not bring any real benefit

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

[unreleased]: https://github.com/fanglingsu/vimb/compare/2.11...HEAD
[2.11]: https://github.com/fanglingsu/vimb/compare/2.10...2.11
