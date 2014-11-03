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

#include "config.h"
#ifdef FEATURE_AUTOCMD

#ifndef _AUTOCMD_H
#define _AUTOCMD_H

#include "main.h"

/* this values correspond to indices in events[] array in autocmd.c */
typedef enum {
    AU_ALL,
    AU_LOAD_PROVISIONAL,
    AU_LOAD_COMMITED,
    AU_LOAD_FIRST_LAYOUT,
    AU_LOAD_FINISHED,
    AU_LOAD_FAILED,
    AU_DOWNLOAD_START,
    AU_DOWNLOAD_FINISHED,
    AU_DOWNLOAD_FAILED,
} AuEvent;

void autocmd_init(void);
void autocmd_cleanup(void);
gboolean autocmd_augroup(char *name, gboolean delete);
gboolean autocmd_add(char *name, gboolean delete);
gboolean autocmd_run(AuEvent event, const char *uri, const char *group);
gboolean autocmd_fill_group_completion(GtkListStore *store, const char *input);
gboolean autocmd_fill_event_completion(GtkListStore *store, const char *input);

#endif /* end of include guard: _AUTOCMD_H */
#endif
