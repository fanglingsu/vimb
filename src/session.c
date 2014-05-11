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
#include "main.h"
#include "session.h"
#ifdef FEATURE_COOKIE
#include "cookiejar.h"
#endif
#ifdef FEATURE_HSTS
#include "hsts.h"
#endif

extern VbCore vb;


void session_init(void)
{
    /* init soup session */
    vb.session = webkit_get_default_session();
    g_object_set(vb.session, "max-conns", SETTING_MAX_CONNS , NULL);
    g_object_set(vb.session, "max-conns-per-host", SETTING_MAX_CONNS_PER_HOST, NULL);
    g_object_set(vb.session, "accept-language-auto", true, NULL);

#ifdef FEATURE_COOKIE
    SoupCookieJar *cookie = cookiejar_new(vb.files[FILES_COOKIE], false);
    soup_session_add_feature(vb.session, SOUP_SESSION_FEATURE(cookie));
    g_object_unref(cookie);
#endif
#ifdef FEATURE_HSTS
    HSTSProvider *hsts = hsts_provider_new();
    soup_session_add_feature(vb.session, SOUP_SESSION_FEATURE(hsts));
    g_object_unref(hsts);
#endif
}

void session_cleanup(void)
{
#ifdef FEATURE_HSTS
    /* remove feature from session to make sure the feature is finalized */
    soup_session_remove_feature_by_type(vb.session, HSTS_TYPE_PROVIDER);
#endif
}
