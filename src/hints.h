#ifndef HINTS_H
#define HINTS_H

#include "main.h"

typedef enum {
    HINTS_MODE_LINK,
    HINTS_MODE_IMAGE,
} HintMode;

void hints_create(const gchar* input, HintMode mode);
void hints_update(const gulong num);
void hints_fire(const gulong num);
void hints_clear(void);
void hints_clear_focus(void);
void hints_focus_next(const gboolean back);

#endif /* end of include guard: HINTS_H */
