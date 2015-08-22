/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2015 Daniel Carl
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

extern VbCore vb;

static gboolean element_is_visible(WebKitDOMDOMWindow* win, WebKitDOMElement* element);
static gboolean auto_insert(Element *element);
static gboolean editable_blur_cb(Element *element, Event *event);
static gboolean editable_focus_cb(Element *element, Event *event);
static Element *get_active_element(Document *doc);


void dom_install_focus_blur_callbacks(Document *doc)
{
    HtmlElement *element = webkit_dom_document_get_body(doc);

    if (!element) {
        element = WEBKIT_DOM_HTML_ELEMENT(webkit_dom_document_get_document_element(doc));
    }
    if (element) {
        webkit_dom_event_target_add_event_listener(
            WEBKIT_DOM_EVENT_TARGET(element), "blur", G_CALLBACK(editable_blur_cb), true, NULL
        );
        webkit_dom_event_target_add_event_listener(
            WEBKIT_DOM_EVENT_TARGET(element), "focus", G_CALLBACK(editable_focus_cb), true, NULL
        );
    }
}

void dom_check_auto_insert(Document *doc)
{
    Element *active = webkit_dom_html_document_get_active_element(WEBKIT_DOM_HTML_DOCUMENT(doc));

    if (active) {
        if (!vb.config.strict_focus) {
            auto_insert(active);
        } else if (vb.mode->id != 'i') {
            /* If strict-focus is enabled and the editable element becomes
             * focus, we explicitely remove the focus. But only if vimb isn't
             * in input mode at the time. This prevents from leaving input
             * mode that was started by user interaction like click to
             * editable element, or the 'gi' normal mode command. */
            webkit_dom_element_blur(active);
        }
    }
    dom_install_focus_blur_callbacks(doc);
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
 * Give the focus to element.
 */
void dom_give_focus(Element *element)
{
    webkit_dom_element_focus(element);
}

/**
 * Find the first editable element and set the focus on it and enter input
 * mode.
 * Returns true if there was an editable element focused.
 */
gboolean dom_focus_input(Document *doc)
{
    WebKitDOMNode *html, *node;
    WebKitDOMDOMWindow *win;
    WebKitDOMNodeList *list;
    WebKitDOMXPathNSResolver *resolver;
    WebKitDOMXPathResult* result;
    Document *frame_doc;
    guint i, len;

    win  = webkit_dom_document_get_default_view(doc);
    list = webkit_dom_document_get_elements_by_tag_name(doc, "html");
    if (!list) {
        return false;
    }

    html = webkit_dom_node_list_item(list, 0);
    g_object_unref(list);

    resolver = webkit_dom_document_create_ns_resolver(doc, html);
    if (!resolver) {
        return false;
    }

    /* Use translate to match xpath expression case insensitive so that also
     * intput filed of type="TEXT" are matched. */
    result = webkit_dom_document_evaluate(
        doc, "//input[not(@type) "
        "or translate(@type,'ETX','etx')='text' "
        "or translate(@type,'ADOPRSW','adoprsw')='password' "
        "or translate(@type,'CLOR','clor')='color' "
        "or translate(@type,'ADET','adet')='date' "
        "or translate(@type,'ADEIMT','adeimt')='datetime' "
        "or translate(@type,'ACDEILMOT','acdeilmot')='datetime-local' "
        "or translate(@type,'AEILM','aeilm')='email' "
        "or translate(@type,'HMNOT','hmnot')='month' "
        "or translate(@type,'BEMNRU','bemnru')='number' "
        "or translate(@type,'ACEHRS','acehrs')='search' "
        "or translate(@type,'ELT','elt')='tel' "
        "or translate(@type,'EIMT','eimt')='time' "
        "or translate(@type,'LRU','lru')='url' "
        "or translate(@type,'EKW','ekw')='week' "
        "]|//textarea",
        html, resolver, 5, NULL, NULL
    );
    if (!result) {
        return false;
    }
    while ((node = webkit_dom_xpath_result_iterate_next(result, NULL))) {
        if (element_is_visible(win, WEBKIT_DOM_ELEMENT(node))) {
            vb_enter('i');
            webkit_dom_element_focus(WEBKIT_DOM_ELEMENT(node));
            return true;
        }
    }

    /* Look for editable elements in frames too. */
    list = webkit_dom_document_get_elements_by_tag_name(doc, "iframe");
    len  = webkit_dom_node_list_get_length(list);

    for (i = 0; i < len; i++) {
        node      = webkit_dom_node_list_item(list, i);
        frame_doc = webkit_dom_html_iframe_element_get_content_document(WEBKIT_DOM_HTML_IFRAME_ELEMENT(node));
        /* Stop on first frame with focused element. */
        if (dom_focus_input(frame_doc)) {
            g_object_unref(list);
            return true;
        }
    }
    g_object_unref(list);

    return false;
}

/**
 * Indicates if the given dom element is an editable element like text input,
 * password or textarea.
 */
gboolean dom_is_editable(Element *element)
{
    gboolean result = false;
    char *tagname, *type, *editable;

    if (!element) {
        return result;
    }

    tagname  = webkit_dom_element_get_tag_name(element);
    type     = webkit_dom_element_get_attribute(element, "type");
    editable = webkit_dom_element_get_attribute(element, "contenteditable"); 
    /* element is editable if it's a text area or input with no type, text or
     * pasword */
    if (!g_ascii_strcasecmp(tagname, "textarea")) {
        result = true;
    } else if (!g_ascii_strcasecmp(tagname, "input")
        && (!*type
            || !g_ascii_strcasecmp(type, "text")
            || !g_ascii_strcasecmp(type, "password")
            || !g_ascii_strcasecmp(type, "color")
            || !g_ascii_strcasecmp(type, "date")
            || !g_ascii_strcasecmp(type, "datetime")
            || !g_ascii_strcasecmp(type, "datetime-local")
            || !g_ascii_strcasecmp(type, "email")
            || !g_ascii_strcasecmp(type, "month")
            || !g_ascii_strcasecmp(type, "number")
            || !g_ascii_strcasecmp(type, "search")
            || !g_ascii_strcasecmp(type, "tel")
            || !g_ascii_strcasecmp(type, "time")
            || !g_ascii_strcasecmp(type, "url")
            || !g_ascii_strcasecmp(type, "week"))
    ) {
        result = true;
    } else if (!g_ascii_strcasecmp(editable, "true")) {
        result = true;
    } else {
        result = false;
    }
    g_free(tagname);
    g_free(type);
    g_free(editable);

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
    /* Change only the mode if we are in normal mode - passthrough should not
     * be disturbed by this and if the user iy typing into inputbox we don't
     * want to remove the content and force a switch to input mode. And to
     * switch to input mode when this is already active makes no sense. */
    if (vb.mode->id == 'n' && dom_is_editable(element)) {
        vb_enter('i');

        return true;
    }
    return false;
}

static gboolean editable_blur_cb(Element *element, Event *event)
{
    if (vb.state.window_has_focus && vb.mode->id == 'i') {
        vb_enter('n');
    }

    return false;
}

static gboolean editable_focus_cb(Element *element, Event *event)
{
    if (vb.state.done_loading_page || !vb.config.strict_focus) {
        auto_insert((Element*)webkit_dom_event_get_target(event));
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
