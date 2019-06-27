//
// Created by jantonio on 5/05/19.
//

#define AGILITYCONTEST_SERIALCHRONOMETER_AJAX_MGR_C
#include <stdio.h>
#include <unistd.h>
#include <curl/curl.h>
#include <errno.h>

#include "../include/main.h"
#include "../include/debug.h"
#include "../include/ajax_mgr.h"
#include "../include/ajax_curl.h"
#include "../include/ajax_json.h"
#include "../include/sc_config.h"
#include "../include/sc_sockets.h"

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
static int ajax_mgr_start(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_int(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_stop(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_fail(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_ok(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_msg(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_walk(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_down(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_fault(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_refusal(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_elim(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_reset(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_exit(configuration * config, int slot, char **tokens, int ntokens) {
    debug(DBG_INFO,"Ajax Manager Thread exit requested");
    return -1;
}
static int ajax_mgr_server(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_dorsal(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
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
        ajax_mgr_fault,  // { 8, "fault",   "Mark fault (+/-/#)",              "< + | - | num >"},
        ajax_mgr_refusal,// { 9, "refusal", "Mark refusal (+/-/#)",            "< + | - | num >"},
        ajax_mgr_elim,   // { 10, "elim",    "Mark elimination [+-]",           "[ + | - ] {+}"},
        ajax_mgr_reset,  // { 11, "reset",  "Reset chronometer and countdown", "" },
        NULL,            // { 12, "help",   "show command list",               "[cmd]"},
        NULL,            // { 13, "version","Show software version",           "" },
        ajax_mgr_exit,   // { 14, "exit",   "End program (from console)",      "" },
        ajax_mgr_server, // { 15, "server", "Set server IP address",           "<x.y.z.t> {0.0.0.0}" },
        NULL,            // { 16, "ports",  "Show available serial ports",     "" },
        NULL,            // { 17, "config", "List configuration parameters",   "" },
        NULL,            // { 18, "status", "Show Fault/Refusal/Elim state",   "" },
        ajax_mgr_dorsal, // { 19, "turn",   "Set current dog order number [+-#]", "[ + | - | num ] {+}"},
        NULL,            // { 20, "clock",  "Enter clock mode",                "[ hh:mm:ss ] {current time}"},
        NULL,            // { 21, "debug",  "Get/Set debug level",             "[ new_level ]"},
        NULL             // { -1, NULL,     "",                                "" }
};

void *ajax_manager_thread(void *arg){
    int sessionID=0;
    int slotIndex= * ((int *)arg);
    sc_thread_slot *slot=&sc_threads[slotIndex];
    configuration *config=slot->config;
    slot->entries = entries;

    // create sock for local communicacion
    char portstr[16];
    snprintf(portstr,16,"%d",config->local_port+config->ring);
    slot->sock=connectUDP("localhost",portstr);
    if (slot->sock <0) {
        debug(DBG_ERROR,"AjaxMgr: Cannot create local socket");
        return NULL;
    }

    // if ajax server is "find", or "0.0.0.0" try to locate before entering loop
    if (strcasecmp(config->ajax_server,"find")==0 || strcasecmp(config->ajax_server,"0.0.0.0")==0) {
        debug(DBG_INFO,"Trying to locate AgilityContest server IP address");
    }

    // retrieve session ID
    sessionID=ajax_connect_server(config);
    if (sessionID<0) {
        debug(DBG_ERROR,"Cannot retrieve session id for ring %d on server %s",config->ring,config->ajax_server);
        return NULL;
    }

    // mark thread alive before start working
    slot->index=slotIndex;

    // wait until init event id is received
    int evtid=0;
    while ( (slot->index>=0) && (evtid==0)) {
        // call ajax connect session
        evtid=ajax_open_session(config,sessionID);
        if (evtid<0) {
            debug(DBG_ERROR,"Cannot open session %d at server '%s'",config->ajax_server);
            return NULL;
        }
        if (evtid==0) sleep(5); // retry in 5 seconds
    }

    // start waitForEvents loop until exit or error
    time_t timestamp=0;

    // create input/output buffer
    char *request=calloc(1024,sizeof(char));
    char *response=calloc(1024,sizeof(char));
    if (!request || !response) {
        debug(DBG_ERROR, "ajax manager: Cannot enter main loop:calloc()");
        return NULL;
    }

    // loop until end requested
    while(slot->index>=0) {
        int res=0;
        char **cmds=ajax_wait_for_events(config,sessionID,&evtid,&timestamp);
        if (!cmds) {
            debug(DBG_NOTICE,"WaitForEvent failed. retrying");
            sleep(5);
            continue;
        }
        // iterate received command list
        for (char **n=cmds;*n;n++) {
            char *p=NULL;
            snprintf(request,1024,"%s %s",SC_AJAXSRV,*n);
            if ((p=strchr(request, '\n')) != NULL) *p='\0'; //strip newline if any

            // send via local udp socket received
            if (strlen(*n)==0) continue; // empty string received
            debug(DBG_TRACE,"Console: sending to local socket: '%s'",request);
            res=send(slot->sock,request,strlen(request),0);
            if (res<0){
                debug(DBG_ERROR,"Console send(): error sending request: %s",strerror(errno));
                continue;
            }
            // retrieve response from main server
            res=recv(slot->sock,response,1024,0);
            if (res<0) {
                debug(DBG_ERROR,"Console recv(): error waiting response: %s",strerror(errno));
                continue;
            } else {
                response[res]='\0'; // put eol at end of recvd string
                fprintf(stdout,"Console command response: %s\n",response);
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
    debug(DBG_TRACE,"Exiting ajax thread");
    return &slot->index;
}
