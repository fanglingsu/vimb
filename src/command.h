#ifndef COMMAND_H
#define COMMAND_H

#include "main.h"
#include <webkit/webkit.h>

typedef void (*Command)(GArray *argv, GString *result);

typedef struct {
    const gchar* name;
    Command      function;
} CommandInfo;


void command_init(void);
void command_parse_line(const gchar* line, GString* result);
const CommandInfo* command_parse_parts(const gchar* line, GArray* a);
void command_run_command(const CommandInfo* c, GArray* a, GString* result);

void quit(GArray* argv, GString* result);
void view_source(GArray* argv, GString* result);

#endif /* end of include guard: COMMAND_H */
