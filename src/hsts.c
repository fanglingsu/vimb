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
#include "hsts.h"
#include <string.h>
#include <glib-object.h>
#include <libsoup/soup.h>

#define HSTS_HEADER_NAME "Strict-Transport-Security"
#define HSTS_PROVIDER_GET_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE((o), HSTS_TYPE_PROVIDER, HSTSProviderPrivate))

/* private interface of the provider */
typedef struct _HSTSProviderPrivate {
    GHashTable* whitelist;
} HSTSProviderPrivate;

typedef struct {
    gint64   expires_at;
    gboolean include_sub_domains;
} HSTSEntry;

static void hsts_provider_class_init(HSTSProviderClass *klass);
static void hsts_provider_init(HSTSProvider *self);
static void hsts_provider_finalize(GObject* obj);
static gboolean should_secure_host(HSTSProvider *provider,
    const char *host);
static void process_hsts_header(SoupMessage *msg, gpointer data);
static void parse_hsts_header(HSTSProvider *provider,
    const char *host, const char *header);
static HSTSEntry *get_new_entry(gint64 max_age, gboolean include_sub_domains);
static void add_host_entry(HSTSProvider *provider, const char *host,
    HSTSEntry *entry);
static void remove_host_entry(HSTSProvider *provider, const char *host);
/* session feature related functions */
static void session_feature_init(
    SoupSessionFeatureInterface *inteface, gpointer data);
static void request_queued(SoupSessionFeature *feature,
    SoupSession *session, SoupMessage *msg);
static void request_started(SoupSessionFeature *feature,
    SoupSession *session, SoupMessage *msg, SoupSocket *socket);
static void request_unqueued(SoupSessionFeature *feature,
    SoupSession *session, SoupMessage *msg);


/**
 * Generates a new hsts provider instance.
 * Unref the instance with g_object_unref if no more used.
 */
HSTSProvider *hsts_provider_new(void)
{
    return g_object_new(HSTS_TYPE_PROVIDER, NULL);
}

G_DEFINE_TYPE_WITH_CODE(
    HSTSProvider, hsts_provider, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(SOUP_TYPE_SESSION_FEATURE, session_feature_init)
)

static void hsts_provider_class_init(HSTSProviderClass *klass)
{
    hsts_provider_parent_class = g_type_class_peek_parent(klass);
    g_type_class_add_private(klass, sizeof(HSTSProviderPrivate));
    G_OBJECT_CLASS(klass)->finalize = hsts_provider_finalize;
}

static void hsts_provider_init(HSTSProvider *self)
{
    /* Initialize private fields */
    HSTSProviderPrivate *priv = HSTS_PROVIDER_GET_PRIVATE(self);
    priv->whitelist = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
}

static void hsts_provider_finalize(GObject* obj)
{
    HSTSProviderPrivate *priv = HSTS_PROVIDER_GET_PRIVATE (obj);

    g_hash_table_destroy(priv->whitelist);
    G_OBJECT_CLASS(hsts_provider_parent_class)->finalize(obj);
}

/**
 * Checks if given host is a known https host according to RFC 6797 8.3
 */
static gboolean should_secure_host(HSTSProvider *provider,
    const char *host)
{
    HSTSProviderPrivate *priv = HSTS_PROVIDER_GET_PRIVATE(provider);
    char *canonical, *p;
    gboolean result = false, is_subdomain = false;

    /* ip is not allowed for hsts */
    if (g_hostname_is_ip_address(host)) {
        return false;
    }

    canonical = g_hostname_to_ascii(host);
    /* don't match empty host */
    if (*canonical) {
        p = canonical;
        while (p != NULL) {
            HSTSEntry *entry = g_hash_table_lookup(priv->whitelist, p);
            if (entry != NULL) {
                /* remove expired entries */
                if (g_get_real_time() > entry->expires_at) {
                    remove_host_entry(provider, p);
                } else if(!is_subdomain || entry->include_sub_domains) {
                    result = true;
                    break;
                }
            }

            is_subdomain = true;
            /* test without the first domain part */
            if ((p = strchr(p, '.'))) {
                p++;
            }
        }
    }
    g_free(canonical);

    return result;
}

static void process_hsts_header(SoupMessage *msg, gpointer data)
{
    HSTSProvider *provider = (HSTSProvider*)data;
    SoupURI *uri           = soup_message_get_uri(msg);
    const char *header, *host = soup_uri_get_host(uri);
    SoupMessageHeaders *hdrs;

    if (!g_hostname_is_ip_address(host)
        && (soup_message_get_flags(msg) & SOUP_MESSAGE_CERTIFICATE_TRUSTED)
    ){
        g_object_get(G_OBJECT(msg), SOUP_MESSAGE_RESPONSE_HEADERS, &hdrs, NULL);

        /* TODO according to RFC 6797 8.1 we must only use the first header */
        header = soup_message_headers_get_one(hdrs, HSTS_HEADER_NAME);
        if (header) {
            parse_hsts_header(provider, host, header);
        }
    }
}

/**
 * Parses the hsts directives from given header like specified in RFC 6797 6.1
 */
static void parse_hsts_header(HSTSProvider *provider,
    const char *host, const char *header)
{
    GHashTable *directives = soup_header_parse_semi_param_list(header);
    gint64 max_age = 0;
    gboolean include_sub_domains = false;
    GHashTableIter iter;
    gpointer key, value;
    gboolean success = true;

    HSTSProviderClass *klass = g_type_class_ref(HSTS_TYPE_PROVIDER);

    g_hash_table_iter_init(&iter, directives);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        /* parse the max-age directive */
        if (!g_ascii_strncasecmp(key, "max-age", 7)) {
            /* max age needs a value */
            if (value) {
                max_age = g_ascii_strtoll(value, NULL, 10);
                if (max_age < 0) {
                    success = false;
                    break;
                }
            } else {
                success = false;
                break;
            }
        } else if (g_ascii_strncasecmp(key, "includeSubDomains", 17)) {
            /* includeSubDomains must not have a value */
            if (!value) {
                include_sub_domains = true;
            } else {
                success = false;
                break;
            }
        }
    }
    soup_header_free_param_list(directives);
    g_type_class_unref(klass);

    if (success) {
        /* remove host if max-age = 0 RFC 6797 6.1.1 */
        if (max_age == 0) {
            remove_host_entry(provider, host);
        } else {
            add_host_entry(provider, host, get_new_entry(max_age, include_sub_domains));
        }
    }
}

/**
 * Create a new hsts entry for given data.
 * Returned entry have to be freed if no more used.
 */
static HSTSEntry *get_new_entry(gint64 max_age, gboolean include_sub_domains)
{
    HSTSEntry *entry  = g_new(HSTSEntry, 1);
    entry->expires_at = g_get_real_time();
    if (max_age > (G_MAXINT64 - entry->expires_at)/G_USEC_PER_SEC) {
        entry->expires_at = G_MAXINT64;
    } else {
        entry->expires_at += max_age * G_USEC_PER_SEC;
    }
    entry->include_sub_domains = include_sub_domains;

    return entry;
}


/**
 * Adds the host to the known host, if it already exists it replaces it with
 * the information contained in entry according to RFC 6797 8.1.
 */
static void add_host_entry(HSTSProvider *provider, const char *host,
    HSTSEntry *entry)
{
    if (g_hostname_is_ip_address(host)) {
        return;
    }

    HSTSProviderPrivate *priv = HSTS_PROVIDER_GET_PRIVATE(provider);
    g_hash_table_replace(priv->whitelist, g_hostname_to_unicode(host), entry);
}

/**
 * Removes stored entry for given host.
 */
static void remove_host_entry(HSTSProvider *provider, const char *host)
{
    HSTSProviderPrivate *priv = HSTS_PROVIDER_GET_PRIVATE(provider);
    char *canonical           = g_hostname_to_unicode(host);

    g_hash_table_remove(priv->whitelist, canonical);
    g_free(canonical);
}

/**
 * Initialise the SoupSessionFeature interface.
 */
static void session_feature_init(
    SoupSessionFeatureInterface *inteface, gpointer data)
{
    inteface->request_queued   = request_queued;
    inteface->request_started  = request_started;
    inteface->request_unqueued = request_unqueued;
}

/**
 * Check if the host is known and switch the URI scheme to https.
 */
static void request_queued(SoupSessionFeature *feature,
    SoupSession *session, SoupMessage *msg)
{
    HSTSProvider *provider = HSTS_PROVIDER(feature);
    SoupURI *uri           = soup_message_get_uri(msg);
    if (soup_uri_get_scheme(uri) == SOUP_URI_SCHEME_HTTP
        && should_secure_host(provider, soup_uri_get_host(uri))
    ) {
        soup_uri_set_scheme(uri, SOUP_URI_SCHEME_HTTPS);
        /* change port if the is explicitly set */
        if (soup_uri_get_port(uri) == 80) {
            soup_uri_set_port(uri, 443);
        }
        soup_session_requeue_message(session, msg);
    }

    /* Only look for HSTS headers sent over https */
    if (soup_uri_get_scheme(uri) == SOUP_URI_SCHEME_HTTPS) {
        soup_message_add_header_handler(
            msg, "got-headers", HSTS_HEADER_NAME, G_CALLBACK(process_hsts_header), feature
        );
    }
}

static void request_started(SoupSessionFeature *feature,
    SoupSession *session, SoupMessage *msg, SoupSocket *socket)
{
    HSTSProvider *provider = HSTS_PROVIDER(feature);
    SoupURI *uri           = soup_message_get_uri(msg);
    const char *host       = soup_uri_get_host(uri);
    if (should_secure_host(provider, host)) {
        if (soup_uri_get_scheme(uri) != SOUP_URI_SCHEME_HTTPS
            || !(soup_message_get_flags(msg) & SOUP_MESSAGE_CERTIFICATE_TRUSTED)
        ) {
            soup_session_cancel_message(session, msg, SOUP_STATUS_SSL_FAILED);
        }
    }
}

static void request_unqueued(SoupSessionFeature *feature,
    SoupSession *session, SoupMessage *msg)
{
    g_signal_handlers_disconnect_by_func(msg, process_hsts_header, feature);
}
