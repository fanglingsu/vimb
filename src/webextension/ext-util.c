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
#include <string.h>
#include <unistd.h>
#include "../config.h"
#include "ext-util.h"

/**
 * Evaluates given string as script and return if this call succeed or not.
 * WebKitGTK 6.0: Updated to use JSC API
 */
gboolean ext_util_js_eval(JSCContext *ctx, const char *script, JSCValue **result)
{
    JSCValue *res = NULL;
    JSCException *exc = NULL;

    if (!ctx) {
        g_warning("ext_util_js_eval: JavaScript context is NULL");
        if (result) {
            *result = NULL;
        }
        return FALSE;
    }

    if (!script) {
        g_warning("ext_util_js_eval: Script is NULL");
        if (result) {
            *result = NULL;
        }
        return FALSE;
    }

    res = jsc_context_evaluate(ctx, script, -1);
    exc = jsc_context_get_exception(ctx);

    if (exc) {
        /* WebKitGTK 6.0: Convert exception to string value */
        char *exc_str = jsc_exception_to_string(exc);
        g_warning("JavaScript evaluation error: %s", exc_str);
        if (result) {
            *result = jsc_value_new_string(ctx, exc_str);
        }
        g_free(exc_str);
        jsc_context_clear_exception(ctx);
        return FALSE;
    }

    if (result) {
        *result = res ? g_object_ref(res) : NULL;
    }
    return TRUE;
}

/**
 * Creates a temporary file with given content.
 *
 * Upon success, and if file is non-NULL, the actual file path used is
 * returned in file. This string should be freed with g_free() when not
 * needed any longer.
 */
gboolean ext_util_create_tmp_file(const char *content, char **file)
{
    int fp;
    ssize_t bytes, len;

    fp = g_file_open_tmp(PROJECT "-XXXXXX", file, NULL);
    if (fp == -1) {
        g_critical("Could not create temp file %s", *file);
        g_free(*file);
        return FALSE;
    }

    len = strlen(content);

    /* write content into temporary file */
    bytes = write(fp, content, len);
    if (bytes < len) {
        close(fp);
        unlink(*file);
        g_critical("Could not write temp file %s", *file);
        g_free(*file);

        return FALSE;
    }
    close(fp);

    return TRUE;
}

/**
 * Returns a new allocates string for given value reference.
 * String must be freed if not used anymore.
 * WebKitGTK 6.0: Updated to use JSC API
 */
char* ext_util_js_ref_to_string(JSCContext *ctx, JSCValue *ref)
{
    char *string;

    g_return_val_if_fail(ref != NULL, NULL);

    string = jsc_value_to_string(ref);
    return string;
}
