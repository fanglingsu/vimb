/**
 * vimp - a webkit based vim like browser.
 *
 * Copyright (C) 2012 Daniel Carl
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

#ifndef DOM_H
#define DOM_H

#include <webkit/webkit.h>

typedef struct {
    gulong left;
    gulong right;
    gulong top;
    gulong bottom;
} DomBoundingRect;

void dom_check_auto_insert(void);
void dom_element_set_style(WebKitDOMElement* element, const gchar* style);
void dom_element_style_set_property(WebKitDOMElement* element, const gchar* property, const gchar* style);
gboolean dom_element_is_visible(WebKitDOMDOMWindow* win, WebKitDOMElement* element);
DomBoundingRect dom_elemen_get_bounding_rect(WebKitDOMElement* element);

#endif /* end of include guard: DOM_H */
