/* Copyright (C) 2016-2019 Michael Mackus */
#ifndef _EVENTS_H
#define _EVENTS_H

#include <gdk/gdkkeysyms.h>
#include <gdk/gdkkeysyms-compat.h>
#include "main.h"
#include "map.h"

void queue_event(GdkEventKey *e);
void process_events();
bool is_processing_events();
void free_events();

#endif /* end of include guard: _MAP_H */
