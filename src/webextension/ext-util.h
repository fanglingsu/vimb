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

#ifndef _EXT_UTIL_H
#define _EXT_UTIL_H

#include <glib.h>
#include <JavaScriptCore/JavaScript.h>

gboolean ext_util_create_tmp_file(const char *content, char **file);
gboolean ext_util_js_eval(JSContextRef ctx, const char *script, JSValueRef *result);
char* ext_util_js_ref_to_string(JSContextRef ctx, JSValueRef ref);

#endif /* end of include guard: _EXT_UTIL_H */
