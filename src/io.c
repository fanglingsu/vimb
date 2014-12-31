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
#ifdef FEATURE_SOCKET
#include "io.h"
#include "main.h"
#include "map.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

extern VbCore vb;

static gboolean socket_accept(GIOChannel *chan);
static gboolean socket_watch(GIOChannel *chan);


gboolean io_init_socket(const char *name)
{
    char *fname, *path;
    int sock;
    struct sockaddr_un local;

    /* create socket in directory as vimb-socket-$PID */
    fname = g_strdup_printf(PROJECT "-socket-%s", name);
    path  = g_build_filename(g_get_user_config_dir(), PROJECT, fname, NULL);
    g_free(fname);

    /* if the file already exists - remove this first */
    if (g_file_test(path, G_FILE_TEST_IS_REGULAR)) {
        unlink(path);
    }

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        g_warning("Can't create socket %s", path);
    }

    /* prepare socket address */
    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, path);

    if (bind(sock, (struct sockaddr*)&local, sizeof(local)) != -1
        && listen(sock, 5) >= 0
    ) {
        GIOChannel *chan = g_io_channel_unix_new(sock);
        if (chan) {
            g_io_channel_set_encoding(chan, NULL, NULL);
            g_io_channel_set_buffered(chan, false);
            g_io_add_watch(chan, G_IO_IN|G_IO_HUP, (GIOFunc)socket_accept, chan);
            /* don't free path - because we want to keep the value in
             * vb.state.socket_path still accessible */
            vb.state.socket_path = path;
            g_setenv("VIMB_SOCKET", path, true);

            return true;
        }
    } else {
        g_warning("no bind");
    }

    g_warning("Could not listen on %s: %s", path, strerror(errno));
    g_free(path);

    return false;
}

void io_cleanup(void)
{
    if (vb.state.socket_path) {
        if (unlink(vb.state.socket_path) == -1) {
            g_warning("Can't remove socket %s", vb.state.socket_path);
        }
        g_free(vb.state.socket_path);
        vb.state.socket_path = NULL;
    }
}

static gboolean socket_accept(GIOChannel *chan)
{
    struct sockaddr_un remote;
    guint size = sizeof(remote);
    GIOChannel *iochan;
    int clientsock;

    clientsock = accept(g_io_channel_unix_get_fd(chan), (struct sockaddr *)&remote, &size);

    if ((iochan = g_io_channel_unix_new(clientsock))) {
        g_io_channel_set_encoding(iochan, NULL, NULL);
        g_io_add_watch(iochan, G_IO_IN|G_IO_HUP, (GIOFunc)socket_watch, iochan);
    }
    return true;
}

static gboolean socket_watch(GIOChannel *chan)
{
    GIOStatus ret;
    GError *error = NULL;
    char *line, *inputtext;
    gsize len;

    ret = g_io_channel_read_line(chan, &line, &len, NULL, &error);
    if (ret == G_IO_STATUS_ERROR || ret == G_IO_STATUS_EOF) {
        if (ret == G_IO_STATUS_ERROR) {
            g_warning("Error reading: %s", error->message);
            g_error_free(error);
        }

        /* shutdown and remove the client channel */
        ret = g_io_channel_shutdown(chan, true, &error);
        g_io_channel_unref(chan);

        if (ret == G_IO_STATUS_ERROR) {
            g_warning("Error closing: %s", error->message);
            g_error_free(error);
        }
        return false;
    }

    /* simulate the typed flag to allow to record the commands in history */
    vb.state.typed = true;

    /* run the commands */
    map_handle_string(line, true);
    g_free(line);

    /* unset typed flag */
    vb.state.typed = false;

    /* We assume that the commands result is still available in the inputbox,
     * so the whole inputbox content is written to the socket. */
    inputtext = vb_get_input_text();
    ret       = g_io_channel_write_chars(chan, inputtext, -1, &len, &error);
    if (ret == G_IO_STATUS_ERROR) {
        g_warning("Error writing: %s", error->message);
        g_error_free(error);
    }
    if (g_io_channel_flush(chan, &error) == G_IO_STATUS_ERROR) {
        g_warning("Error flushing: %s", error->message);
        g_error_free(error);
    }

    g_free(inputtext);

    return true;
}

#endif
