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
#ifdef FEATURE_HSTS
#include "hsts.h"
#include "util.h"
#include "main.h"
#include <string.h>
#include <glib-object.h>
#include <libsoup/soup.h>

#define HSTS_HEADER_NAME "Strict-Transport-Security"
#define HSTS_FILE_FORMAT "%s\t%s\t%c\n"
#define HSTS_PROVIDER_GET_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE((o), HSTS_TYPE_PROVIDER, HSTSProviderPrivate))

extern VbCore vb;

/* private interface of the provider */
typedef struct _HSTSProviderPrivate {
    GHashTable* whitelist;
} HSTSProviderPrivate;

typedef struct {
    SoupDate *expires_at;
    gboolean include_sub_domains;
} HSTSEntry;

static void hsts_provider_class_init(HSTSProviderClass *klass);
static void hsts_provider_init(HSTSProvider *self);
static void hsts_provider_finalize(GObject* obj);
static inline gboolean should_secure_host(HSTSProvider *provider,
    const char *host);
static void process_hsts_header(SoupMessage *msg, gpointer data);
static void parse_hsts_header(HSTSProvider *provider,
    const char *host, const char *header);
static void free_entry(HSTSEntry *entry);
static void add_host_entry(HSTSProvider *provider, const char *host,
    HSTSEntry *entry);
static void add_host_entry_to_file(HSTSProvider *provider, const char *host,
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
/* caching related functions */
static void load_entries(HSTSProvider *provider, const char *file);
static void save_entries(HSTSProvider *provider, const char *file);


/**
 * Generates a new hsts provider instance.
 * Unref the instance with g_object_unref if no more used.
 */
HSTSProvider *hsts_provider_new(void)
{
    return g_object_new(HSTS_TYPE_PROVIDER, NULL);
}

/**
 * Change scheme and port of soup messages uri if the host is a known and
 * valid hsts host.
 * This logic should be implemented in request_queued function but the changes
 * that are done there to the uri do not appear in webkit_web_view_get_uri().
 * If a valid hsts host is requested via http and the url is changed to https
 * vimb would still show the http uri in url bar. This seems to be a
 * missbehaviour in webkit, but for now we provide this function to put in the
 * logic in the scope of the resource-request-starting event of the webview.
 */
void hsts_prepare_message(SoupSession* session, SoupMessage *msg)
{
    SoupSessionFeature *feature;
    HSTSProvider *provider;
    SoupURI *uri;

    feature = soup_session_get_feature_for_message(session, HSTS_TYPE_PROVIDER, msg);
    uri     = soup_message_get_uri(msg);
    if (!feature || !uri) {
        return;
    }

    provider = HSTS_PROVIDER(feature);
    if (should_secure_host(provider, uri->host)) {
        /* the ports is set by soup uri if scheme is changed */
        soup_uri_set_scheme(uri, SOUP_URI_SCHEME_HTTPS);
    }
}

G_DEFINE_TYPE_WITH_CODE(
    HSTSProvider, hsts_provider, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE(SOUP_TYPE_SESSION_FEATURE, session_feature_init)
)

static void hsts_provider_class_init(HSTSProviderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    hsts_provider_parent_class = g_type_class_peek_parent(klass);
    g_type_class_add_private(klass, sizeof(HSTSProviderPrivate));
    object_class->finalize = hsts_provider_finalize;
}

static void hsts_provider_init(HSTSProvider *self)
{
    /* initialize private fields */
    HSTSProviderPrivate *priv = HSTS_PROVIDER_GET_PRIVATE(self);
    priv->whitelist = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)free_entry);

    /* load entries from hsts file */
    load_entries(self, vb.files[FILES_HSTS]);
}

static void hsts_provider_finalize(GObject* obj)
{
    HSTSProviderPrivate *priv = HSTS_PROVIDER_GET_PRIVATE (obj);

    /* save all the entries in hsts file */
    save_entries(HSTS_PROVIDER(obj), vb.files[FILES_HSTS]);

    g_hash_table_destroy(priv->whitelist);
    G_OBJECT_CLASS(hsts_provider_parent_class)->finalize(obj);
}

/**
 * Checks if given host is a known https host according to RFC 6797 8.2f
 */
static inline gboolean should_secure_host(HSTSProvider *provider,
    const char *host)
{
    HSTSProviderPrivate *priv = HSTS_PROVIDER_GET_PRIVATE(provider);
    HSTSEntry *entry;
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
        /* Try to find the whole congruent matching host in hash table - if
         * not found strip of the first label and try to find a superdomain
         * match. Specified is a from right to left comparison 8.3, but in the
         * end this should be lead to the same result. */
        while (p != NULL) {
            entry = g_hash_table_lookup(priv->whitelist, p);
            if (entry != NULL) {
                /* remove expired entries RFC 6797 8.1.1 */
                if (soup_date_is_past(entry->expires_at)) {
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
    SoupMessageHeaders *hdrs;
    const char *header;

    if (!g_hostname_is_ip_address(uri->host)
        && (soup_message_get_flags(msg) & SOUP_MESSAGE_CERTIFICATE_TRUSTED)
    ){
        g_object_get(G_OBJECT(msg), SOUP_MESSAGE_RESPONSE_HEADERS, &hdrs, NULL);

        /* TODO according to RFC 6797 8.1 we must only use the first header */
        header = soup_message_headers_get_one(hdrs, HSTS_HEADER_NAME);
        if (header) {
            parse_hsts_header(provider, uri->host, header);
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
    HSTSEntry *entry;
    int max_age = G_MAXINT;
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
            entry = g_slice_new(HSTSEntry);
            entry->expires_at          = soup_date_new_from_now(max_age);
            entry->include_sub_domains = include_sub_domains;

            add_host_entry(provider, host, entry);
            add_host_entry_to_file(provider, host, entry);
        }
    }
}

static void free_entry(HSTSEntry *entry)
{
    soup_date_free(entry->expires_at);
    g_slice_free(HSTSEntry, entry);
}

/**
 * Adds the host to the known host, if it already exists it replaces it with
 * the information contained in entry according to RFC 6797 8.1.
 */
static void add_host_entry(HSTSProvider *provider, const char *host,
    HSTSEntry *entry)
{
    HSTSProviderPrivate *priv = HSTS_PROVIDER_GET_PRIVATE(provider);
    g_hash_table_replace(priv->whitelist, g_hostname_to_unicode(host), entry);
}

static void add_host_entry_to_file(HSTSProvider *provider, const char *host,
    HSTSEntry *entry)
{
    char *date = soup_date_to_string(entry->expires_at, SOUP_DATE_ISO8601_FULL);

    util_file_append(
        vb.files[FILES_HSTS], HSTS_FILE_FORMAT, host, date, entry->include_sub_domains ? 'y' : 'n'
    );
    g_free(date);
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
    SoupURI *uri = soup_message_get_uri(msg);

    /* only look for HSTS headers sent over https RFC 6797 7.2*/
    if (uri->scheme == SOUP_URI_SCHEME_HTTPS) {
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
    GTlsCertificate *certificate;
    GTlsCertificateFlags errors;

    if (should_secure_host(provider, uri->host)) {
        if (uri->scheme != SOUP_URI_SCHEME_HTTPS
            || (soup_message_get_https_status(msg, &certificate, &errors) && errors)
        ) {
            soup_session_cancel_message(session, msg, SOUP_STATUS_SSL_FAILED);
            g_warning("cancel invalid hsts request to %s://%s", uri->scheme, uri->host);
        }
    }
}

static void request_unqueued(SoupSessionFeature *feature,
    SoupSession *session, SoupMessage *msg)
{
    g_signal_handlers_disconnect_by_func(msg, process_hsts_header, feature);
}

/**
 * Reads the entries save in given file and store them in the whitelist.
 */
static void load_entries(HSTSProvider *provider, const char *file)
{
    char **lines, **parts, *host, *line;
    int i, len, partlen;
    gboolean include_sub_domains;
    SoupDate *date;
    HSTSEntry *entry;

    lines = util_get_lines(file);
    if (!(len = g_strv_length(lines))) {
        return;
    }

    for (i = len - 1; i >= 0; i--) {
        line = lines[i];
        /* skip empty or commented lines */
        if (!*line || *line == '#') {
            continue;
        }

        parts   = g_strsplit(line, "\t", 3);
        partlen = g_strv_length(parts);
        if (partlen == 3) {
            host = parts[0];
            if (g_hostname_is_ip_address(host)) {
                continue;
            }
            date = soup_date_new_from_string(parts[1]);
            if (!date) {
                continue;
            }
            include_sub_domains = (*parts[2] == 'y') ? true : false;

            /* built the new entry to add */
            entry = g_slice_new(HSTSEntry);
            entry->expires_at          = soup_date_new_from_string(parts[1]);
            entry->include_sub_domains = include_sub_domains;

            add_host_entry(provider, host, entry);
        } else {
            g_warning("could not parse hsts line '%s'", line);
        }
        g_strfreev(parts);
    }
    g_strfreev(lines);
}

/**
 * Saves all entries of given provider in given file.
 */
static void save_entries(HSTSProvider *provider, const char *file)
{
    GHashTableIter iter;
    char *host, *date;
    HSTSEntry *entry;
    FILE *f;
    HSTSProviderPrivate *priv = HSTS_PROVIDER_GET_PRIVATE(provider);

    if ((f = fopen(file, "w"))) {
        FLOCK(fileno(f), F_WRLCK);

        g_hash_table_iter_init(&iter, priv->whitelist);
        while (g_hash_table_iter_next (&iter, (gpointer)&host, (gpointer)&entry)) {
            date = soup_date_to_string(entry->expires_at, SOUP_DATE_ISO8601_FULL);
            fprintf(f, HSTS_FILE_FORMAT, host, date, entry->include_sub_domains ? 'y' : 'n');
            g_free(date);
        }

        FLOCK(fileno(f), F_UNLCK);
        fclose(f);
    }
}
#endif
