# vimb
Vimb is a web browser that behaves like the Vimprobable but with some
paradigms from dwb and hopefully a cleaner code base. The goal of Vimb is to
build a completely keyboard-driven, efficient and pleasurable
browsing-experience with low memory and cpu usage.

More information and some screenshots of vimb browser in action can be found on
the [vimb project page][vimb].

## changedd bookmark and history file format
The history file and bookmarks file will now holds also the page titles to
serve them in the completion list. This changed the file format and will break
previous bookmark files. The history file will still be usable also with the
latest changes, but the bookmark file must be adapted to the new file format.

```
// old format
http://very-long.uri/path.html tag1 tag2
http://very-long.uri/path-no-tags.html
```

```
// new format
http://very-long.uri/path.html<tab>title of the page<tab>tag1 tag2
http://very-long.uri/path-no-title.html<tab><tab>tag1 tag2
http://very-long.uri/path-no-title-and-no-tags.html
```

The parts of the history and bookmark file are now separated by `<tab>` or
`\t` char like the cookie file. Following `sed` command can be used to apply
the required changes to the bookmark file. **Don't forget to backup the file
before running the command**

```
// replace the first space in earch line of file to \t\t
// if the line haven't already a \t in it.
sed -i -e '/\t/!{s/ /\t\t/}' bookmark
```

## features
- vim-like modal
- vim-like keybindings
- nearly every configuration can be changed on runtime with `:set varname=value`
  - allow to inspect the current set values of variables `:set varname?`
  - allow to toggle boolean variables with `:set varname!`
- keybindings for each browser mode assignable
- the center of `vimb` are the commands that can be called from inputbox or
  via keybinding
- history for
  - commands
  - search queries
  - urls
- completions for
  - commands
  - urls
  - variable names of settings
  - search-queries
- hinting - marks links, form fields and other clickable elements to be
  clicked, opened or inspected
- webinspector that opens ad the bottom of the browser window like in some
  other fat browsers
- ssl validation against ca-certificate file
- custom configuration files
- tagged bookmarks
- open input or textarea with configurable external editor
- user defined URL-shortcuts with placeholders

## dependencies
- libwebkit >=1.3.10
- libgtk+-2.0
- libsoup-2.4

# license
Information about the license are found in the file LICENSE.

# mailing list
- feature requests, issues and patches can be discussed on the [mailing list][mail]

[vimb]: http://fanglingsu.github.io/vimb/ "vimb - vim-like webkit browser project page"
[mail]: https://lists.sourceforge.net/lists/listinfo/vimb-users "vimb - mailing list"
