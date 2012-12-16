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

// Types
#define Document       WebKitDOMDocument
#define Body           WebKitDOMHTMLBodyElement
#define Head           WebKitDOMHTMLHeadElement
#define Window         WebKitDOMDOMWindow
#define NodeList       WebKitDOMNodeList
#define Node           WebKitDOMNode
#define Style          WebKitDOMCSSStyleDeclaration
#define HtmlElement    WebKitDOMHTMLElement
#define Element        WebKitDOMElement
#define CssDeclaration WebKitDOMCSSStyleDeclaration
#define Event          WebKitDOMEvent
#define EventTarget    WebKitDOMEventTarget

// style 
#define style_compare_property(style, name, value)    (!strcmp(webkit_dom_css_style_declaration_get_property_value(style, name), value))

typedef struct {
    gulong left;
    gulong right;
    gulong top;
    gulong bottom;
} DomBoundingRect;

void dom_check_auto_insert(void);
void dom_element_set_style(Element* element, const gchar* format, ...);
void dom_element_style_set_property(Element* element, const gchar* property, const gchar* style);
gboolean dom_element_is_visible(Window* win, Element* element);
DomBoundingRect dom_elemen_get_bounding_rect(Element* element);
void dom_dispatch_mouse_event(Document* doc, Element* element, gchar* type, gushort button);

#endif /* end of include guard: DOM_H */
