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

#include <glib.h>
#include <webkitdom/webkitdom.h>

#include "ext-main.h"
#include "ext-dom.h"

static gboolean is_element_visible(WebKitDOMHTMLElement *element);


/**
 * Checks if given dom element is an editable element.
 */
gboolean ext_dom_is_editable(WebKitDOMElement *element)
{
    char *type;
    gboolean result = FALSE;

    if (!element) {
        return FALSE;
    }

    /* element is editable if it's a text area or input with no type, text or
     * password */
    if (webkit_dom_html_element_get_is_content_editable(WEBKIT_DOM_HTML_ELEMENT(element))
            || WEBKIT_DOM_IS_HTML_TEXT_AREA_ELEMENT(element)) {
        return TRUE;
    }

    if (WEBKIT_DOM_IS_HTML_INPUT_ELEMENT(element)) {
        type = webkit_dom_html_input_element_get_input_type(WEBKIT_DOM_HTML_INPUT_ELEMENT(element));
        /* Input element without type attribute are rendered and behave like
         * type = text and there are a lot of pages in the wild using input
         * field without type attribute. */
        if (!*type
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
        {
            result = TRUE;
        }

        g_free(type);
    }

    return result;
}

/**
 * Find the first editable element and set the focus on it and enter input
 * mode.
 * Returns true if there was an editable element focused.
 */
gboolean ext_dom_focus_input(WebKitDOMDocument *doc)
{
    WebKitDOMNode *html, *node;
    WebKitDOMHTMLCollection *collection;
    WebKitDOMXPathNSResolver *resolver;
    WebKitDOMXPathResult* result;
    WebKitDOMDocument *frame_doc;
    guint i, len;

    collection = webkit_dom_document_get_elements_by_tag_name_as_html_collection(doc, "html");
    if (!collection) {
        return FALSE;
    }

    html = webkit_dom_html_collection_item(collection, 0);
    g_object_unref(collection);

    resolver = webkit_dom_document_create_ns_resolver(doc, html);
    if (!resolver) {
        return FALSE;
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
        return FALSE;
    }
    while ((node = webkit_dom_xpath_result_iterate_next(result, NULL))) {
        if (is_element_visible(WEBKIT_DOM_HTML_ELEMENT(node))) {
            webkit_dom_element_focus(WEBKIT_DOM_ELEMENT(node));
            return TRUE;
        }
    }

    /* Look for editable elements in frames too. */
    collection = webkit_dom_document_get_elements_by_tag_name_as_html_collection(doc, "iframe");
    len        = webkit_dom_html_collection_get_length(collection);

    for (i = 0; i < len; i++) {
        node      = webkit_dom_html_collection_item(collection, i);
        frame_doc = webkit_dom_html_iframe_element_get_content_document(WEBKIT_DOM_HTML_IFRAME_ELEMENT(node));
        /* Stop on first frame with focused element. */
        if (ext_dom_focus_input(frame_doc)) {
            g_object_unref(collection);
            return TRUE;
        }
    }
    g_object_unref(collection);

    return FALSE;
}

/**
 * Retrieves the content of given editable element.
 * Not that the returned value must be freed.
 */
char *ext_dom_editable_get_value(WebKitDOMElement *element)
{
    char *value = NULL;

    if ((webkit_dom_html_element_get_is_content_editable(WEBKIT_DOM_HTML_ELEMENT(element)))) {
        value = webkit_dom_html_element_get_inner_text(WEBKIT_DOM_HTML_ELEMENT(element));
    } else if (WEBKIT_DOM_IS_HTML_INPUT_ELEMENT(WEBKIT_DOM_HTML_INPUT_ELEMENT(element))) {
        value = webkit_dom_html_input_element_get_value(WEBKIT_DOM_HTML_INPUT_ELEMENT(element));
    } else {
        value = webkit_dom_html_text_area_element_get_value(WEBKIT_DOM_HTML_TEXT_AREA_ELEMENT(element));
    }

    return value;
}

void ext_dom_lock_input(WebKitDOMDocument *parent, char *element_id)
{
    WebKitDOMElement *elem;

    elem = webkit_dom_document_get_element_by_id(parent, element_id);
    if (elem != NULL) {
        webkit_dom_element_set_attribute(elem, "disabled", "true", NULL);
    }
}

void ext_dom_unlock_input(WebKitDOMDocument *parent, char *element_id)
{
    WebKitDOMElement *elem;

    elem = webkit_dom_document_get_element_by_id(parent, element_id);
    if (elem != NULL) {
        webkit_dom_element_remove_attribute(elem, "disabled");
        webkit_dom_element_focus(elem);
    }
}

/**
 * Indicates if the give nelement is visible.
 */
static gboolean is_element_visible(WebKitDOMHTMLElement *element)
{
    return TRUE;
}

