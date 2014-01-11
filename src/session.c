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
#include <sys/file.h>
#include "main.h"
#include "session.h"

#ifdef FEATURE_COOKIE

#define COOKIEJAR_TYPE (cookiejar_get_type())
#define COOKIEJAR(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), COOKIEJAR_TYPE, CookieJar))

typedef struct {
    SoupCookieJarText parent_instance;
    int lock;
} CookieJar;

typedef struct {
    SoupCookieJarTextClass parent_class;
} CookieJarClass;

static GType cookiejar_get_type(void);

G_DEFINE_TYPE(CookieJar, cookiejar, SOUP_TYPE_COOKIE_JAR_TEXT)

static SoupCookieJar *cookiejar_new(const char *file, gboolean ro);
static void cookiejar_changed(SoupCookieJar *self, SoupCookie *old, SoupCookie *new);
static void cookiejar_class_init(CookieJarClass *class);
static void cookiejar_finalize(GObject *self);
static void cookiejar_init(CookieJar *self);
static void cookiejar_set_property(GObject *self, guint prop_id,
    const GValue *value, GParamSpec *pspec);
#endif
static void request_started_cb(SoupSession *session, SoupMessage *msg, gpointer data);

extern VbCore vb;


void session_init(void)
{
    /* init soup session */
    vb.session = webkit_get_default_session();
    g_object_set(vb.session, "max-conns", SETTING_MAX_CONNS , NULL);
    g_object_set(vb.session, "max-conns-per-host", SETTING_MAX_CONNS_PER_HOST, NULL);
    g_object_set(vb.session, "accept-language-auto", true, NULL);

    g_signal_connect(
        vb.session, "request-started",
        G_CALLBACK(request_started_cb), NULL
    );
#ifdef FEATURE_COOKIE
    soup_session_add_feature(
        vb.session,
        SOUP_SESSION_FEATURE(cookiejar_new(vb.files[FILES_COOKIE], false))
    );
#endif
}

#ifdef FEATURE_COOKIE
static SoupCookieJar *cookiejar_new(const char *file, gboolean ro)
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
    flock(COOKIEJAR(self)->lock, LOCK_EX);
    SoupDate *expire;
    if (new_cookie && !new_cookie->expires && vb.config.cookie_timeout) {
        expire = soup_date_new_from_now(vb.config.cookie_timeout);
        soup_cookie_set_expires(new_cookie, expire);
        soup_date_free(expire);
    }
    SOUP_COOKIE_JAR_CLASS(cookiejar_parent_class)->changed(self, old_cookie, new_cookie);
    flock(COOKIEJAR(self)->lock, LOCK_UN);
}

static void cookiejar_class_init(CookieJarClass *class)
{
    SOUP_COOKIE_JAR_CLASS(class)->changed = cookiejar_changed;
    G_OBJECT_CLASS(class)->get_property   = G_OBJECT_CLASS(cookiejar_parent_class)->get_property;
    G_OBJECT_CLASS(class)->set_property   = cookiejar_set_property;
    G_OBJECT_CLASS(class)->finalize       = cookiejar_finalize;
    g_object_class_override_property(G_OBJECT_CLASS(class), 1, "filename");
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
    flock(COOKIEJAR(self)->lock, LOCK_SH);
    G_OBJECT_CLASS(cookiejar_parent_class)->set_property(self, prop_id, value, pspec);
    flock(COOKIEJAR(self)->lock, LOCK_UN);
}
#endif

static void request_started_cb(SoupSession *session, SoupMessage *msg, gpointer data)
{
    GHashTableIter iter;
    char *name, *value;

    if (!vb.config.headers) {
        return;
    }

    g_hash_table_iter_init(&iter, vb.config.headers);
    while (g_hash_table_iter_next(&iter, (gpointer*)&name, (gpointer*)&value)) {
        /* allow to remove header with null value */
        if (value == NULL) {
            soup_message_headers_remove(msg->request_headers, name);
        } else {
            soup_message_headers_replace(msg->request_headers, name, value);
        }
    }
}
