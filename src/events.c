#include "events.h"

/* this is only to queue GDK key events, in order to later send them if the map didn't match */
static struct {
    GSList *list;       /* queue holding submitted events */
    bool    processing; /* whether or not events are processing */
} events;

extern VbCore vb;

static void process_event(GdkEventKey *event);

/**
 * Append an event into the queue.
 */
void queue_event(GdkEventKey *e)
{
    GdkEventKey *copy = g_memdup(e, sizeof(GdkEventKey));
    /* copy memory (otherwise event gets cleared by gdk) */
    events.list = g_slist_append(events.list, copy);
}

/**
 * Process events in the queue, sending the key events to GDK.
 */
void process_events()
{
    for (GSList *l = events.list; l != NULL; l = l->next) {
        process_event((GdkEventKey*)l->data);
        g_free(l->data);
        events.list = g_slist_delete_link(events.list, l);
        /* TODO take into account qk mapped key? */
    }
}

static void process_event(GdkEventKey *event)
{
    if (!event) {
        return;
    }

    /* signal not to queue other events */
    events.processing = true;
    gtk_main_do_event((GdkEvent*)event);
    events.processing = false;
}

/**
 * Check if the events are currently processing (i.e. being sent to GDK
 * unhandled).  Provided in order to encapsulate the "events" global struct.
 */
bool is_processing_events()
{
    return events.processing;
}

/**
 * Clear the event queue by resetting the length. Provided in order to
 * encapsulate the "events" global struct.
 */
void free_events()
{
    if (events.list) {
        g_slist_free_full(events.list, (GDestroyNotify)g_free);
        events.list = NULL;
    }
}
