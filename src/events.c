#include "events.h"

/* this is only to queue GDK key events, in order to later send them if the map didn't match */
static struct {
    GdkEventKey **queue;                /* queue holding submitted events */
    int            qlen;                /* pointer to last char in queue */
    bool           processing;          /* whether or not events are processing */
} events;

/**
 * Append an event into the queue.
 */
void queue_event(GdkEventKey* e)
{
    if (vb.mode->id != 'i') {
        /* events are only needed for input mode */
        return;
    }

    if (events.queue == NULL) {
        events.queue = malloc(0);
    }

    events.queue = realloc(events.queue, (events.qlen + 1) * sizeof *events.queue);
    events.queue[events.qlen] = e;
    events.qlen ++;
}

/**
 * Process events in the queue, sending the key events to GDK.
 */
void process_events()
{
    if (vb.mode->id != 'i') {
        /* events are only needed for input mode */
        return;
    }

    events.processing = true; /* signal not to map our events */

    for (int i = 0; i < events.qlen; ++i)
    {
        // TODO send to gdk
    }

    events.qlen = 0;
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
void clear_events()
{
    events.qlen = 0;
}
