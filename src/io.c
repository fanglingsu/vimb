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
#ifdef FEATURE_FIFO
#include "io.h"
#include "main.h"
#include "map.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

extern VbCore vb;

static gboolean fifo_watch(GIOChannel *gio, GIOCondition condition);


gboolean io_init_fifo(const char *name)
{
    char *fname, *path;

    /* create fifo in directory as vimb-fifo-$PID */
    fname = g_strdup_printf(PROJECT "-fifo-%s", name);
    path  = g_build_filename(g_get_user_config_dir(), PROJECT, fname, NULL);
    g_free(fname);

    /* if the file already exists - remove this first */
    if (g_file_test(path, G_FILE_TEST_IS_REGULAR)) {
        unlink(path);
    }

    if (mkfifo(path, 0666) == 0) {
        GError *error    = NULL;
        /* use r+ to avoid blocking caused by wait of a writer */
        GIOChannel *chan = g_io_channel_new_file(path, "r+", &error);
        if (chan) {
            if (g_io_add_watch(chan, G_IO_IN|G_IO_HUP, (GIOFunc)fifo_watch, NULL)) {
                /* don't free path - because we want to keep the value in
                 * vb.state.fifo_path still accessible */
                vb.state.fifo_path = path;
                g_setenv("VIMB_FIFO", path, TRUE);

                return true;
            }
        } else {
            g_warning ("Can't open fifo: %s", error->message);
            g_error_free(error);
        }
    } else {
        g_warning("Can't create fifo %s", path);
    }
    g_free(path);

    return false;
}

void io_cleanup(void)
{
    if (vb.state.fifo_path) {
        if (unlink(vb.state.fifo_path) == -1) {
            g_warning("Can't remove fifo %s", vb.state.fifo_path);
        }
        g_free(vb.state.fifo_path);
        vb.state.fifo_path = NULL;
    }
}

static gboolean fifo_watch(GIOChannel *gio, GIOCondition condition)
{
    char *line;
    GIOStatus ret;
    GError *err = NULL;

    if (condition & G_IO_HUP) {
        g_error("fifo: read end of pipe died");
    }

    if (!gio) {
        g_error("fifo: GIOChannel broken");
    }

    ret = g_io_channel_read_line(gio, &line, NULL, NULL, &err);
    if (ret == G_IO_STATUS_ERROR) {
        g_error("fifo: error reading from fifo: %s", err->message);
        g_error_free(err);
    }

    map_handle_string(line, true);
    g_free(line);

    return true;
}

#endif
