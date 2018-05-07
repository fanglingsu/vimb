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

#include "main.h"

typedef void (*CompletionSelectFunc) (Client *c, char *match);

enum {
    COMPLETION_STORE_FIRST,
#ifdef FEATURE_TITLE_IN_COMPLETION
    COMPLETION_STORE_SECOND,
#endif
    COMPLETION_STORE_NUM
};


void completion_clean(Client *c);
void completion_cleanup(Client *c);
gboolean completion_create(Client *c, GtkTreeModel *model,
        CompletionSelectFunc selfunc, gboolean back);
void completion_init(Client *c);
gboolean completion_next(Client *c, gboolean back);

#endif /* end of include guard: _COMPLETION_H */
