/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2014 Daniel Carl
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

#ifdef FEATURE_COOKIE
#include "main.h"
#include "cookiejar.h"

G_DEFINE_TYPE(CookieJar, cookiejar, SOUP_TYPE_COOKIE_JAR_TEXT)

static void cookiejar_changed(SoupCookieJar *self, SoupCookie *old, SoupCookie *new);
static void cookiejar_class_init(CookieJarClass *klass);
static void cookiejar_finalize(GObject *self);
static void cookiejar_init(CookieJar *self);
static void cookiejar_set_property(GObject *self, guint prop_id,
    const GValue *value, GParamSpec *pspec);

extern VbCore vb;


SoupCookieJar *cookiejar_new(const char *file, gboolean ro)
{
    return g_object_new(
        COOKIEJAR_TYPE,
        SOUP_COOKIE_JAR_TEXT_FILENAME, file,
        SOUP_COOKIE_JAR_READ_ONLY, ro,
        NULL
    );
}

static void cookiejar_changed(SoupCookieJar *self, SoupCookie *old_cookie, SoupCookie *new_cookie)
{
    FLOCK(COOKIEJAR(self)->lock, F_WRLCK);
    SoupDate *expire;
    if (new_cookie) {
    /* session-expire-time handling */
    if (vb.config.cookie_expire_time == 0) {
        soup_cookie_set_expires(new_cookie, NULL);

    } else if (vb.config.cookie_expire_time > 0 && new_cookie->expires) {
        expire = soup_date_new_from_now(vb.config.cookie_expire_time);
        if (soup_date_to_time_t(expire) < soup_date_to_time_t(new_cookie->expires)) {
        soup_cookie_set_expires(new_cookie, expire);
        }
        soup_date_free(expire);
    }

    /* session-cookie handling */
    if (!new_cookie->expires && vb.config.cookie_timeout) {
        expire = soup_date_new_from_now(vb.config.cookie_timeout);
        soup_cookie_set_expires(new_cookie, expire);
        soup_date_free(expire);
    }
    }
    SOUP_COOKIE_JAR_CLASS(cookiejar_parent_class)->changed(self, old_cookie, new_cookie);
    FLOCK(COOKIEJAR(self)->lock, F_UNLCK);
}

static void cookiejar_class_init(CookieJarClass *klass)
{
    SOUP_COOKIE_JAR_CLASS(klass)->changed = cookiejar_changed;
    G_OBJECT_CLASS(klass)->get_property   = G_OBJECT_CLASS(cookiejar_parent_class)->get_property;
    G_OBJECT_CLASS(klass)->set_property   = cookiejar_set_property;
    G_OBJECT_CLASS(klass)->finalize       = cookiejar_finalize;
    g_object_class_override_property(G_OBJECT_CLASS(klass), 1, "filename");
}

static void cookiejar_finalize(GObject *self)
{
    close(COOKIEJAR(self)->lock);
    G_OBJECT_CLASS(cookiejar_parent_class)->finalize(self);
}

static void cookiejar_init(CookieJar *self)
{
    self->lock = open(vb.files[FILES_COOKIE], 0);
}

static void cookiejar_set_property(GObject *self, guint prop_id, const
    GValue *value, GParamSpec *pspec)
{
    FLOCK(COOKIEJAR(self)->lock, F_RDLCK);
    G_OBJECT_CLASS(cookiejar_parent_class)->set_property(self, prop_id, value, pspec);
    FLOCK(COOKIEJAR(self)->lock, F_UNLCK);
}
#endif
