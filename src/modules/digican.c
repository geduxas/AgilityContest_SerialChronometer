//
// Created by jantonio on 20/06/19.
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

#include "modules.h"
#include "sc_config.h"
#include "debug.h"

static char error_str[1024];

static configuration *config;
/* Declare our Add function using the above definitions. */
int ADDCALL module_init(configuration *cfg){
    config=cfg;
    enum sp_return ret=sp_get_port_by_name(config->comm_port,&config->serial_port);
    if (ret!= SP_OK) {
        snprintf(error_str,strlen(error_str),"Cannot locate serial port '%s'",config->comm_port);
        debug(DBG_ERROR,error_str);
        return -1;
    }
    return 0;
}
int ADDCALL module_end() {
    if (config->serial_port) {
        sp_free_port(config->serial_port);
    }
    config->serial_port=NULL;
    return 0;
}

int ADDCALL module_open(){
    if (config->serial_port) {
        enum sp_return ret = sp_open(config->serial_port,SP_MODE_READ_WRITE);
        if (ret != SP_OK) {
            snprintf(error_str,strlen(error_str),"Cannot open serial port %s",config->comm_port);
            debug(DBG_ERROR,error_str);
            return -1;
        }
    }
    sp_set_baudrate(config->serial_port, config->baud_rate);
    sp_set_rts(config->serial_port,SP_RTS_ON);
    return 0;
}

int ADDCALL module_close(){
    if (config->serial_port) {
        sp_close(config->serial_port);
    }
    return 0;
}

int ADDCALL module_read(char *buffer,size_t length){
    enum sp_return ret;
    static char *inbuff=NULL;
    if (inbuff==NULL) inbuff=malloc(1024*sizeof(char));
    memset(inbuff,0,1024*sizeof(char));

    do {
        ret = sp_blocking_read(config->serial_port,inbuff,1024,500); // timeout 0.5 seconds
        if (ret>=0)inbuff[ret]='\0';
    } while(ret==0);
    if (ret <0 ) {
        debug(DBG_ERROR,"libserial_read() error %s",sp_last_error_message());
        snprintf(buffer,length,"");
        return strlen(buffer);
    };
    debug(DBG_TRACE,"module_read() received '%s'",inbuff);
    // At this moment, Digican only sends 2 comands "START" and "PARAR". So just a simple parser
    if (strncmp(inbuff,"START",5)==0) {
        snprintf(buffer,length,"start 0\n");
    }
    else if (strncmp(inbuff,"PARAR",5)==0) {
        int elapsed= 10* atoi(inbuff+7);
        snprintf(buffer,length,"stop %d\n",elapsed);
    }
    // PENDING: parse and handle additional comands sent from digican chrono ( ask oitoinnova )
    else snprintf(buffer,length,"");
    debug(DBG_TRACE,"module_read() sending to serial manager '%s'",buffer);
    return strlen(buffer);
}

static int digican_faltas=0;
static int digican_rehuses=0;

int ADDCALL module_write(char **tokens, size_t ntokens){
    static char *buffer=NULL;
    int len=0;
    if (buffer==NULL) buffer=malloc(1024*sizeof(char));
    memset(buffer,0,1024);
    char *cmd=tokens[1];

    // { 0, "start",   "Start of course run",             "[miliseconds] {0}"},
    if (strcasecmp("start",cmd)==0) {
        sprintf(buffer,"START$");
    }
    // { 1, "int",     "Intermediate time mark",          "<miliseconds>"},
    else if (strcasecmp("int",cmd)==0) {
        // on intermediate time cannot provide timestamp to digican, so just call to show intermediate time
        // this may lead in some precission errors due to communications lag
        sprintf(buffer,"TEMPI$");
    }
    // { 2, "stop",    "End of course run",               "<miliseconds>"},
    else if (strcasecmp("stop",cmd)==0) {
        // digican uses cents of seconds, so remove last digit
        long cents=(config->status.stop_time - config->status.start_time)/10;
        sprintf(buffer,"PARAR%04d$",cents);
    }
    // { 3, "fail",    "Sensor faillure detected",        ""},
    // unsupported in digican chrono
    // { 4, "ok",      "Sensor recovery. Chrono ready",   ""},
    // unsupported in digican chrono
    // { 5, "msg",     "Show message on chrono display",  "<message> [seconds] {2}"},
    // unsupported in digican chrono
    // { 6, "walk",    "Course walk (0:stop)",            "<seconds> {420}"},
    else if (strcasecmp("walk",cmd)==0) {
        // PENDING: ask digican how to provide course walk duration in seconds ( or minutes )
        sprintf(buffer,"RECON$");
    }
    // { 7, "down",    "Start 15 seconds countdown",      ""},
    else if (strcasecmp("walk",cmd)==0) {
        // PENDING: Ask digican how to provide countdown duration instead of 15 seconds
        sprintf(buffer,"INICI$");
    }
    /*
     * In digican chrono we cannot provide absolute values for faults and refusals, so need
     * to track an inner count and sincronize it with master data
     * so the way to work is:
     * On reset,reloj and so clear variables
     */
    // { 8, "fault",   "Mark fault (+/-/#)",              "< + | - | num {+}>"},
    else if (strcasecmp("fault",cmd)==0) {
        int len=0;
        int ft=config->status.faults+config->status.touchs;
        if (digican_faltas < config->status.faults) {
            for (;digican_faltas < ft ; digican_faltas++) len+=sprintf(buffer+len,"FALT+$");
        } else {
            for (;digican_faltas>ft;digican_faltas--) len+=sprintf(buffer+len,"FALT-$");
        }
    }
    // { 9, "refusal", "Mark refusal (+/-/#)",            "< + | - | num {+}>"},
    else if (strcasecmp("refusal",cmd)==0) {
        int len=0;
        if (digican_rehuses < config->status.refusals) {
            for (;digican_rehuses < config->status.refusals; digican_rehuses++) len+=sprintf(buffer+len,"REHU+$");
        } else {
            for (;digican_rehuses > config->status.refusals; digican_rehuses--) len+=sprintf(buffer+len,"REHU-$");
        }
    }
    // { 10, "elim",    "Mark elimination [+-]",          "[ + | - ] {+}"},
    else if (strcasecmp("elim",cmd)==0) {
        // digican has just "fire up" eliminated. no way to remove mark :-(
        sprintf(buffer,"ELIMI$");
    }
    // { 11, "data",    "Set course fault/ref/disq info", "<faults>:<refulsals>:<disq>"},
    else if (strcasecmp("data",cmd)==0) {
        int len=0;
        int ft=config->status.faults+config->status.touchs;
        if (digican_faltas < ft) {
            for (;digican_faltas <ft;digican_faltas++) len+=sprintf(buffer+len,"FALT+$");
        } else {
            for (;ft;digican_faltas--) len+=sprintf(buffer+len,"FALT-$");
        }
        if (digican_rehuses < config->status.refusals) {
            for (;digican_rehuses < config->status.refusals; digican_rehuses++) len+=sprintf(buffer+len,"REHU+$");
        } else {
            for (;digican_rehuses > config->status.refusals; digican_rehuses--) len+=sprintf(buffer+len,"REHU-$");
        }
        if (config->status.eliminated!=0) len+=sprintf(buffer+len,"ELIMI$");
    }
    // { 12, "reset",  "Reset chronometer and countdown", "" },
    else if (strcasecmp("reset",cmd)==0) {
        digican_faltas=0;
        digican_rehuses=0;
        sprintf(buffer,"RESET$");
    }
    // { 13, "help",   "show command list",               "[cmd]"},
    //  useless outside console
    // { 14, "version","Show software version",           "" },
    //  useless outside console
    // { 15, "exit",   "End program (from console)",      "" },
    // useless outside console
    // { 16, "server", "Set server IP address",           "<x.y.z.t> {0.0.0.0}" },
    //  useless outside console
    // { 17, "ports",  "Show available serial ports",     "" },
    //  useless outside console
    // { 18, "config", "List configuration parameters",   "" },
    //  useless outside console
    // { 19, "status", "Show Fault/Refusal/Elim state",   "" },
    //  useless outside console
    // { 20, "turn",   "Set current dog order number [+-#]", "[ + | - | num ] {+}"},
    //  useless outside console
    // { 21, "clock",  "Enter clock mode",                "[ hh:mm:ss ] {current time}"},
    // PENDING: ask oito-innova how to specify current time
    else if (strcasecmp("clock",cmd)==0) {
        // PENDING: check if setting hh/mm is available in digican crono
        digican_faltas=0;
        digican_rehuses=0;
        sprintf(buffer,"RELOJ$");
    }
    // { 22, "debug",  "Get/Set debug level",             "[ new_level ]"},
    // only for console
    // { -1, NULL,     "",                                "" }
    else {
        // arriving here means unrecognized or not supported command.
        debug(DBG_NOTICE,"Unrecognized or Unsupported command '%s'",tokens[1]);
        return 0;
    }

    // so that's allmost all done, just sending command to serial port....
    // notice "blocking" mode. needed as digican does not support full duplex communications
    debug(DBG_TRACE,"module_write(), send: '%s'",buffer);
    enum sp_return ret=sp_blocking_write(config->serial_port,buffer,strlen(buffer),0);
    return ret;
}

char * ADDCALL module_error() {
    return error_str;
}