#ifndef COMMAND_H
#define COMMAND_H

#include "main.h"
#include <webkit/webkit.h>

typedef void (*Command)(const Arg* arg);

typedef struct {
    const gchar* name;
    Command      function;
    const Arg    arg;
} CommandInfo;


void command_init(void);
gboolean command_run(const gchar* name, const gchar* param);

#endif /* end of include guard: COMMAND_H */
