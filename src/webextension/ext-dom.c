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
