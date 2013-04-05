/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2013 Daniel Carl
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

#ifndef _SESSION_H
#define _SESSION_H

#include <glib.h>
#include <libsoup/soup.h>

#define SESSION_COOKIEJAR_TYPE (session_cookiejar_get_type())
#define SESSION_COOKIEJAR(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), SESSION_COOKIEJAR_TYPE, SessionCookieJar))

typedef struct {
    SoupCookieJarText parent_instance;
    int lock;
} SessionCookieJar;

typedef struct {
    SoupCookieJarTextClass parent_class;
} SessionCookieJarClass;

GType session_cookiejar_get_type(void);

void session_init(void);

#endif /* end of include guard: _SESSION_H */
