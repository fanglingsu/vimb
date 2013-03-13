/**
 * vimp - a webkit based vim like browser.
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

#ifndef _CONFIG_H
#define _CONFIG_H

#include "stdlib.h"

/* time in seconds after that message will be removed from inputbox if the
 * message where only temporary */
#define MESSAGE_TIMEOUT             5
#define SETTING_MAX_CONNS           25
#define SETTING_MAX_CONNS_PER_HOST  5

const unsigned int MAXIMUM_HINTS = 500;

const unsigned int COMMAND_HISTORY_SIZE = 30;

const struct {
    char* command;
} default_config[] = {
    {"nmap gf=source"},
    {"nmap gF=inspect"},
    {"nmap <shift-:>=input"},
    {"nmap <shift-/>=input /"},
    {"nmap <shift-?>=input ?"},
    {"smap n=search-forward"},
    {"smap N=search-backward"},
    {"nmap o=input :open "},
    {"nmap t=input :tabopen "},
    {"nmap O=inputuri :open "},
    {"nmap T=inputuri :tabopen "},
    {"nmap gh=open"},
    {"nmap gH=tabopen"},
    {"nmap u=open-closed"},
    {"nmap U=tabopen-closed"},
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
    {"nmap ;i=hint-image-open"},
    {"nmap ;I=hint-image-tabopen"},
    {"nmap y=yank-uri"},
    {"nmap Y=yank-selection"},
    {"nmap p=open-clipboard"},
    {"nmap P=tabopen-clipboard"},
    {"nmap zi=zoomin"},
    {"nmap zI=zoominfull"},
    {"nmap zo=zoomout"},
    {"nmap zO=zoomoutfull"},
    {"nmap zz=zoomreset"},
    {"cmap <tab>=complete"},
    {"cmap <shift-tab>=complete-back"},
    {"cmap <ctrl-p>=command-hist-prev"},
    {"cmap <ctrl-n>=command-hist-next"},
    {"hmap <tab>=hint-focus-next"},
    {"hmap <shift-tab>=hint-focus-prev"},
    {"searchengine-add dl=https://duckduckgo.com/lite/?q=%s"},
    {"searchengine-add dd=https://duckduckgo.com/?q=%s"},
    {NULL}
};

#endif /* end of include guard: _CONFIG_H */
