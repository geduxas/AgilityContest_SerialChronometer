//
// Created by jantonio on 5/05/19.
//

#define AGILITYCONTEST_SERIALCHRONOMETER_AJAX_MGR_C
#include <stdio.h>
#include <unistd.h>
#include <curl/curl.h>
#include <errno.h>

#include "main.h"
#include "debug.h"
#include "sc_tools.h"
#include "ajax_mgr.h"
#include "ajax_curl.h"
#include "ajax_json.h"
#include "sc_config.h"
#include "sc_sockets.h"

/*
# Request to server are made by sending json request to:
#
# http://ip.addr.of.server/base_url/ajax/database/eventFunctions.php
#
# Parameter list
# Operation=chronoEvent
# Type= one of : ( from ajax/database/Eventos.php )
#               'crono_start'   // Arranque Crono electronico
#               'crono_int'     // Tiempo intermedio Crono electronico
#               'crono_stop'    // Parada Crono electronico
#               'crono_rec'     // comienzo/fin del reconocimiento de pista
#               'crono_dat'     // Envio de Falta/Rehuse/Eliminado desde el crono (need extra data. see below)
#               'crono_reset'   // puesta a cero del contador
#               'crono_error'   // sensor error detected (Value=1) or solved (Value=0)
#               'crono_ready'   // chrono synced and listening (Value=1) or disabled (Value=0)
# Session= Session ID to join. You should select it from retrieved list of available session ID's from server
# Source= Chronometer ID. should be in form "chrono_sessid"
# Value= start/stop/int: time of event detection
#                 error: 1: detected 0:solved
# Timestamp= time mark of last event parsed as received from server
# example:
# ?Operation=chronoEvent&Type=crono_rec&TimeStamp=150936&Source=chrono_2&Session=2&Value=150936
*/

// NOTE:
// a ¿bug or javascript feature? in AgilityContest, makes zero as invalid value for "start" command
// as SerialAPI states zero as default, we need a dirty trick: add "1" to every sent start/int/and stop values
// no problem as back event will be rejected due to being originated here

static int ajax_mgr_start(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_AJAXSRV,tokens[0])==0) return 0; // to avoid get/put loop
    char buffer[32];
    snprintf(buffer,32,"%lu",1+config->status.start_time);
    sc_extra_data_t data= { "",buffer,0,0,0 }; /* oper, value, start, stop, tiempo */
    int res=ajax_put_event(config,"crono_start",&data,0);
    return res;
}
static int ajax_mgr_int(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_AJAXSRV,tokens[0])==0) return 0; // to avoid get/put loop
    char buffer[32];
    snprintf(buffer,32,"%lu",1+config->status.int_time);
    sc_extra_data_t data= { "",buffer,0,0,0 }; /* oper, value, start, stop, tiempo */
    int res=ajax_put_event(config,"crono_int",&data,0);
    return res;
}
static int ajax_mgr_stop(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_AJAXSRV,tokens[0])==0) return 0; // to avoid get/put loop
    char buffer[32];
    snprintf(buffer,32,"%lu",1+config->status.stop_time);
    sc_extra_data_t data= { "",buffer,0,0,0 }; /* oper, value, start, stop, tiempo */
    int res=ajax_put_event(config,"crono_stop",&data,0);
    return res;
}
static int ajax_mgr_fail(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_AJAXSRV,tokens[0])==0) return 0; // to avoid get/put loop
    sc_extra_data_t data= { "","1",0,0,0 }; /* oper, value, start, stop, tiempo */
    int res=ajax_put_event(config,"crono_error",&data,0);
    return res;
}
static int ajax_mgr_ok(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_AJAXSRV,tokens[0])==0) return 0; // to avoid get/put loop
    sc_extra_data_t data= { "","0",0,0,0 }; /* oper, value, start, stop, tiempo */
    int res=ajax_put_event(config,"crono_error",&data,0);
    return res;
}
static int ajax_mgr_msg(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_AJAXSRV,tokens[0])==0) return 0; // to avoid get/put loop
    // compose message
    char buff[512];
    int len=sprintf(buff,"%s:%s",tokens[2],tokens[3]);
    for (int n=4;n<ntokens;n++) len+=sprintf(buff+len,"%%20%s",tokens[n]);
    // send command EVTCMD_MESSAGE (8)
    sc_extra_data_t data= { "8",buff,0,0,0 }; /* oper, value, start, stop, tiempo */
    int res=ajax_put_event(config,"command",&data,0);
    return res;
}
static int ajax_mgr_walk(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_AJAXSRV,tokens[0])==0) return 0; // to avoid get/put loop
    // defaults to 420 seconds (7minutes)
    sc_extra_data_t data= { "","",420,0,0 }; /* oper, value, start, stop, tiempo */
    if (ntokens==3) data.start=atol(tokens[2]); // else use provided value for countdown
    int res=ajax_put_event(config,"crono_rec",&data,0);
    return res;
}
static int ajax_mgr_down(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_AJAXSRV,tokens[0])==0) return 0; // to avoid get/put loop
    // defaults to 15 seconds
    sc_extra_data_t data= { "","15",0,0,0 }; /* oper, value, start, stop, tiempo */
    if (ntokens==3) data.value=tokens[2]; // else use provided value for countdown
    int res=ajax_put_event(config,"salida",&data,1);
    return res;
}
// faults/refusals/eliminated and so are already prcessed in main loop, so just send
// stored values. No extra data required
static int ajax_mgr_data(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_AJAXSRV,tokens[0])==0) return 0; // to avoid get/put loop
    int res=ajax_put_event(config,"crono_dat",NULL,1);
    return res;
}
static int ajax_mgr_reset(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_AJAXSRV,tokens[0])==0) return 0; // to avoid get/put loop
    int res=ajax_put_event(config,"crono_reset",NULL,0);
    return res;
}
static int ajax_mgr_exit(configuration * config, int slot, char **tokens, int ntokens) {
    debug(DBG_INFO,"Ajax Manager Thread exit requested");
    return -1;
}

static func entries[32]= {
        ajax_mgr_start,  // { 0, "start",   "Start of course run",             "[miliseconds] {0}"},
        ajax_mgr_int,    // { 1, "int",     "Intermediate time mark",          "<miliseconds>"},
        ajax_mgr_stop,   // { 2, "stop",    "End of course run",               "<miliseconds>"},
        ajax_mgr_fail,   // { 3, "fail",    "Sensor faillure detected",        ""},
        ajax_mgr_ok,     // { 4, "ok",      "Sensor recovery. Chrono ready",   ""},
        ajax_mgr_msg,    // { 5, "msg",     "Show message on chrono display",  "<message> [seconds] {2}"},
        ajax_mgr_walk,   // { 6, "walk",    "Course walk (0:stop)",            "<seconds> {420}"},
        ajax_mgr_down,   // { 7, "down",    "Start 15 seconds countdown",      ""},
        ajax_mgr_data,   // { 8, "fault",   "Mark fault (+/-/#)",              "< + | - | num >"},
        ajax_mgr_data,   // { 9, "refusal", "Mark refusal (+/-/#)",            "< + | - | num >"},
        ajax_mgr_data,   // { 10, "elim",    "Mark elimination [+-]",           "[ + | - ] {+}"},
        ajax_mgr_data,   // { 11, "data",   "Set faults/refusal/disq info",    "<flt>:<reh>:<disq>"},
        ajax_mgr_reset,  // { 12, "reset",  "Reset chronometer and countdown", "" },
        NULL,            // { 13, "help",   "show command list",               "[cmd]"},
        NULL,            // { 14, "version","Show software version",           "" },
        ajax_mgr_exit,   // { 15, "exit",   "End program (from console)",      "" },
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

void *ajax_manager_thread(void *arg){
    int slotIndex= * ((int *)arg);
    sc_thread_slot *slot=&sc_threads[slotIndex];
#ifdef __APPLE__
    pthread_setname_np(slot->tname);
#endif
    configuration *config=slot->config;
    slot->entries = entries;

    // create sock for local communicacion
    char portstr[16];
    snprintf(portstr,16,"%d",config->local_port+config->ring);
    slot->sock=connectUDP("localhost",portstr);
    if (slot->sock <0) {
        debug(DBG_ERROR,"%s: Cannot create local socket",SC_AJAXSRV);
        return NULL;
    }

    // if ajax server is "find", or "0.0.0.0" try to locate before entering loop
    if (strcasecmp(config->ajax_server,"find")==0 || strcasecmp(config->ajax_server,"0.0.0.0")==0) {
        debug(DBG_INFO,"%s: Trying to locate AgilityContest server IP address",SC_AJAXSRV);
    }

    // retrieve session ID
    int ses=ajax_connect_server(config);
    if (ses<0) {
        debug(DBG_ERROR,"%s Cannot retrieve session id for ring %d on server %s",SC_AJAXSRV,config->ring,config->ajax_server);
        return NULL;
    }
    config->status.sessionID=ses;

    debug(DBG_TRACE,"AgilityContest link thread initialized. Entering loop connecting to '%s'",config->ajax_server);
    // mark thread alive before start working
    slot->index=slotIndex;

    // wait until init event id is received
    int evtid=0;
    while ( (slot->index>=0) && (evtid==0)) {
        // call ajax connect session
        evtid=ajax_open_session(config);
        if (evtid<0) {
            debug(DBG_ERROR,"%s: Cannot open session %d at server '%s'",SC_AJAXSRV,ses,config->ajax_server);
            return NULL;
        }
        if (evtid==0) sleep(2); // retry in 5 seconds
    }

    // start waitForEvents loop until exit or error
    time_t timestamp=0;

    // create input/output buffer
    char *request=calloc(1024,sizeof(char));
    char *response=calloc(1024,sizeof(char));
    if (!request || !response) {
        debug(DBG_ERROR, "%s: Cannot enter main loop:calloc()",SC_AJAXSRV);
        return NULL;
    }

    // loop until end requested
    while(slot->index>=0) {
        int res=0;
        char **cmds=ajax_wait_for_events(config,&evtid,&timestamp);
        if (!cmds) {
            // debug(DBG_NOTICE,"%s WaitForEvent() failed. retrying",SC_AJAXSRV);
            sleep(1);
            continue;
        }
        // iterate received command list
        for (char **n=cmds;*n;n++) {
            char *p=NULL;
            snprintf(request,1024,"%s %s",SC_AJAXSRV,*n);
            if ((p=strchr(request, '\n')) != NULL) *p='\0'; //strip newline if any

            // send via local udp socket received
            if (strlen(*n)==0) continue; // empty string received
            debug(DBG_TRACE,"%s sending to local socket: '%s'",SC_AJAXSRV,request);
            res=send(slot->sock,request,strlen(request),0);
            if (res<0){
                debug(DBG_ERROR,"%s send(): error sending request: %s",SC_AJAXSRV,strerror(errno));
                continue;
            }
            // retrieve response from main server
            res=recv(slot->sock,response,1024,0);
            if (res<0) {
                debug(DBG_ERROR,"%s recv(): error waiting response: %s",SC_AJAXSRV,strerror(errno));
                continue;
            } else {
                response[res]='\0'; // put eol at end of recvd string
                debug(DBG_NOTICE,"%s main loop command response: %s\n",SC_AJAXSRV,response);
            }
            // free message
            free(*n);
        }
        // free command list
        free(cmds);
    }
    // mark thread inactive and exit
    free(request);
    free(response);
    slot->index=-1;
    debug(DBG_TRACE,"Exiting %s thread",SC_AJAXSRV);
    return &slot->index;
}
