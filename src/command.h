/**
 * vimp - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2013 Daniel Carl
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

enum {
    COMMAND_YANK_PRIMARY   = VP_CLIPBOARD_PRIMARY,
    COMMAND_YANK_SECONDARY = VP_CLIPBOARD_SECONDARY,
    COMMAND_YANK_URI       = (COMMAND_YANK_SECONDARY<<1),
    COMMAND_YANK_SELECTION = (COMMAND_YANK_SECONDARY<<2)
};

enum {
    COMMAND_ZOOM_OUT,
    COMMAND_ZOOM_IN,
    COMMAND_ZOOM_FULL  = (1<<1),
    COMMAND_ZOOM_RESET = (1<<2)
};

typedef gboolean (*Command)(const Arg* arg);

typedef struct {
    const char* name;
    Command      function;
    const Arg    arg;       /* arguments to call the command with */
} CommandInfo;


void command_init(void);
void command_cleanup(void);
gboolean command_exists(const char* name);
gboolean command_run(const char* name, const char* param);

gboolean command_open(const Arg* arg);
gboolean command_open_home(const Arg* arg);
gboolean command_open_closed(const Arg* arg);
gboolean command_input(const Arg* arg);
gboolean command_close(const Arg* arg);
gboolean command_view_source(const Arg* arg);
gboolean command_navigate(const Arg* arg);
gboolean command_scroll(const Arg* arg);
gboolean command_map(const Arg* arg);
gboolean command_unmap(const Arg* arg);
gboolean command_set(const Arg* arg);
gboolean command_complete(const Arg* arg);
gboolean command_inspect(const Arg* arg);
gboolean command_hints(const Arg* arg);
gboolean command_hints_focus(const Arg* arg);
gboolean command_yank(const Arg* arg);
gboolean command_paste(const Arg* arg);
gboolean command_search(const Arg* arg);
gboolean command_searchengine(const Arg* arg);
gboolean command_zoom(const Arg* arg);

#endif /* end of include guard: _COMMAND_H */
