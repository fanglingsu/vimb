#include "hints.h"
#include "dom.h"

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
    WebKitDOMElement* elem;                 /* hinted element */
    gchar*            elemColor;            /* element color */
    gchar*            elemBackgroundColor;  /* element background color */
    WebKitDOMElement* hint;                 /* numbered hint element */
} Hint;

static void hints_create_for_window(const gchar* input, WebKitDOMDOMWindow* win, gulong offsetX, gulong offsetY);
static void hints_focus(const gulong num);
static Hint* hints_get_hint_by_number(const gulong num);
static GList* hints_get_hint_list_by_number(const gulong num);
static gchar* hints_get_xpath(const gchar* input);

static GList* hints = NULL;
static gulong currentFocusNum = 0;
static HintMode currentMode = HINTS_MODE_LINK;
static WebKitDOMElement* hintContainer = NULL;

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
        }

        g_list_free(hints);
        hints = NULL;
    }
    /* remove the hint container */
    if (hintContainer) {
        WebKitDOMNode* parent = webkit_dom_node_get_parent_node(WEBKIT_DOM_NODE(hintContainer));
        webkit_dom_node_remove_child(parent, WEBKIT_DOM_NODE(hintContainer), NULL);
        hintContainer = NULL;
    }
}

void hints_create(const gchar* input, HintMode mode)
{
    hints_clear();

    currentMode = mode;

    WebKitWebView* webview = WEBKIT_WEB_VIEW(vp.gui.webview);
    WebKitDOMDocument* doc = webkit_web_view_get_dom_document(webview);
    if (!doc) {
        return;
    }
    WebKitDOMDOMWindow* win = webkit_dom_document_get_default_view(doc);
    hints_create_for_window(input, win, 0, 0);

    hints_clear_focus();
    hints_focus(1);

    if (g_list_length(hints) == 1) {
        /* only one element hinted - we can fire it */
        hints_fire(1);
    }
}

void hints_update(const gulong num)
{

}

void hints_fire(const gulong num)
{
    PRINT_DEBUG("fire hint %li", num);
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

static void hints_create_for_window(const gchar* input, WebKitDOMDOMWindow* win, gulong offsetX, gulong offsetY)
{
    WebKitDOMDocument* doc = webkit_dom_dom_window_get_document(win);
    WebKitDOMNodeList* list = webkit_dom_document_get_elements_by_tag_name(doc, "body");
    if (!list) {
        return;
    }
    WebKitDOMNode* body = webkit_dom_node_list_item(list, 0);

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

    /* create the hint container element */
    hintContainer = webkit_dom_document_create_element(doc, "p", NULL);
    dom_element_set_style(hintContainer, HINT_CONTAINER_STYLE);

    webkit_dom_node_append_child(body, WEBKIT_DOM_NODE(hintContainer), NULL);
    webkit_dom_html_element_set_id(WEBKIT_DOM_HTML_ELEMENT(hintContainer), HINT_CONTAINER_ID);

    gulong snapshot_length = webkit_dom_xpath_result_get_snapshot_length(result, NULL);

    for (gulong i = 0; i < snapshot_length; i++) {
        WebKitDOMNode* node = webkit_dom_xpath_result_snapshot_item(result, i, NULL);
        WebKitDOMCSSStyleDeclaration* css_style = webkit_dom_element_get_style(WEBKIT_DOM_ELEMENT(node));
        gchar* visibility = webkit_dom_css_style_declaration_get_property_value(css_style, "visibility");
        if (visibility != NULL && g_ascii_strcasecmp(visibility, "hidden") == 0) {
            continue;
        }
        gchar* display = webkit_dom_css_style_declaration_get_property_value(css_style, "display");
        if (display != NULL && g_ascii_strcasecmp(display, "none") == 0) {
            continue;
        }

        /* create the hint element */
        WebKitDOMElement* hint = webkit_dom_document_create_element(doc, "span", NULL);

        Hint* newHint = g_new0(Hint, 1);
        newHint->num  = i + 1;
        newHint->elem = WEBKIT_DOM_ELEMENT(node);
        newHint->elemColor = webkit_dom_css_style_declaration_get_property_value(css_style, "color");
        newHint->elemBackgroundColor = webkit_dom_css_style_declaration_get_property_value(css_style, "background-color");
        newHint->hint = hint;
        hints = g_list_append(hints, newHint);

        /* change the style of the hinted element */
        dom_element_style_set_property(newHint->elem, "background-color", ELEM_BACKGROUND);
        dom_element_style_set_property(newHint->elem, "color", ELEM_COLOR);

        /*
        WebKitDOMDOMWindow* win = webkit_dom_document_get_default_view(doc);
        gulong win_height = webkit_dom_dom_window_get_inner_height(win);
        gulong win_width  = webkit_dom_dom_window_get_inner_width(win);
        */

        gchar* num = g_strdup_printf("%li", i + 1);
        webkit_dom_html_element_set_inner_text(WEBKIT_DOM_HTML_ELEMENT(hint), num, NULL);
        webkit_dom_html_element_set_class_name(WEBKIT_DOM_HTML_ELEMENT(hint), HINT_CLASS);
        g_free(num);

        /* calculate the hint position */
        glong left = webkit_dom_element_get_offset_left(WEBKIT_DOM_ELEMENT(node));
        glong top  = webkit_dom_element_get_offset_top(WEBKIT_DOM_ELEMENT(node));

        left -= 3;
        top  -= 3;
        gchar* hint_style = g_strdup_printf(HINT_STYLE, left, top);
        dom_element_set_style(hint, hint_style);
        g_free(hint_style);

        webkit_dom_node_append_child(WEBKIT_DOM_NODE(hintContainer), WEBKIT_DOM_NODE(hint), NULL);

        if (g_list_length(hints) >= MAX_HINTS) {
            break;
        }
    }
}

static void hints_focus(const gulong num)
{
    Hint* hint = hints_get_hint_by_number(currentFocusNum);
    if (hint) {
        /* reset previous focused element */
        dom_element_style_set_property(hint->elem, "background-color", ELEM_BACKGROUND);
    }

    hint = hints_get_hint_by_number(num);
    if (hint) {
        /* mark new hint as focused */
        dom_element_style_set_property(hint->elem, "background-color", ELEM_BACKGROUND_FOCUS);
    }

    currentFocusNum = num;
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
 * Retreives the xpatch epression according to current hinting mode and filter
 * input text.
 *
 * The returned string have to be freed.
 */
static gchar* hints_get_xpath(const gchar* input)
{
    gchar* xpath = NULL;

    switch (currentMode) {
        case HINTS_MODE_LINK:
            if (input == NULL) {
                xpath = g_strdup(
                    "//*[@onclick or @onmouseover or @onmousedown or @onmouseup or @oncommand or @class='lk' or @ role='link'] | "
                    "//a[@href]"
                );
            } else {
                xpath = g_strdup_printf(
                    "//*[(@onclick or @onmouseover or @onmousedown or @onmouseup or @oncommand or @class='lk' or @role='link') and contains(., '%s')] | "
                    "//a[@href and contains(., '%s')]",
                    input, input
                );
            }
            break;

        case HINTS_MODE_IMAGE:
            if (input == NULL) {
                xpath = g_strdup("//img[@src]");
            } else {
                xpath = g_strdup_printf("//img[@src and contains(., '%s')]", input);
            }
            break;
    }

    return xpath;
}
