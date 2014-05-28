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

#ifndef _HSTS_H
#define _HSTS_H

#include <glib-object.h>
#include <libsoup/soup.h>

#define HSTS_TYPE_PROVIDER            (hsts_provider_get_type())
#define HSTS_PROVIDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), HSTS_TYPE_PROVIDER, HSTSProvider))
#define HSTS_PROVIDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), HSTS_TYPE_PROVIDER, HSTSProviderClass))
#define HSTS_IS_PROVIDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), HSTS_TYPE_PROVIDER))
#define HSTS_IS_PROVIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), HSTS_TYPE_PROVIDER))
#define HSTS_PROVIDER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), HSTS_TYPE_PROVIDER, HSTSProviderClass))

/* public interface of the provider */
typedef struct {
    GObject parent_instance;
} HSTSProvider;

/* class members of the provider */
typedef struct {
    GObjectClass parent_class;
} HSTSProviderClass;


GType hsts_provider_get_type(void);
HSTSProvider *hsts_provider_new(void);
void hsts_prepare_message(SoupSession* session, SoupMessage *msg);

#endif /* end of include guard: _HSTS_H */
#endif
