/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2013 Daniel Carl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#ifndef _CONFIG_H
#define _CONFIG_H


/* features */
#define FEATURE_COOKIE
#define FEATURE_SEARCH_HIGHLIGHT
#define FEATURE_NO_SCROLLBARS
#define FEATURE_GTK_PROGRESSBAR
#define FEATURE_TITLE_IN_COMPLETION


/* time in seconds after that message will be removed from inputbox if the
 * message where only temporary */
#define MESSAGE_TIMEOUT              5

#define SETTING_MAX_CONNS           25
#define SETTING_MAX_CONNS_PER_HOST   5

#define MAXIMUM_HINTS              500

/* remove this if the bookmark file fits to the new format "URL<tab>title of page<tab>tag1 tag2" */
#define ANNOUNCEMENT "\
<!DOCTYPE html><html lang=\"en\" dir=\"ltr\"><head> \
<title>Bookmark file format changed</title> \
<style media=\"all\">body{width:60em;margin:0 auto}h1{color:#f90}code{display:block;margin:0 2em}</style> \
</head> \
<body> \
<h1>Bookmark file format changed</h1> \
<p>The history file and bookmarks file will now holds also the page titles to serve them in the completion list. This will break previous bookmark files.</p> \
<p>The parts of the history and bookmark file are now separated by <b>\\t</b> char like the cookie file.</p> \
<code> \
// old format<br/> \
http://very-long.uri/path.html tag1 tag2<br/> \
http://very-long.uri/path-no-tags.html<br/> \
</code> \
<code> \
// new format<br/> \
http://very-long.uri/path.html<b>\\t</b>title of the page<b>\\t</b>tag1 tag2<br/> \
http://very-long.uri/path-no-title.html<b>\\t\\t</b>tag1 tag2<br/> \
http://very-long.uri/path-no-title-and-no-tags.html<br/> \
</code> \
<h2>What to do?</h2> \
<p> \
If you have no entries in you bookmark you must edit them to fit the new format. This can be done with following 'sed' command. </p> \
<p><b>Don't forget to backup the bookmark file before running the command.</b></p> \
<code> \
// replace the first space in earch line of file to <b>\\t\\t</b> if the line haven't already a \t in it.<br/> \
sed -i -e '/\\t/!{s/ /\\t\\t/}' bookmark<br/> \
</code> \
<p>If you had no entries in the bookmark file or you changed it successfully, you should remove the '#define ANNOUNCEMENT' from <i>config.h</i> file and recompile vimb.</p> \
</body> \
</html>"

#endif /* end of include guard: _CONFIG_H */
