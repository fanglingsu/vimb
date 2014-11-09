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

#include "config.h"
#include "main.h"
#include "dom.h"
#include "mode.h"

extern VbCore vb;

static gboolean element_is_visible(WebKitDOMDOMWindow* win, WebKitDOMElement* element);
static gboolean auto_insert(Element *element);
static gboolean editable_focus_cb(Element *element, Event *event);
static Element *get_active_element(Document *doc);


void dom_check_auto_insert(WebKitWebView *view)
{
    Document *doc   = webkit_web_view_get_dom_document(view);
    Element *active = get_active_element(doc);

    if (vb.config.strict_focus || !auto_insert(active)) {
        /* if the strict-focus is on also blur the possible active element */
        if (vb.config.strict_focus) {
            dom_clear_focus(view);
        }
        /* the focus was not set automatically - add event listener to track
         * focus events on the document */
        HtmlElement *element = webkit_dom_document_get_body(doc);
        if (!element) {
            element = WEBKIT_DOM_HTML_ELEMENT(webkit_dom_document_get_document_element(doc));
        }
        webkit_dom_event_target_add_event_listener(
            WEBKIT_DOM_EVENT_TARGET(element), "focus", G_CALLBACK(editable_focus_cb), false, NULL
        );
    }
}

/**
 * Remove focus from active and editable elements.
 */
void dom_clear_focus(WebKitWebView *view)
{
    Element *active = dom_get_active_element(view);
    if (active) {
        webkit_dom_element_blur(active);
    }
}

/**
 * Set focus to the first found editable element and returns if a element was
 * found to focus.
 */
gboolean dom_focus_input(WebKitWebView *view)
{
    gboolean found = false;
    WebKitDOMNode *html, *node;
    WebKitDOMDocument *doc;
    WebKitDOMDOMWindow *win;
    WebKitDOMNodeList *list;
    WebKitDOMXPathNSResolver *resolver;
    WebKitDOMXPathResult* result;

    doc  = webkit_web_view_get_dom_document(view);
    win  = webkit_dom_document_get_default_view(doc);
    list = webkit_dom_document_get_elements_by_tag_name(doc, "html");
    if (!list) {
        return false;
    }

    html     = webkit_dom_node_list_item(list, 0);
    resolver = webkit_dom_document_create_ns_resolver(doc, html);
    if (!resolver) {
        return false;
    }

    result = webkit_dom_document_evaluate(
        doc, "//input[not(@type) or @type='text' or @type='password']|//textarea",
        html, resolver, 0, NULL, NULL
    );
    if (!result) {
        return false;
    }
    while ((node = webkit_dom_xpath_result_iterate_next(result, NULL))) {
        if (element_is_visible(win, WEBKIT_DOM_ELEMENT(node))) {
            webkit_dom_element_focus(WEBKIT_DOM_ELEMENT(node));
            found = true;
            break;
        }
    }
    g_object_unref(list);

    return found;
}

/**
 * Indicates if the given dom element is an editable element like text input,
 * password or textarea.
 */
gboolean dom_is_editable(Element *element)
{
    gboolean result = false;
    char *tagname, *type;

    if (!element) {
        return result;
    }

    tagname = webkit_dom_element_get_tag_name(element);
    type    = webkit_dom_element_get_attribute(element, "type");
    /* element is editable if it's a text area or input with no type, text or
     * pasword */
    if (!g_ascii_strcasecmp(tagname, "textarea")) {
        result = true;
    } else if (!g_ascii_strcasecmp(tagname, "input")
        && (!*type || !g_ascii_strcasecmp(type, "text") || !g_ascii_strcasecmp(type, "password"))
    ) {
        result = true;
    } else {
        result = false;
    }
    g_free(tagname);
    g_free(type);

    return result;
}

Element *dom_get_active_element(WebKitWebView *view)
{
    return get_active_element(webkit_web_view_get_dom_document(view));
}

const char *dom_editable_element_get_value(Element *element)
{
    const char *value = NULL;

    if (WEBKIT_DOM_IS_HTML_INPUT_ELEMENT((HtmlInputElement*)element)) {
        value = webkit_dom_html_input_element_get_value((HtmlInputElement*)element);
    } else {
        /* we should check WEBKIT_DOM_IS_HTML_TEXT_AREA_ELEMENT, but this
         * seems to return alway false */
        value = webkit_dom_html_text_area_element_get_value((HtmlTextareaElement*)element);
    }

    return value;
}

void dom_editable_element_set_value(Element *element, const char *value)
{
    if (WEBKIT_DOM_IS_HTML_INPUT_ELEMENT(element)) {
        webkit_dom_html_input_element_set_value((HtmlInputElement*)element, value);
    } else {
        webkit_dom_html_text_area_element_set_value((HtmlTextareaElement*)element, value);
    }
}

void dom_editable_element_set_disable(Element *element, gboolean value)
{
    if (WEBKIT_DOM_IS_HTML_INPUT_ELEMENT(element)) {
        webkit_dom_html_input_element_set_disabled((HtmlInputElement*)element, value);
    } else {
        webkit_dom_html_text_area_element_set_disabled((HtmlTextareaElement*)element, value);
    }
}

static gboolean element_is_visible(WebKitDOMDOMWindow* win, WebKitDOMElement* element)
{
    gchar* value = NULL;

    WebKitDOMCSSStyleDeclaration* style = webkit_dom_dom_window_get_computed_style(win, element, "");
    value = webkit_dom_css_style_declaration_get_property_value(style, "visibility");
    if (value && g_ascii_strcasecmp(value, "hidden") == 0) {
        return false;
    }
    value = webkit_dom_css_style_declaration_get_property_value(style, "display");
    if (value && g_ascii_strcasecmp(value, "none") == 0) {
        return false;
    }

    return true;
}

static gboolean auto_insert(Element *element)
{
    /* don't change mode if we are in pass through mode */
    if (vb.mode->id != 'p' && dom_is_editable(element)) {
        mode_enter('i');

        return true;
    }
    return false;
}

static gboolean editable_focus_cb(Element *element, Event *event)
{
    webkit_dom_event_target_remove_event_listener(
        WEBKIT_DOM_EVENT_TARGET(element), "focus", G_CALLBACK(editable_focus_cb), false
    );
    if (vb.mode->id != 'i') {
        EventTarget *target = webkit_dom_event_get_target(event);
        auto_insert((void*)target);
    }
    return false;
}

static Element *get_active_element(Document *doc)
{
    char *tagname;
    Document *d = NULL;
    Element *active, *result = NULL;

    active = webkit_dom_html_document_get_active_element((void*)doc);
    if (!active) {
        return result;
    }
    tagname = webkit_dom_element_get_tag_name(active);

    if (!g_strcmp0(tagname, "FRAME")) {
        d = webkit_dom_html_frame_element_get_content_document(WEBKIT_DOM_HTML_FRAME_ELEMENT(active));
        result = get_active_element(d);
    } else if (!g_strcmp0(tagname, "IFRAME")) {
        d = webkit_dom_html_iframe_element_get_content_document(WEBKIT_DOM_HTML_IFRAME_ELEMENT(active));
        result = get_active_element(d);
    }
    g_free(tagname);

    if (result) {
        return result;
    }

    return active;
}
