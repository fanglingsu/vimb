#ifndef _EVENTS_H
#define _EVENTS_H

#include <gdk/gdkkeysyms.h>
#include <gdk/gdkkeysyms-compat.h>
#include "main.h"
#include "map.h"

void queue_event(GdkEventKey* e);
void clear_events();
void process_events();
bool is_processing_events();

extern VbCore vb;

#endif /* end of include guard: _MAP_H */
