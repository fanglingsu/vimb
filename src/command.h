#ifndef COMMAND_H
#define COMMAND_H

#include "main.h"
#include <webkit/webkit.h>

typedef void (*Command)(Arg* arg);

typedef struct {
    const gchar* name;
    Command      function;
    const Arg    arg;
} CommandInfo;


void command_init(void);
void command_run(const gchar* name);

void quit(Arg* arg);
void view_source(Arg* arg);

#endif /* end of include guard: COMMAND_H */
