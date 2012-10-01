#ifndef COMMAND_H
#define COMMAND_H

#include "main.h"
#include <webkit/webkit.h>

typedef void (*Command)(Arg* arg);

typedef struct {
    const gchar* name;
    Command      function;
} CommandInfo;


void command_init(void);
void command_parse_line(const gchar* line);
const CommandInfo* command_parse_parts(const gchar* line, Arg* arg);
void command_run_command(const CommandInfo* c, Arg* arg);

void quit(Arg* arg);
void view_source(Arg* arg);

#endif /* end of include guard: COMMAND_H */
