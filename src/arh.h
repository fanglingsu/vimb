/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2014 Daniel Carl
 * Copyright (C) 2014 Sébastien Marie
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#include "config.h"
#ifdef FEATURE_ARH

#ifndef _ARH_H
#define _ARH_H

#include "main.h"

GSList *arh_parse(const char *, const char **);
void    arh_free(GSList *);
void    arh_run(GSList *, const char *, SoupMessage *);

#endif /* end of include guard: _ARH_H */
#endif
