//
// Created by jantonio on 5/05/19.
//

#define AGILITYCONTEST_SERIALCHRONOMETER_CONSOLE_MGR_C

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "../include/main.h"
#include "../include/sc_tools.h"
#include "../include/sc_sockets.h"
#include "../include/debug.h"
#include "../include/console_mgr.h"
#include "../include/sc_config.h"
#include "../include/parser.h"

/* start [timestamp] */
static int console_mgr_start(configuration * config, int slot, char **tokens, int ntokens) {
    if (ntokens==2)  config->status.timestamp=current_timestamp();
    else config->status.timestamp= strtoull(tokens[2],NULL,10);
    debug(DBG_TRACE,"START: timestamp:%l\n",config->status.timestamp);
    return 0;
}
static int console_mgr_int(configuration * config, int slot, char **tokens, int ntokens) {
    if (config->status.timestamp<0) {
        debug(DBG_WARN,"INT: chrono is not running");
        fprintf(stderr,"Intermediate time: chrono is not running\n");
        return -1;
    }
    long long end= (ntokens==2)?current_timestamp():strtoull(tokens[2],NULL,10);
    float elapsed=(float)(end-config->status.timestamp)/1000.0f;
    debug(DBG_WARN,"INT: elapsed time:%f",elapsed);
    fprintf(stderr,"Intermediate time: %f seconds",elapsed);
    return 0;
}
static int console_mgr_stop(configuration * config, int slot, char **tokens, int ntokens) {
    if (config->status.timestamp<0) {
        debug(DBG_WARN,"STOP: chrono is not running");
        fprintf(stderr,"Course time: chrono is not running\n");
        return -1;
    }
    long long end= (ntokens==2)?current_timestamp():strtoull(tokens[2],NULL,10);
    float elapsed=(float)(end-config->status.timestamp)/1000.0f;
    debug(DBG_TRACE,"STOP: elapsed time:%f",elapsed);
    fprintf(stderr,"Course time: %f seconds",elapsed);
    config->status.timestamp=-1;
    config->status.elapsed= elapsed;
    return 0;
}
static int console_mgr_fail(configuration * config, int slot, char **tokens, int ntokens) {
    debug(DBG_TRACE,"sensor(s) FAIL");
    fprintf(stderr,"Sensor(s) failure noticed");
    return 0;
}
static int console_mgr_ok(configuration * config, int slot, char **tokens, int ntokens) {
    debug(DBG_TRACE,"chrono OK");
    fprintf(stderr,"Sensor(s) OK . Chronometer is ready");
    return 0;
}
/* msg <message> [duration] */
static int console_mgr_msg(configuration * config, int slot, char **tokens, int ntokens) {
    char *msg =(ntokens==3)?tokens[2]:"(empty)";
    int duration = (ntokens==4)?atoi(tokens[3]):3;
    debug(DBG_TRACE,"MSG: %s duration %d",msg,duration);
    fprintf(stderr,"Received message %s",msg);
    return 0;
}
static int console_mgr_walk(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int console_mgr_down(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int console_mgr_fault(configuration * config, int slot, char **tokens, int ntokens) {
    if (ntokens==3) {
        if (strcmp(tokens[2],"+")==0) config->status.faults++;
        else if (strcmp(tokens[2],"-")==0) config->status.faults--;
        else config->status.faults=atoi(tokens[2]);
    } else {
        config->status.faults++;
    }
    if (config->status.faults<0) config->status.faults=0;
    debug(DBG_TRACE,"FAULT: %d",config->status.faults);
    fprintf(stderr,"Fault count is: %d",config->status.faults);
    return 0;
}
static int console_mgr_refusal(configuration * config, int slot, char **tokens, int ntokens) {
    if (ntokens==3) {
        if (strcmp(tokens[2],"+")==0) config->status.refusals++;
        else if (strcmp(tokens[2],"-")==0) config->status.refusals--;
        else config->status.refusals=atoi(tokens[2]);
    } else {
        config->status.refusals++;
    }
    if (config->status.refusals<0) config->status.refusals=0;
    debug(DBG_TRACE,"REFUSAL: %d",config->status.refusals);
    fprintf(stderr,"Refusal count is: %d",config->status.refusals);
    return 0;
}
static int console_mgr_elim(configuration * config, int slot, char **tokens, int ntokens) {
    if (ntokens==3) {
        if (strcmp(tokens[2],"+")==0) config->status.eliminated=1;
        else if (strcmp(tokens[2],"-")==0) config->status.eliminated=0;
        else config->status.eliminated= ( atoi(tokens[2])==0)?0:1;
    } else {
        config->status.eliminated=1;
    }
    debug(DBG_TRACE,"ELIM: %d",config->status.eliminated);
    fprintf(stderr,"Eliminated status is: %d",config->status.eliminated);
    return 0;
}
static int console_mgr_reset(configuration * config, int slot, char **tokens, int ntokens) {
    config->status.timestamp=-1L;
    config->status.faults=0;
    config->status.refusals=0;
    config->status.eliminated=0;
    // DO NOT reset "dorsal"
    debug(DBG_TRACE,"RESET");
    fprintf(stderr,"Received RESET");
    return 0;
}
static int console_mgr_help(configuration * config, int slot, char **tokens, int ntokens) {
    return sc_help(config,ntokens,tokens);
}
static int console_mgr_version(configuration * config, int slot, char **tokens, int ntokens) {
    debug(DBG_INFO,"Console_manager version:%s",CONSOLE_MGR_VERSION);
    fprintf(stdout,"Console_manager version:%s\n",CONSOLE_MGR_VERSION);
    return 0;
}
static int console_mgr_exit(configuration * config, int slot, char **tokens, int ntokens) {
    debug(DBG_INFO,"Console Manager Thread exit requested");
    return 0; // command results ok, not error
}
static int console_mgr_server(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int console_mgr_ports(configuration * config, int slot, char **tokens, int ntokens) {
    return sc_enumerate_ports(config,ntokens,tokens);
}
static int console_mgr_config(configuration * config, int slot, char **tokens, int ntokens) {
    return sc_print_configuration(config,ntokens,tokens);
}

static int console_mgr_status(configuration * config, int slot, char **tokens, int ntokens) {
    return sc_print_status(config,ntokens,tokens);
}

static func entries[32]= {
        console_mgr_start,  // { 0, "start",   "Start of course run",             "[miliseconds] {0}"},
        console_mgr_int,    // { 1, "int",     "Intermediate time mark",          "<miliseconds>"},
        console_mgr_stop,   // { 2, "stop",    "End of course run",               "<miliseconds>"},
        console_mgr_fail,   // { 3, "fail",    "Sensor faillure detected",        ""},
        console_mgr_ok,     // { 4, "ok",      "Sensor recovery. Chrono ready",   ""},
        console_mgr_msg,    // { 5, "msg",     "Show message on chrono display",  "<message> [seconds] {2}"},
        console_mgr_walk,   // { 6, "walk",    "Course walk (0:stop)",            "<seconds> {420}"},
        console_mgr_down,   // { 6, "down",    "Start 15 seconds countdown",      ""},
        console_mgr_fault,  // { 7, "fault",   "Mark fault (+/-/#)",              "< + | - | num >"},
        console_mgr_refusal,// { 8, "refusal", "Mark refusal (+/-/#)",            "< + | - | num >"},
        console_mgr_elim,   // { 9, "elim",    "Mark elimination [+-]",           "[ + | - ] {+}"},
        console_mgr_reset,  // { 10, "reset",  "Reset chronometer and countdown", "" },
        console_mgr_help,   // { 11, "help",   "show command list",               "[cmd]"},
        console_mgr_version,// { 12, "version","Show software version",           "" },
        console_mgr_exit,   // { 13, "exit",   "End program (from console)",      "" },
        console_mgr_server, // { 14, "server", "Set server IP address",           "<x.y.z.t> {0.0.0.0}" },
        console_mgr_ports,  // { 15, "ports",  "Show available serial ports",     "" },
        console_mgr_config, // { 16, "config", "List configuration parameters",   "" },
        console_mgr_status, // { 16, "status", "Show faults/refusal/elim info",   "" },
        NULL                // { -1, NULL,     "",                                "" }
};

void *console_manager_thread(void *arg){

    int slotIndex= * ((int *)arg);
    sc_thread_slot *slot=&sc_threads[slotIndex];
    configuration *config=slot->config;

    slot->entries = entries;

    // create sock
    char portstr[16];
    snprintf(portstr,16,"%d",config->local_port+config->ring);
    slot->sock=connectUDP("localhost",portstr);
    if (slot->sock <0) {
        debug(DBG_ERROR,"Console: Cannot create local socket");
        return NULL;
    }
    // create input buffer
    char *request=calloc(1024,sizeof(char));
    char *response=calloc(1024,sizeof(char));
    if (!request || !response) {
        debug(DBG_ERROR,"Console: Cannot enter interactive mode:calloc()");
        return NULL;
    }

    // mark thread alive before entering loop
    slot->index=slotIndex;
    // loop until end requested
    int res=0;
    sprintf(request,"%s ",SC_CONSOLE);
    int offset=strlen(request);
    while(res>=0) {
        fprintf(stdout,"cmd> ");
        char *p=fgets(&request[offset],1000,stdin);
        if (p) {
            if ((p=strchr(request, '\n')) != NULL) *p='\0'; //strip newline
            debug(DBG_TRACE,"Console: sent to local socket: '%s'",request);
            res=send(slot->sock,request,strlen(request),0);
            if (res<0){
                debug(DBG_ERROR,"Console write(): error sending request: %s",strerror(errno));
                continue;
            }
            res=recv(slot->sock,response,1024,0);
            if (res<0) {
                debug(DBG_ERROR,"Console read(): error waiting response: %s",strerror(errno));
                continue;
            } else {
                response[res]='\0';
                fprintf(stdout,"Command response: %s\n",response);
            }
        } else {
            debug(DBG_TRACE,"Console: received EOF from user input");
            res=-1; // received eof from stdin
        }
        // check for end requested
        if (slot->index<0) {
            debug(DBG_TRACE,"Console: 'exit' command invoked");
            res=-1;
        }
    }
    debug(DBG_TRACE,"Exiting console thread");
    slot->index=-1;
    return &slot->index;
}