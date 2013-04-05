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

#include <sys/file.h>
#include "main.h"
#include "session.h"

G_DEFINE_TYPE(SessionCookieJar, session_cookiejar, SOUP_TYPE_COOKIE_JAR_TEXT)

static SoupCookieJar *session_cookiejar_new(const char *file, gboolean ro);
static void session_cookiejar_changed(SoupCookieJar *self, SoupCookie *old, SoupCookie *new);
static void session_cookiejar_class_init(SessionCookieJarClass *class);
static void session_cookiejar_finalize(GObject *self);
static void session_cookiejar_init(SessionCookieJar *self);
static void session_cookiejar_set_property(GObject *self, guint prop_id,
    const GValue *value, GParamSpec *pspec);

extern VbCore vb;
extern const unsigned int SETTING_MAX_CONNS;
extern const unsigned int SETTING_MAX_CONNS_PER_HOST;


void session_init(void)
{
    /* init soup session */
    vb.soup_session = webkit_get_default_session();
    soup_session_add_feature(
        vb.soup_session,
        SOUP_SESSION_FEATURE(session_cookiejar_new(vb.files[FILES_COOKIE], false))
    );
    g_object_set(vb.soup_session, "max-conns", SETTING_MAX_CONNS , NULL);
    g_object_set(vb.soup_session, "max-conns-per-host", SETTING_MAX_CONNS_PER_HOST, NULL);

}

static SoupCookieJar *session_cookiejar_new(const char *file, gboolean ro)
{
    return g_object_new(
        SESSION_COOKIEJAR_TYPE, SOUP_COOKIE_JAR_TEXT_FILENAME, file, SOUP_COOKIE_JAR_READ_ONLY, ro, NULL
    );
}

static void session_cookiejar_changed(SoupCookieJar *self, SoupCookie *old_cookie, SoupCookie *new_cookie)
{
    flock(SESSION_COOKIEJAR(self)->lock, LOCK_EX);
    if (new_cookie && !new_cookie->expires) {
        soup_cookie_set_expires(new_cookie, soup_date_new_from_now(vb.config.cookie_timeout));
    }
    SOUP_COOKIE_JAR_CLASS(session_cookiejar_parent_class)->changed(self, old_cookie, new_cookie);
    flock(SESSION_COOKIEJAR(self)->lock, LOCK_UN);
}

static void session_cookiejar_class_init(SessionCookieJarClass *class)
{
    SOUP_COOKIE_JAR_CLASS(class)->changed = session_cookiejar_changed;
    G_OBJECT_CLASS(class)->get_property   = G_OBJECT_CLASS(session_cookiejar_parent_class)->get_property;
    G_OBJECT_CLASS(class)->set_property   = session_cookiejar_set_property;
    G_OBJECT_CLASS(class)->finalize       = session_cookiejar_finalize;
    g_object_class_override_property(G_OBJECT_CLASS(class), 1, "filename");
}

static void session_cookiejar_finalize(GObject *self)
{
    close(SESSION_COOKIEJAR(self)->lock);
    G_OBJECT_CLASS(session_cookiejar_parent_class)->finalize(self);
}

static void session_cookiejar_init(SessionCookieJar *self)
{
    self->lock = open(vb.files[FILES_COOKIE], 0);
}

static void session_cookiejar_set_property(GObject *self, guint prop_id, const
    GValue *value, GParamSpec *pspec)
{
    flock(SESSION_COOKIEJAR(self)->lock, LOCK_SH);
    G_OBJECT_CLASS(session_cookiejar_parent_class)->set_property(self, prop_id, value, pspec);
    flock(SESSION_COOKIEJAR(self)->lock, LOCK_UN);
}
