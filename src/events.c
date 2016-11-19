#include "events.h"

/* this is only to queue GDK key events, in order to later send them if the map didn't match */
static struct {
    GdkEventKey **queue;        /* queue holding submitted events */
    int           qlen;         /* pointer to last char in queue */
    bool          processing;   /* whether or not events are processing */
} events;

extern VbCore vb;

static void process_event(GdkEventKey *event);

/**
 * Append an event into the queue.
 */
void queue_event(GdkEventKey *e)
{
    events.queue = g_realloc(events.queue, (events.qlen + 1) * sizeof(GdkEventKey*));

    /* copy memory (otherwise event gets cleared by gdk) */
    events.queue[events.qlen] = g_memdup(e, sizeof(GdkEventKey));
    events.qlen ++;
}

/**
 * Process events in the queue, sending the key events to GDK.
 */
void process_events()
{
    for (int i = 0; i < events.qlen; ++i) {
        process_event(events.queue[i]);
        g_free(events.queue[i]);
        /* TODO take into account qk mapped key? */
    }

    events.qlen = 0;
}

static void process_event(GdkEventKey *event)
{
    if (!event) {
        return;
    }

    events.processing = true; /* signal not to queue other events */
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
    for (int i = 0; i < events.qlen; ++i) {
        g_free(events.queue[i]);
    }

    events.qlen = 0;
}
