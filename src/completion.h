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

#ifndef _COMPLETION_H
#define _COMPLETION_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include "main.h"

typedef void (*CompletionSelectFunc) (Client *c, char *match);

/* CompletionItem GObject for use with GListStore */
#define COMPLETION_TYPE_ITEM (completion_item_get_type())
G_DECLARE_FINAL_TYPE(CompletionItem, completion_item, COMPLETION, ITEM, GObject)

CompletionItem *completion_item_new(const char *first, const char *second);
const char *completion_item_get_first(CompletionItem *item);
const char *completion_item_get_second(CompletionItem *item);

void completion_clean(Client *c);
void completion_cleanup(Client *c);
gboolean completion_create(Client *c, GListStore *store,
        CompletionSelectFunc selfunc, gboolean back);
void completion_init(Client *c);
gboolean completion_next(Client *c, gboolean back);

/* Helper to create a new GListStore for completion items */
GListStore *completion_store_new(void);

#endif /* end of include guard: _COMPLETION_H */
