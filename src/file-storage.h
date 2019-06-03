/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2019 Daniel Carl
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

#ifndef _FILE_STORAGE_H
#define _FILE_STORAGE_H

#include <glib.h>

typedef struct filestorage FileStorage;
FileStorage *file_storage_new(const char *dir, const char *filename, int mode);
void file_storage_free(FileStorage *storage);
gboolean file_storage_append(FileStorage *storage, const char *format, ...);
char **file_storage_get_lines(FileStorage *storage);
const char *file_storage_get_path(FileStorage *storage);
gboolean file_storage_is_readonly(FileStorage *storage);

#endif /* end of include guard: _FILE_STORAGE_H */
