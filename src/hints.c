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

#include <gdk/gdkkeysyms.h>
#include <gdk/gdkkeysyms-compat.h>
#include "hints.h"
#include "dom.h"
#include "command.h"

#define MAX_HINTS 200
#define HINT_CONTAINER_ID "__hint_container"
#define HINT_CLASS "__hint"

#define ELEM_BACKGROUND "#ff0"
#define ELEM_BACKGROUND_FOCUS "#8f0"
#define ELEM_COLOR "#000"

#define HINT_CONTAINER_STYLE "line-height:1em;"
#define HINT_STYLE "z-index:100000;"\
    "position:absolute;"\
    "font-family:monospace;"\
    "font-size:10px;"\
    "font-weight:bold;"\
    "color:#000;"\
    "background-color:#fff;"\
    "margin:0;"\
    "padding:0px 1px;"\
    "border:1px solid #444;"\
    "opacity:0.7;"\
    "left:%lipx;top:%lipx;"

typedef struct {
    gulong num;
    Element* elem;                 /* hinted element */
    gchar*   elemColor;            /* element color */
    gchar*   elemBackgroundColor;  /* element background color */
    Element* hint;                 /* numbered hint element */
} Hint;

static void hints_create_for_window(
    const gchar* input, Window* win, gulong top_width,
    gulong top_height, gulong offsetX, gulong offsetY);
static void hints_focus(const gulong num);
static void hints_fire(const gulong num);
static void hints_click_fired_hint(guint mode, Element* elem);
static void hints_process_fired_hint(guint mode, const gchar* uri);
static Hint* hints_get_hint_by_number(const gulong num);
static GList* hints_get_hint_list_by_number(const gulong num);
static gchar* hints_get_xpath(const gchar* input);
static void hints_observe_input(gboolean observe);
static gboolean hints_changed_callback(GtkEditable *entry, gpointer data);
static gboolean hints_keypress_callback(WebKitWebView* webview, GdkEventKey* event);
static gboolean hints_num_has_prefix(gulong num, gulong prefix);

/* variables */
static GList* hints = NULL;
static gulong currentFocusNum = 0;
static gulong hintCount = 0;
static gulong hintNum = 0;
static guint currentMode = HINTS_TYPE_LINK;
static Element* hintContainer = NULL;

void hints_clear(void)
{
    /* free the list of hints */
    if (hints) {
        GList* link;
        for (link = hints; link != NULL; link = link->next) {
            Hint* hint = (Hint*)link->data;

            /* reset the previous color of the hinted elements */
            dom_element_style_set_property(hint->elem, "color", hint->elemColor);
            dom_element_style_set_property(hint->elem, "background-color", hint->elemBackgroundColor);
            g_free(hint);
        }

        g_list_free(hints);
        hints = NULL;
        hintCount = 0;
        hintNum = 0;
    }
    /* remove the hint container */
    if (hintContainer) {
        Node* parent = webkit_dom_node_get_parent_node(WEBKIT_DOM_NODE(hintContainer));
        webkit_dom_node_remove_child(parent, WEBKIT_DOM_NODE(hintContainer), NULL);
        hintContainer = NULL;
    }

    hints_observe_input(FALSE);
}

void hints_create(const gchar* input, guint mode)
{
    Document* doc;
    Window* win;
    gulong top_width, top_height, offsetX, offsetY;

    currentMode = mode;
    hints_clear();

    doc = webkit_web_view_get_dom_document(WEBKIT_WEB_VIEW(vp.gui.webview));
    if (!doc) {
        return;
    }

    /* add event hanlder for inputbox */
    hints_observe_input(TRUE);

    win        = webkit_dom_document_get_default_view(doc);
    top_width  = webkit_dom_dom_window_get_inner_width(win);
    top_height = webkit_dom_dom_window_get_inner_height(win);
    offsetY    = webkit_dom_dom_window_get_scroll_y(win);
    offsetX    = webkit_dom_dom_window_get_scroll_x(win);

    /* create the hints for the main window */
    hints_create_for_window(input, win, top_width, top_height, offsetX, offsetY);

    hints_clear_focus();
    hints_focus(1);

    if (hintCount == 1) {
        /* only one element hinted - we can fire it */
        hints_fire(1);
    }
}

void hints_update(const gulong num)
{
    Hint* hint = NULL;
    GList* next = NULL;
    GList* link = hints;

    if (num == 0) {
        /* recreate the hints */
        hints_create(NULL, currentMode);
        return;
    }

    while (link != NULL) {
        hint = (Hint*)link->data;
        if (!hints_num_has_prefix(hint->num, num)) {
            /* reset the previous color of the hinted elements */
            dom_element_style_set_property(hint->elem, "color", hint->elemColor);
            dom_element_style_set_property(hint->elem, "background-color", hint->elemBackgroundColor);
            webkit_dom_node_remove_child(WEBKIT_DOM_NODE(hintContainer), WEBKIT_DOM_NODE(hint->hint), NULL);
            g_free(hint);

            /* store next element before remove current */
            next = g_list_next(link);
            hints = g_list_remove_link(hints, link);
            link = next;
        } else {
            link = g_list_next(link);
        }
    }
    if (g_list_length(hints) == 1) {
        hints_fire(num);
    } else {
        hints_focus(num);
    }
}

void hints_clear_focus(void)
{

}

void hints_focus_next(const gboolean back)
{
    Hint* hint  = NULL;
    GList* list = hints_get_hint_list_by_number(currentFocusNum);

    list = back ? g_list_previous(list) : g_list_next(list);
    if (list != NULL) {
        hint = (Hint*)list->data;
    } else {
        /* if we reached begin or end start on the opposite side */
        list = back ? g_list_last(hints) : g_list_first(hints);
        hint = (Hint*)list->data;
    }
    hints_focus(hint->num);
}

static void hints_create_for_window(
    const gchar* input,
    Window* win,
    gulong top_width,
    gulong top_height,
    gulong offsetX,
    gulong offsetY)
{
    Document* doc = webkit_dom_dom_window_get_document(win);
    NodeList* list = webkit_dom_document_get_elements_by_tag_name(doc, "body");
    if (!list) {
        return;
    }
    Node* body = webkit_dom_node_list_item(list, 0);

    WebKitDOMXPathNSResolver* ns_resolver = webkit_dom_document_create_ns_resolver(doc, body);
    if (!ns_resolver) {
        return;
    }

    gchar* xpath = hints_get_xpath(input);
    WebKitDOMXPathResult* result = webkit_dom_document_evaluate(
        doc, xpath, WEBKIT_DOM_NODE(doc), ns_resolver, 7, NULL, NULL
    );
    g_free(xpath);

    if (!result) {
        return;
    }

    /* get the bounds */
    gulong win_height = webkit_dom_dom_window_get_inner_height(win);
    gulong win_width  = webkit_dom_dom_window_get_inner_width(win);
    gulong minX = offsetX < 0 ? -offsetX : offsetX;
    gulong minY = offsetY < 0 ? -offsetY : offsetY;
    gulong maxX = offsetX + win_width;
    gulong maxY = offsetY + win_height;

    /* create the hint container element */
    hintContainer = webkit_dom_document_create_element(doc, "p", NULL);
    dom_element_set_style(hintContainer, HINT_CONTAINER_STYLE);

    webkit_dom_node_append_child(body, WEBKIT_DOM_NODE(hintContainer), NULL);
    webkit_dom_html_element_set_id(WEBKIT_DOM_HTML_ELEMENT(hintContainer), HINT_CONTAINER_ID);

    gulong snapshot_length = webkit_dom_xpath_result_get_snapshot_length(result, NULL);

    for (gulong i = 0; i < snapshot_length && hintCount < MAX_HINTS; i++) {
        /* TODO this cast causes a bug if hinting is started after a link was
         * opened into new window via middle mouse click or right click
         * context menu */
        Element* elem = WEBKIT_DOM_ELEMENT(webkit_dom_xpath_result_snapshot_item(result, i, NULL));
        if (!dom_element_is_visible(win, elem)) {
            continue;
        }
        DomBoundingRect rect = dom_elemen_get_bounding_rect(elem);
        if (rect.left > maxX || rect.right < minX || rect.top > maxY || rect.bottom < minY) {
            continue;
        }

        hintCount++;

        /* create the hint element */
        Element* hint = webkit_dom_document_create_element(doc, "span", NULL);
        CssDeclaration* css_style = webkit_dom_element_get_style(elem);

        Hint* newHint = g_new0(Hint, 1);
        newHint->num                 = hintCount;
        newHint->elem                = elem;
        newHint->elemColor           = webkit_dom_css_style_declaration_get_property_value(css_style, "color");
        newHint->elemBackgroundColor = webkit_dom_css_style_declaration_get_property_value(css_style, "background-color");
        newHint->hint                = hint;
        hints = g_list_append(hints, newHint);

        gulong left = rect.left - 3;
        gulong top  = rect.top - 3;
        dom_element_set_style(hint, HINT_STYLE, left, top);

        gchar* num = g_strdup_printf("%li", newHint->num);
        webkit_dom_html_element_set_inner_text(WEBKIT_DOM_HTML_ELEMENT(hint), num, NULL);
        webkit_dom_html_element_set_class_name(WEBKIT_DOM_HTML_ELEMENT(hint), HINT_CLASS);
        g_free(num);

        /* change the style of the hinted element */
        dom_element_style_set_property(newHint->elem, "background-color", ELEM_BACKGROUND);
        dom_element_style_set_property(newHint->elem, "color", ELEM_COLOR);

        webkit_dom_node_append_child(WEBKIT_DOM_NODE(hintContainer), WEBKIT_DOM_NODE(hint), NULL);
    }

    /* TODO call this function for every found frame or iframe too */
}

static void hints_focus(const gulong num)
{
    /* TODO use the document the hinted element belongs to */
    Document* doc = webkit_web_view_get_dom_document(vp.gui.webview);
    Hint* hint = hints_get_hint_by_number(currentFocusNum);
    if (hint) {
        /* reset previous focused element */
        dom_element_style_set_property(hint->elem, "background-color", ELEM_BACKGROUND);

        dom_dispatch_mouse_event(doc, hint->elem, "mouseout", 0);
    }

    hint = hints_get_hint_by_number(num);
    if (hint) {
        /* mark new hint as focused */
        dom_element_style_set_property(hint->elem, "background-color", ELEM_BACKGROUND_FOCUS);

        dom_dispatch_mouse_event(doc, hint->elem, "mouseover", 0);
        webkit_dom_element_blur(hint->elem);
    }

    currentFocusNum = num;
}

static void hints_fire(const gulong num)
{
    Hint* hint = hints_get_hint_by_number(num);
    if (!hint) {
        return;
    }
    if (dom_is_editable(hint->elem)) {
        webkit_dom_element_focus(hint->elem);
        vp_set_mode(VP_MODE_INSERT, FALSE);
    } else {
        if (currentMode & HINTS_PROCESS) {
            hints_process_fired_hint(currentMode, dom_element_get_source(hint->elem));
        } else {
            hints_click_fired_hint(currentMode, hint->elem);

            /* remove the hint filter input and witch to normal mode */
            vp_set_mode(VP_MODE_NORMAL, TRUE);
        }
    }
    hints_clear();
}

/**
 * Perform a mouse click to given element.
 */
static void hints_click_fired_hint(guint mode, Element* elem)
{
    /* TODO I don't get the mouse click event dispatched here to
     * invoke the gtk events for button press wich might be the better
     * way hanlding to open new windows than setting temporary target
     * attribute here */
    gchar* target = webkit_dom_element_get_attribute(elem, "target");
    if (mode & HINTS_TARGET_BLANK) {                /* open in new window */
        webkit_dom_element_set_attribute(elem, "target", "_blank", NULL);
    } else if (g_strcmp0(target, "_blank") == 0) {  /* remove possible target attribute */
        webkit_dom_element_remove_attribute(elem, "target");
    }

    /* dispatch click event */
    /* TODO use document of the hint an not the default one, but this is only
     * required if we hint also into frames and iframes */
    Document* doc = webkit_web_view_get_dom_document(vp.gui.webview);
    dom_dispatch_mouse_event(doc, elem, "click", 0);

    /* reset previous target attribute */
    if (target && strlen(target)) {
        webkit_dom_element_set_attribute(elem, "target", target, NULL);
    } else {
        webkit_dom_element_remove_attribute(elem, "target");
    }
}

/**
 * Handle fired hints that are not opened via simulated mouse click.
 */
static void hints_process_fired_hint(guint mode, const gchar* uri)
{
    HintsProcess type = HINTS_GET_PROCESSING(mode);
    Arg a = {0};
    switch (type) {
        case HINTS_PROCESS_INPUT:
            a.s = g_strconcat((mode & HINTS_TARGET_BLANK) ? ":tabopen " : ":open ", uri, NULL);
            command_input(&a);
            g_free(a.s);
            break;

        case HINTS_PROCESS_YANK:
            /* TODO not implemented */
            break;
    }
}

static Hint* hints_get_hint_by_number(const gulong num)
{
    GList* list = hints_get_hint_list_by_number(num);
    if (list) {
        return (Hint*)list->data;
    }

    return NULL;
}

static GList* hints_get_hint_list_by_number(const gulong num)
{
    GList* link;
    for (link = hints; link != NULL; link = link->next) {
        Hint* hint = (Hint*)link->data;
        /* TODO check if it would be faster to use the sorting of the numbers
         * in the list to get the items */
        if (hint->num == num) {
            return link;
        }
    }

    return NULL;
}

/**
 * Retreives the xpath epression according to current hinting mode and filter
 * input text.
 *
 * The returned string have to be freed.
 */
static gchar* hints_get_xpath(const gchar* input)
{
    gchar* xpath = NULL;

    switch (HINTS_GET_TYPE(currentMode)) {
        case HINTS_TYPE_LINK:
            if (input == NULL) {
                xpath = g_strdup(
                    "//*[@onclick or @onmouseover or @onmousedown or @onmouseup or @oncommand or @class='lk' or @ role='link'] | "
                    "//a[@href] | "
                    "//input[not(@type='hidden')] | "
                    "//textarea | "
                    "//button | "
                    "//select | "
                    "//area"
                );
            } else {
                xpath = g_strdup_printf(
                    "//*[(@onclick or @onmouseover or @onmousedown or @onmouseup or @oncommand or @class='lk' or @role='link') and contains(., '%s')] | "
                    "//a[@href and contains(., '%s')] | "
                    "//input[not(@type='hidden') and contains(., '%s')] | "
                    "//textarea[contains(., '%s')] | "
                    "//button[contains(@value, '%s')] | "
                    "//select[contains(., '%s')] | "
                    "//area[contains(., '%s')]",
                    input, input, input, input, input, input, input
                );
            }
            break;

        case HINTS_TYPE_IMAGE:
            if (input == NULL) {
                xpath = g_strdup("//img[@src]");
            } else {
                xpath = g_strdup_printf("//img[@src and contains(., '%s')]", input);
            }
            break;
    }

    return xpath;
}

static void hints_observe_input(gboolean observe)
{
    static gulong changeHandler   = 0;
    static gulong keypressHandler = 0;

    if (observe) {
        changeHandler = g_signal_connect(
            G_OBJECT(vp.gui.inputbox), "changed", G_CALLBACK(hints_changed_callback), NULL
        );
        keypressHandler = g_signal_connect(
            G_OBJECT(vp.gui.inputbox), "key-press-event", G_CALLBACK(hints_keypress_callback), NULL
        );
    } else if (changeHandler && keypressHandler) {
        g_signal_handler_disconnect(G_OBJECT(vp.gui.inputbox), changeHandler);
        g_signal_handler_disconnect(G_OBJECT(vp.gui.inputbox), keypressHandler);
        changeHandler = 0;
        keypressHandler = 0;
    }
}

static gboolean hints_changed_callback(GtkEditable *entry, gpointer data)
{
    const gchar* text = GET_TEXT();

    /* skip hinting prefixes like '. ', ', ', ';y' ... */
    hints_create(text + 2, currentMode);

    return TRUE;
}

static gboolean hints_keypress_callback(WebKitWebView* webview, GdkEventKey* event)
{
    gint numval;
    guint keyval = event->keyval;
    guint state  = CLEAN_STATE_WITH_SHIFT(event);

    if (keyval == GDK_Return) {
        hints_fire(currentFocusNum);
        return TRUE;
    }
    if (keyval == GDK_BackSpace && (state & GDK_SHIFT_MASK) && (state & GDK_CONTROL_MASK)) {
        hintNum /= 10;
        hints_update(hintNum);

        return TRUE;
    }
    numval = g_unichar_digit_value((gunichar)gdk_keyval_to_unicode(keyval));
    if ((numval >= 1 && numval <= 9) || (numval == 0 && hintNum)) {
        /* allow a zero as non-first number */
        hintNum = (hintNum ? hintNum * 10 : 0) + numval;
        hints_update(hintNum);

        return TRUE;
    }

    return FALSE;
}

static gboolean hints_num_has_prefix(gulong num, gulong prefix)
{
    if (prefix == num) {
        return TRUE;
    }
    if (num >= 10) {
        return hints_num_has_prefix(num / 10, prefix);
    }

    return FALSE;
}
