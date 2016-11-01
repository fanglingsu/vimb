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

    GdkEventKey **newqueue = realloc(events.queue, (events.qlen + 1) * sizeof **newqueue);

    if (newqueue == NULL) {
        // error allocating memory
        return;
    }

    events.queue = newqueue;

    /* copy memory (otherwise event gets cleared by gdk) */
    GdkEventKey* tmp = malloc(sizeof *tmp);
    memcpy(tmp, e, sizeof *e);

    if (tmp == NULL) {
        // error allocating memory
        return;
    }

    events.queue[events.qlen] = tmp;
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
        GdkEventKey* event = events.queue[i];
        gtk_main_do_event ((GdkEvent*) event);
        gdk_event_free ((GdkEvent*) event);
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
    for (int i = 0; i < events.qlen; ++i)
    {
        GdkEventKey* event = events.queue[events.qlen];
        gdk_event_free ((GdkEvent*) event);
    }

    events.qlen = 0;
}
