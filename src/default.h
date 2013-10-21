/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2013 Daniel Carl
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

#ifndef _DEFAULT_H
#define _DEFAULT_H

#include "stdlib.h"

static char *default_config[] = {
    "shortcut-add dl=https://duckduckgo.com/html/?q=$0",
    "shortcut-add dd=https://duckduckgo.com/?q=$0",
    "shortcut-default dl",
    "set defaultencoding=utf-8",
    "set fontsize=11",
    "set monofontsize=11",
    "set minimumfontsize=5",
    "set useragent=" PROJECT "/" VERSION " (X11; Linux i686) AppleWebKit/535.22+ Compatible (Safari)",
    "set stylesheet=on",
    "set proxy=on",
    "set cookie-timeout=4800",
    "set strict-ssl=on",
    "set strict-focus=off",
    "set scrollstep=40",
    "set status-color-bg=#000",
    "set status-color-fg=#fff",
    "set status-font=monospace bold 8",
    "set status-ssl-color-bg=#95e454",
    "set status-ssl-color-fg=#000",
    "set status-ssl-font=monospace bold 8",
    "set status-sslinvalid-color-bg=#f77",
    "set status-sslinvalid-color-fg=#000",
    "set status-sslinvalid-font=monospace bold 8",
    "set input-bg-normal=#fff",
    "set input-bg-error=#f77",
    "set input-fg-normal=#000",
    "set input-fg-error=#000",
    "set input-font-normal=monospace normal 8",
    "set input-font-error=monospace bold 8",
    "set completion-font=monospace normal 8",
    "set completion-fg-normal=#f6f3e8",
    "set completion-fg-active=#fff",
    "set completion-bg-normal=#656565",
    "set completion-bg-active=#777",
    "set ca-bundle=/etc/ssl/certs/ca-certificates.crt",
    "set home-page=http://fanglingsu.github.io/vimb/",
    "set download-path=",
    "set history-max-items=2000",
    "set editor-command=x-terminal-emulator -e vi %s",
#if WEBKIT_CHECK_VERSION(2, 0, 0)
    "set insecure-content-show=off",
    "set insecure-content-run=off",
#endif
    "set timeoutlen=1000",
    NULL
};

#endif /* end of include guard: _DEFAULT_H */
