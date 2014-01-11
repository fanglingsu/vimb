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

#ifndef _MODE_H
#define _MODE_H

#include "config.h"
#include "main.h"

void mode_init(void);
void mode_cleanup(void);
void mode_add(char id, ModeTransitionFunc enter, ModeTransitionFunc leave,
    ModeKeyFunc keypress, ModeInputChangedFunc input_changed);
void mode_enter(char id);
void mode_enter_prompt(char id, const char *prompt, gboolean print_prompt);
VbResult mode_handle_key(int key);
gboolean mode_input_focusin(GtkWidget *widget, GdkEventFocus *event, gpointer data);
gboolean mode_input_focusout(GtkWidget *widget, GdkEventFocus *event, gpointer data);
void mode_input_changed(GtkTextBuffer* buffer, gpointer data);

#endif /* end of include guard: _MODE_H */
