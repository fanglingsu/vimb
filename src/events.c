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
        return free_events();
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
 * Free latest event & decrement qlen.
 */
void pop_event()
{
    if (events.qlen == 0) {
        return;
    }

    free(events.queue[events.qlen - 1]);
    events.qlen --;
}

/**
 * Process events in the queue, sending the key events to GDK.
 */
void process_events(bool is_timeout)
{
    if (vb.mode->id != 'i') {
        /* events are only needed for input mode */
        return free_events();
    }

    if (!is_timeout || events.qlen > 1) {
        /* pop last event to prevent duplicate input */
        pop_event();
    }

    events.processing = true; /* signal not to map our events */

    for (int i = 0; i < events.qlen; ++i)
    {
        GdkEventKey* event = events.queue[i];
        gtk_main_do_event ((GdkEvent*) event);
    }

    free_events();
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
    for (int i = 0; i < events.qlen; ++i)
    {
        free(events.queue[i]);
    }

    events.qlen = 0;
}
