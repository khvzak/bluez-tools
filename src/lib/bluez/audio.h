/*
 *
 *  bluez-tools - a set of tools to manage bluetooth devices for linux
 *
 *  Copyright (C) 2010  Alexander Orlenko <zxteam@gmail.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef __AUDIO_H
#define __AUDIO_H

#include <glib-object.h>

#define AUDIO_DBUS_INTERFACE "org.bluez.Audio"

/*
 * Type macros
 */
#define AUDIO_TYPE				(audio_get_type())
#define AUDIO(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), AUDIO_TYPE, Audio))
#define AUDIO_IS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), AUDIO_TYPE))
#define AUDIO_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), AUDIO_TYPE, AudioClass))
#define AUDIO_IS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), AUDIO_TYPE))
#define AUDIO_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), AUDIO_TYPE, AudioClass))

typedef struct _Audio Audio;
typedef struct _AudioClass AudioClass;
typedef struct _AudioPrivate AudioPrivate;

struct _Audio {
	GObject parent_instance;

	/*< private >*/
	AudioPrivate *priv;
};

struct _AudioClass {
	GObjectClass parent_class;
};

/* used by AUDIO_TYPE */
GType audio_get_type(void) G_GNUC_CONST;

/*
 * Method definitions
 */
void audio_connect(Audio *self, GError **error);
void audio_disconnect(Audio *self, GError **error);
GHashTable *audio_get_properties(Audio *self, GError **error);

const gchar *audio_get_dbus_object_path(Audio *self);
const gchar *audio_get_state(Audio *self);

#endif /* __AUDIO_H */

