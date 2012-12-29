/**
 * vimp - a webkit based vim like browser.
 *
 * Copyright (C) 2012 Daniel Carl
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

#ifndef CONFIG_H
#define CONFIG_H

#include "stdlib.h"

#define START_PAGE      "https://github.com/fanglingsu/vimp"

#define SETTING_DEFAUL_FONT_SIZE    12
#define SETTING_USER_AGENT          PROJECT "/" VERSION " (X11; Linux i686) AppleWebKit/535.22+ Compatible (Safari)"
#define SETTING_MAX_CONNS           25
#define SETTING_MAX_CONNS_PER_HOST  5

const struct {
    char* command;
} default_config[] = {
    {"nmap gf=source"},
    {"nmap <shift-:>=input"},
    {"nmap o=inputopen"},
    {"nmap t=inputtabopen"},
    {"nmap O=inputopencurrent"},
    {"nmap T=inputtabopencurrent"},
    {"nmap d=quit"},
    {"nmap <ctrl-o>=back"},
    {"nmap <ctrl-i>=forward"},
    {"nmap r=reload"},
    {"nmap R=reload!"},
    {"nmap <ctrl-c>=stop"},
    {"nmap <ctrl-f>=pagedown"},
    {"nmap <ctrl-b>=pageup"},
    {"nmap <ctrl-d>=halfpagedown"},
    {"nmap <ctrl-u>=halfpageup"},
    {"nmap gg=jumptop"},
    {"nmap G=jumpbottom"},
    {"nmap 0=jumpleft"},
    {"nmap $=jumpright"},
    {"nmap h=scrollleft"},
    {"nmap l=scrollright"},
    {"nmap k=scrollup"},
    {"nmap j=scrolldown"},
    {"nmap f=hint-link"},
    {"nmap F=hint-link-new"},
    {"nmap ;o=hint-input-open"},
    {"nmap ;t=hint-input-tabopen"},
    {"nmap ;y=hint-yank"},
    {"nmap y=yank-uri"},
    {"nmap Y=yank-selection"},
    {"cmap <tab>=complete"},
    {"cmap <shift-tab>=complete-back"},
    {"hmap <tab>=hint-focus-next"},
    {"hmap <shift-tab>=hint-focus-prev"},
    {NULL}
};

#endif /* end of include guard: CONFIG_H */
