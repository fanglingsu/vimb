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

#ifndef _JS_H
#define _JS_H

#include "main.h"

gboolean js_eval_file(JSContextRef ctx, const char *file);
gboolean js_eval(JSContextRef ctx, const char *script, const char *file,
    char **value);
JSObjectRef js_create_object(JSContextRef ctx, const char *script);
char* js_object_call_function(JSContextRef ctx, JSObjectRef obj,
    const char *func, int count, JSValueRef params[]);
char *js_ref_to_string(JSContextRef ctx, JSValueRef ref);
JSValueRef js_string_to_ref(JSContextRef ctx, const char *string);
JSValueRef js_object_to_ref(JSContextRef ctx, const char *json);

#endif /* end of include guard: _JS_H */
