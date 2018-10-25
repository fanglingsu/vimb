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

#ifndef _COMMAND_H
#define _COMMAND_H

#include <gtk/gtk.h>
#include "main.h"

enum {
    COMMAND_YANK_ARG,
    COMMAND_YANK_URI,
    COMMAND_YANK_SELECTION
};

enum {
    COMMAND_SAVE_CURRENT,
    COMMAND_SAVE_URI
};

#ifdef FEATURE_QUEUE
enum {
    COMMAND_QUEUE_PUSH,
    COMMAND_QUEUE_UNSHIFT,
    COMMAND_QUEUE_POP,
    COMMAND_QUEUE_CLEAR
};
#endif

typedef void (*PostEditFunc)(const char *, Client *, gpointer);

gboolean command_search(Client *c, const Arg *arg, bool commit);
gboolean command_yank(Client *c, const Arg *arg, char buf);
gboolean command_save(Client *c, const Arg *arg);
#ifdef FEATURE_QUEUE
gboolean command_queue(Client *c, const Arg *arg);
#endif
gboolean command_spawn_editor(Client *c, const Arg *arg, PostEditFunc posteditfunc, gpointer data);

#endif /* end of include guard: _COMMAND_H */
