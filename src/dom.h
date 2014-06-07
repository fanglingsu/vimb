/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2014 Daniel Carl
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

#ifndef _DOM_H
#define _DOM_H

#include <webkit/webkit.h>

/* Types */
#define Document            WebKitDOMDocument
#define HtmlElement         WebKitDOMHTMLElement
#define Element             WebKitDOMElement
#define Node                WebKitDOMNode
#define Event               WebKitDOMEvent
#define EventTarget         WebKitDOMEventTarget
#define HtmlInputElement    WebKitDOMHTMLInputElement
#define HtmlTextareaElement WebKitDOMHTMLTextAreaElement

void dom_check_auto_insert(WebKitWebView *view);
void dom_clear_focus(WebKitWebView *view);
gboolean dom_focus_input(WebKitWebView *view);
gboolean dom_is_editable(Element *element);
Element *dom_get_active_element(WebKitWebView *view);
const char *dom_editable_element_get_value(Element *element);
void dom_editable_element_set_value(Element *element, const char *value);
void dom_editable_element_set_disable(Element *element, gboolean value);

#endif /* end of include guard: _DOM_H */
