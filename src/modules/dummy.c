//
// Created by jantonio on 21/02/07
// Copyright 2019 by Juan Antonio Martínez Castaño <info@agilitycontest.es>

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
// and associated documentation files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies
// or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "modules.h"
#include "sc_config.h"
#include "debug.h"


static char error_str[1024];

static configuration *config;
/* Declare our Add function using the above definitions. */

int ADDCALL module_init(configuration *cfg){
    config=cfg;
    return 0;
}

int ADDCALL module_end() {
    return 0;
}

int ADDCALL module_open(){
    return 0;
}

int ADDCALL module_close(){
    return 0;
}

int ADDCALL module_read(char *buffer,size_t length){
    snprintf(buffer,length,"");
    debug(DBG_TRACE,"module_write(), return 0 bytes (dummy) read");
    return strlen(buffer);
}

int ADDCALL module_write(char **tokens,size_t ntokens){
    static char *buffer=NULL;
    int len=0;
    if (buffer==NULL) buffer=calloc(1024,sizeof(char));
    // compose message by adding tokens
    // int len=sprintf(buffer,"%s",tokens[1]);
    for (len=0;len<strlen(tokens[1]);len++) buffer[len]=toupper(tokens[1][len]); // uppercase command (as manual says)
    for (int n=2;n<ntokens;n++) len += sprintf(buffer+len," %s",tokens[n]);
    len += sprintf(buffer+len,"\r\n");
    // notice "blocking" mode. needed as most modules do not support full duplex communications
    debug(DBG_TRACE,"module_write(), dummy send %d bytes: '%s'",len,buffer);
    return len;
}

char * ADDCALL module_error() {
    return error_str;
}