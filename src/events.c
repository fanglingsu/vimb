/* Copyright (C) 2016-2019 Michael Mackus */
#include "events.h"

/* this is only to queue GDK key events, in order to later send them if the map didn't match */
static struct {
    GdkEventKey **queue;               /* queue holding submitted events */
    int           qlen;                /* pointer to last char in queue */
    bool          processing;          /* whether or not events are processing */
} events = {0};

/**
 * Append an event into the queue.
 */
void queue_event(GdkEventKey *e)
{
    GdkEventKey **newqueue = realloc(events.queue, (events.qlen + 1) * sizeof **newqueue);

    if (newqueue == NULL) {
        // error allocating memory
        return;
    }

    events.queue = newqueue;

    /* copy memory (otherwise event gets cleared by gdk) */
    GdkEventKey *tmp = malloc(sizeof *tmp);
    memcpy(tmp, e, sizeof *e);

    if (tmp == NULL) {
        // error allocating memory
        return;
    }

    events.queue[events.qlen] = tmp;
    events.qlen ++;
}

void process_event(GdkEventKey* event)
{
    if (event == NULL) {
        return;
    }

    events.processing = true; /* signal not to queue other events */
    gtk_main_do_event ((GdkEvent *) event);
    events.processing = false;
    free(event);
}

/**
 * Process events in the queue, sending the key events to GDK.
 */
void process_events()
{
    for (int i = 0; i < events.qlen; ++i) {
        process_event(events.queue[i]); /* process & free the event */
        /* TODO take into account qk mapped key? */
    }

    events.qlen = 0;
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
        free(events.queue[i]);
    }

    events.qlen = 0;
}
