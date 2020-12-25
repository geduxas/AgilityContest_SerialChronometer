//
// Created by jantonio on 5/05/19.
//

#define AGILITYCONTEST_SERIALCHRONOMETER_QRCODE_MGR_C
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#include "main.h"
#include "debug.h"
#include "sc_tools.h"
#include "qrcode_mgr.h"
#include "sc_config.h"
#include "sc_sockets.h"
#include "libserialport.h"

static int qrcode_mgr_exit(configuration * config, int slot, char **tokens, int ntokens) {
    debug(DBG_INFO,"QRCode Manager Thread exit requested");
    if (config->serial_port) {
        sp_close(config->serial_port);
        config->serial_port=NULL;
    }
    return -1;
}

/*
 * extract "dor":number or drs":number or "dorsal":number from json received QRCode request
 */
static int qrcode_parse_dorsal(char *request) {
    char *p=request;
    int offset=0;
    for(;*p;p++) *p=tolower(*p);
    p=strstr(request,"dor\":"); offset=5;
    if (!p) {p=strstr(request,"drs\":");offset=5;}
    if (!p) {p=strstr(request,"dorsal\":");offset=8;}
    if (!p) return -1;
    return atoi(p+offset);
}

static func entries[32]= {
        NULL,           // { 0, "start",   "Start of course run",             "[miliseconds] {0}"},
        NULL,           // { 1, "int",     "Intermediate time mark",          "<miliseconds>"},
        NULL,           // { 2, "stop",    "End of course run",               "<miliseconds>"},
        NULL,           // { 3, "fail",    "Sensor faillure detected",        ""},
        NULL,           // { 4, "ok",      "Sensor recovery. Chrono ready",   ""},
        NULL,           // { 5, "msg",     "Show message on chrono display",  "<message> [seconds] {2}"},
        NULL,           // { 6, "walk",    "Course walk (0:stop)",            "<seconds> {420}"},
        NULL,           // { 7, "down",    "Start 15 seconds countdown",      ""},
        NULL,           // { 8, "fault",   "Mark fault (+/-/#)",              "< + | - | num >"},
        NULL,           // { 9, "refusal", "Mark refusal (+/-/#)",            "< + | - | num >"},
        NULL,           // { 10, "elim",    "Mark elimination [+-]",           "[ + | - ] {+}"},
        NULL,           // { 11, "data",   "Set faults/refusal/disq info",    "<flt>:<reh>:<disq>"},
        NULL,           // { 12, "reset",  "Reset chronometer and countdown", "" },
        NULL,            // { 13, "help",   "show command list",               "[cmd]"},
        NULL,            // { 14, "version","Show software version",           "" },
        qrcode_mgr_exit,   // { 15, "exit",   "End program (from console)",      "" },
        NULL,            // { 16, "server", "Set server IP address",           "<x.y.z.t> {0.0.0.0}" },
        NULL,            // { 17, "ports",  "Show available serial ports",     "" },
        NULL,            // { 18, "config", "List configuration parameters",   "" },
        NULL,            // { 19, "status", "Show Fault/Refusal/Elim state",   "" },
        NULL,            // { 20, "turn",   "Set current dog order number [+-#]", "[ + | - | num ] {+}"},
        NULL,            // { 21, "bright", "Set display bright level [+-#]",  "[ + | - | num ] {+}"},
        NULL,            // { 22, "clock",  "Enter clock mode",                "[ hh:mm:ss ] {current time}"},
        NULL,            // { 23, "debug",  "Get/Set debug level",             "[ new_level ]"},
        NULL,            // { 24, "dorsal",  "get dorsal from qrcode reader",  "[ dorsal # ]"},
        NULL             // { -1, NULL,     "",                                "" }
};

void *qrcode_manager_thread(void *arg){
    int slotIndex= * ((int *)arg);
    sc_thread_slot *slot=&sc_threads[slotIndex];
#ifdef __APPLE__
    pthread_setname_np(slot->tname);
#endif
    configuration *config=slot->config;
    slot->entries = entries;

    // create sock to communicate with main() loop
    char portstr[16];
    snprintf(portstr,16,"%d",config->local_port+config->ring);
    slot->sock=connectUDP("localhost",portstr);
    if (slot->sock <0) {
        debug(DBG_ERROR,"QRCode: Cannot create local socket");
        return NULL;
    }

    // open serial port. if "none" or fail, just exit thread
    if (!config->qrcomm_port ||  strcasecmp(config->qrcomm_port,"none")==0) {
        debug(DBG_INFO,"QRCode: No QRCode reader set. exit thread");
        return NULL;
    }
    enum sp_return ret=sp_get_port_by_name(config->qrcomm_port,&config->qrcode_port);
    if (ret!= SP_OK) {
        debug(DBG_ERROR,"Cannot locate qrcode serial port '%s'",config->qrcomm_port);
        return NULL;
    }
    if (config->qrcode_port) {
        ret = sp_open(config->qrcode_port,SP_MODE_READ);
        if (ret != SP_OK) {
            debug(DBG_ERROR,"Cannot open qrcode serial port %s",config->qrcomm_port);
            return NULL;
        }
    }
    sp_set_baudrate(config->qrcode_port, 115200); // fixed speed at 115200 bauds
    sp_set_bits(config->qrcode_port, 8);
    sp_set_flowcontrol(config->qrcode_port, SP_FLOWCONTROL_NONE);
    sp_set_parity(config->qrcode_port, SP_PARITY_NONE);
    sp_set_stopbits(config->qrcode_port, 1);
    sp_set_rts(config->qrcode_port,SP_RTS_ON);

    // create input buffer
    char *request=calloc(1024,sizeof(char)); // to receive data from QRCode reader
    char *response=calloc(1024,sizeof(char)); // to get comands from main loop
    if (!request || !response) {
        debug(DBG_ERROR,"QRCode:calloc() Cannot create i/o buffers");
        return NULL;
    }

    // mark thread alive before entering loop
    slot->index=slotIndex;

    // loop until end requested

    int res=0;
    while(res>=0) {
        memset(request,0,1024);
        sprintf(request,"%s ",SC_QRCODE);
        size_t offset=strlen(request);
        do {
            ret = sp_blocking_read(config->serial_port,&request[offset],1024-offset,500); // timeout 0.5 seconds
            if (ret>=0)request[ret]='\0';
        } while( (ret==0) || (slot->index>=0));
        if (ret >0 ) {
            debug(DBG_TRACE,"qrcode_read() received '%s'",request);
            char *p="";
            if ((p=strchr(request, '\n')) != NULL) *p='\0'; //strip newline
            if (strlen(&request[offset])==0) continue; // empty string received
            // extract dorsal number from QRCode info
            int dorsal=qrcode_parse_dorsal(request);
            if (dorsal<1) {
                debug(DBG_ERROR,"qrcode parse_dorsal(): cannot get dorsal from qr code: %s",request);
                continue;
            }
            snprintf(request,1024,"%s dorsal %d",SC_QRCODE,dorsal);
            debug(DBG_TRACE,"qrcode: sending to local socket: '%s'",request);
            res=send(slot->sock,request,strlen(request),0);
            if (res<0){
                debug(DBG_ERROR,"qrcode send(): error sending request: %s",strerror(errno));
                continue;
            }
            res=recv(slot->sock,response,1024,0);
            if (res<0) {
                debug(DBG_ERROR,"qrcode recv(): error waiting response: %s",strerror(errno));
                continue;
            } else {
                response[res] = '\0'; // put eol at end of recvd string
                debug(DBG_NOTICE, "%s main loop command response: %s\n", SC_CONSOLE, response);
            }
        } else {
            debug(DBG_ERROR,"libserial_read() error %s",sp_last_error_message());
            res=-1;
        };
        // check for end requested
        if (slot->index<0) {
            debug(DBG_TRACE,"qrcode: 'exit' command invoked");
            res=-1;
        }
    }
    free(request);
    free(response);
    slot->index=-1;
    qrcode_mgr_exit(config,slotIndex,NULL,0);
    debug(DBG_TRACE,"Exiting qrcode thread");
    return &slot->index;
}
