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

#include <JavaScriptCore/JavaScript.h>
#include <glib.h>
#include <webkit2/webkit2.h>

#include "js.h"
#include "config.h"

/**
 * Returns a new allocates string for given value javascript result.
 * String must be freed if not used anymore.
 */
char *js_result_as_string(WebKitJavascriptResult *res)
{
    JSGlobalContextRef cr;
    JSStringRef jsstring;
    JSValueRef jsvalue;
    gsize max;

    g_return_val_if_fail(res != NULL, NULL);

    jsvalue  = webkit_javascript_result_get_value(res);
    cr       = webkit_javascript_result_get_global_context(res);
    jsstring = JSValueToStringCopy(cr, jsvalue, NULL);
    max      = JSStringGetMaximumUTF8CStringSize(jsstring);
    if (max > 0) {
        char *string = g_new(char, max);
        JSStringGetUTF8CString(jsstring, string, max);

        return string;
    }
    return NULL;
}

