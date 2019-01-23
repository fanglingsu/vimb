/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2018 Daniel Carl
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

#ifndef _EXT_DOM_H
#define _EXT_DOM_H

#include <glib.h>
#include <webkitdom/webkitdom.h>

gboolean ext_dom_is_editable(WebKitDOMElement *element);
gboolean ext_dom_focus_input(WebKitDOMDocument *doc);
char *ext_dom_editable_get_value(WebKitDOMElement *element);

#endif /* end of include guard: _EXT-DOM_H */
