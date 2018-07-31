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

#ifndef _EXT_PROXY_H
#define _EXT_PROXY_H

#include "main.h"

const char *ext_proxy_init(void);
void ext_proxy_eval_script(Client *c, char *js, GAsyncReadyCallback callback);
GVariant *ext_proxy_eval_script_sync(Client *c, char *js);
void ext_proxy_focus_input(Client *c);
void ext_proxy_set_header(Client *c, const char *headers);
void ext_proxy_lock_input(Client *c, const char *element_id);
void ext_proxy_unlock_input(Client *c, const char *element_id);

#endif /* end of include guard: _EXT_PROXY_H */
