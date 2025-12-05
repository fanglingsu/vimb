/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2019 Daniel Carl
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

#ifndef _CONTEXT_MENU_H
#define _CONTEXT_MENU_H

#include "main.h"

/* WebKitGTK 6.0: context-menu signal no longer has GdkEvent parameter */
gboolean on_webview_context_menu(WebKitWebView *webview,
        WebKitContextMenu *context_menu,
        WebKitHitTestResult *hit_test_result, Client *c);

#endif /* end of include guard: _CONTEXT_MENU_H */
