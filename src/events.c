/* Copyright (C) 2016-2019 Michael Mackus */
#include <gtk/gtk.h>

#include "events.h"

/* this is only to queue GDK key events, in order to later send them if the map didn't match */
static struct {
    GSList *list;       /* queue holding submitted events */
    bool    processing; /* whether or not events are processing */
} events = {0};

static void process_event(GdkEventKey* event);

/**
 * Append an event into the queue.
 */
void queue_event(GdkEventKey *e)
{
    events.list = g_slist_append(events.list, gdk_event_copy((GdkEvent*)e));
}

/**
 * Process events in the queue, sending the key events to GDK.
 */
void process_events(void)
{
    for (GSList *l = events.list; l != NULL; l = l->next) {
        process_event((GdkEventKey*)l->data);
        /* TODO take into account qk mapped key? */
    }
    free_events();
}

/**
 * Check if the events are currently processing (i.e. being sent to GDK
 * unhandled).
 */
gboolean is_processing_events(void)
{
    return events.processing;
}

/**
 * Clear the events list and free the allocated memory.
 */
void free_events(void)
{
    if (events.list) {
        g_slist_free_full(events.list, (GDestroyNotify)gdk_event_free);
        events.list = NULL;
    }
}

static void process_event(GdkEventKey* event)
{
    if (event) {
        /* signal not to queue other events */
        events.processing = TRUE;
        gtk_main_do_event((GdkEvent*)event);
        events.processing = FALSE;
    }
}
