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

#include "dom.h"
#include "main.h"

static gboolean dom_auto_insert(WebKitDOMElement* element);
static gboolean dom_editable_focus_cb(WebKitDOMElement* element, WebKitDOMEvent* event);
static gboolean dom_is_editable(WebKitDOMElement* element);
static WebKitDOMElement* dom_get_active_element(WebKitDOMDocument* doc);


void dom_check_auto_insert(void)
{
    WebKitDOMDocument* doc   = webkit_web_view_get_dom_document(vp.gui.webview);
    WebKitDOMElement* active = dom_get_active_element(doc);
    if (!dom_auto_insert(active)) {
        WebKitDOMHTMLElement *element = webkit_dom_document_get_body(doc);
        if (!element) {
            element = WEBKIT_DOM_HTML_ELEMENT(webkit_dom_document_get_document_element(doc));
        }
        webkit_dom_event_target_add_event_listener(
            WEBKIT_DOM_EVENT_TARGET(element), "focus", G_CALLBACK(dom_editable_focus_cb), true, NULL
        );
    }
}

void dom_element_set_style(WebKitDOMElement* element, const gchar* style)
{
    WebKitDOMCSSStyleDeclaration* css = webkit_dom_element_get_style(element);
    if (css) {
        webkit_dom_css_style_declaration_set_css_text(css, style, NULL);
    }
}

void dom_element_style_set_property(WebKitDOMElement* element, const gchar* property, const gchar* style)
{
    WebKitDOMCSSStyleDeclaration* css = webkit_dom_element_get_style(element);
    if (css) {
        webkit_dom_css_style_declaration_set_property(css, property, style, "", NULL);
    }
}

gboolean dom_element_is_visible(WebKitDOMDOMWindow* win, WebKitDOMElement* element)
{
    gchar* value = NULL;

    WebKitDOMCSSStyleDeclaration* style = webkit_dom_dom_window_get_computed_style(win, element, "");
    value = webkit_dom_css_style_declaration_get_property_value(style, "visibility");
    if (value && g_ascii_strcasecmp(value, "hidden") == 0) {
        return FALSE;
    }
    value = webkit_dom_css_style_declaration_get_property_value(style, "display");
    if (value && g_ascii_strcasecmp(value, "none") == 0) {
        return FALSE;
    }

    return TRUE;
}

DomBoundingRect dom_elemen_get_bounding_rect(WebKitDOMElement* element)
{
    DomBoundingRect rect;
    rect.left   = webkit_dom_element_get_offset_left(element);
    rect.top    = webkit_dom_element_get_offset_top(element);
    rect.right  = rect.left + webkit_dom_element_get_offset_width(element);
    rect.bottom = rect.top + webkit_dom_element_get_offset_height(element);

    return rect;
}

static gboolean dom_auto_insert(WebKitDOMElement* element)
{
    if (dom_is_editable(element)) {
        vp_set_mode(VP_MODE_INSERT, FALSE);
        return TRUE;
    }
    return FALSE;
}

static gboolean dom_editable_focus_cb(WebKitDOMElement* element, WebKitDOMEvent* event)
{
    webkit_dom_event_target_remove_event_listener(
        WEBKIT_DOM_EVENT_TARGET(element), "focus", G_CALLBACK(dom_editable_focus_cb), true
    );
    if (GET_CLEAN_MODE() != VP_MODE_INSERT) {
        WebKitDOMEventTarget* target = webkit_dom_event_get_target(event);
        dom_auto_insert((void*)target);
    }
    return FALSE;
}

/**
 * Indicates if the given dom element is an editable element like text input,
 * password or textarea.
 */
static gboolean dom_is_editable(WebKitDOMElement* element)
{
    if (!element) {
        return FALSE;
    }

    gchar* tagname = webkit_dom_node_get_node_name(WEBKIT_DOM_NODE(element));
    if (!g_strcmp0(tagname, "TEXTAREA")) {
        return TRUE;
    }
    if (!g_strcmp0(tagname, "INPUT")) {
        gchar *type = webkit_dom_element_get_attribute((void*)element, "type");
        if (!g_strcmp0(type, "text") || !g_strcmp0(type, "password")) {
            return TRUE;
        }
    }
    return FALSE;
}

static WebKitDOMElement* dom_get_active_element(WebKitDOMDocument* doc)
{
    WebKitDOMDocument* d     = NULL;
    WebKitDOMElement* active = webkit_dom_html_document_get_active_element((void*)doc);
    gchar* tagname           = webkit_dom_element_get_tag_name(active);

    if (!g_strcmp0(tagname, "FRAME")) {
        d   = webkit_dom_html_frame_element_get_content_document(WEBKIT_DOM_HTML_FRAME_ELEMENT(active));
        return dom_get_active_element(d);
    }
    if (!g_strcmp0(tagname, "IFRAME")) {
        d   = webkit_dom_html_iframe_element_get_content_document(WEBKIT_DOM_HTML_IFRAME_ELEMENT(active));
        return dom_get_active_element(d);
    }
    return active;
}
