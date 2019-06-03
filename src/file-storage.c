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

#include <glib.h>
#include <stdio.h>
#include <sys/file.h>
#include <glib/gstdio.h>

#include "file-storage.h"

struct filestorage {
    char        *file_path;
    gboolean    readonly;
    GString     *str;
};

/**
 * Create new file storage instance for given directory and filename. If the
 * file does not exists in the directory and give mode is not 0 the file is
 * created with the given mode.
 *
 * The returned FileStorage must be freed by file_storage_free().
 *
 * @dir:        Directory in which the file is searched.
 * @filename:   Filename to built the absolute path with.
 * @mode:       Mode (file permission as chmod(2)) used for the file when
 *              creating it. If 0 the file is not created and the storage is
 *              used in read only mode - no data written to the file.
 */
FileStorage *file_storage_new(const char *dir, const char *filename, gboolean readonly)
{
    FileStorage *storage;

    storage            = g_slice_new(FileStorage);
    storage->readonly  = readonly;
    storage->file_path = g_build_filename(dir, filename, NULL);

    /* Use gstring as storage in case when the file is used read only. */
    if (storage->readonly) {
        storage->str = g_string_new(NULL);
    } else {
        storage->str = NULL;
    }

    return storage;
}

/**
 * Free memory for given file storage.
 */
void file_storage_free(FileStorage *storage)
{
    if (storage) {
        g_free(storage->file_path);
        if (storage->str) {
            g_string_free(storage->str, TRUE);
        }
        g_slice_free(FileStorage, storage);
    }
}

/**
 * Append new data to file.
 *
 * @fileStorage: FileStorage to append the data to
 * @format: Format string used to process va_list
 */
gboolean file_storage_append(FileStorage *storage, const char *format, ...)
{
    FILE *f;
    va_list args;

    g_assert(storage);

    /* Write data to in memory list in case the file storage is read only. */
    if (storage->readonly) {
        va_start(args, format);
        g_string_append_vprintf(storage->str, format, args);
        va_end(args);
        return TRUE;
    }
    if ((f = fopen(storage->file_path, "a+"))) {
        flock(fileno(f), LOCK_EX);
        va_start(args, format);
        vfprintf(f, format, args);
        va_end(args);
        flock(fileno(f), LOCK_UN);
        fclose(f);
        return TRUE;
    }

    return FALSE;
}

/**
 * Retrieves all the lines from file storage.
 *
 * The result have to be freed by g_strfreev().
 */
char **file_storage_get_lines(FileStorage *storage)
{
    char *fullcontent = NULL;
    char *content     = NULL;
    char **lines      = NULL;

    g_file_get_contents(storage->file_path, &content, NULL, NULL);

    if (storage->str && storage->str->len) {
        if (content) {
            fullcontent = g_strconcat(content, storage->str->str, NULL);
            lines       = g_strsplit(fullcontent, "\n", -1);
            g_free(fullcontent);
        } else {
            lines = g_strsplit(storage->str->str, "\n", -1);
        }
    } else {
        lines = g_strsplit(content ? content : "", "\n", -1);
    }

    if (content) {
        g_free(content);
    }

    return lines;
}

const char *file_storage_get_path(FileStorage *storage)
{
    return storage->file_path;
}

gboolean file_storage_is_readonly(FileStorage *storage)
{
    return storage->readonly;
}
