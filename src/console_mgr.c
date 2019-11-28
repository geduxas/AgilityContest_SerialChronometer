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
    fprintf(stderr,"Course run START %s\n",(ntokens==2)?"":tokens[2]);
    return 0;
}
static int console_mgr_int(configuration * config, int slot, char **tokens, int ntokens) {
    float elapsed=(float)(config->status.int_time - config->status.start_time)/1000.0f;
    fprintf(stderr,"Course run INT %s\nIntermediate time: %f seconds\n",tokens[2],elapsed);
    return 0;
}
static int console_mgr_stop(configuration * config, int slot, char **tokens, int ntokens) {
    float elapsed=(float)(config->status.stop_time - config->status.start_time)/1000.0f;
    fprintf(stderr,"Course run STOP %s\nCourse time: %f seconds\n",tokens[2],elapsed);
    return 0;
}
static int console_mgr_fail(configuration * config, int slot, char **tokens, int ntokens) {
    fprintf(stderr,"Sensor(s) failure noticed\n");
    return 0;
}
static int console_mgr_ok(configuration * config, int slot, char **tokens, int ntokens) {
    debug(DBG_TRACE,"chrono OK");
    fprintf(stderr,"Sensor(s) OK . Chronometer is ready\n");
    return 0;
}
/* source msg <duration> <message> <...> */
static int console_mgr_msg(configuration * config, int slot, char **tokens, int ntokens) {
    fprintf(stderr,"Received message:\n");
    for (int n=3;n<ntokens;n++) fprintf(stderr," %s",tokens[n]);
    fprintf(stderr,"\n");
    return 0;
}
static int console_mgr_walk(configuration * config, int slot, char **tokens, int ntokens) {
    int minutes=(ntokens==3)?atoi(tokens[2])/60:7;
    fprintf(stderr,"Rquested %d minutes course walk\n",minutes);
    return 0;
}
static int console_mgr_down(configuration * config, int slot, char **tokens, int ntokens) {
    int seconds=(ntokens==3)?atoi(tokens[2]):15;
    fprintf(stderr,"Starting %d seconds countdown for start running\n",seconds);
    return 0;
}
static int console_mgr_fault(configuration * config, int slot, char **tokens, int ntokens) {
    fprintf(stderr,"Fault/Touch count is: %d\n",config->status.faults+config->status.touchs);
    return 0;
}
static int console_mgr_refusal(configuration * config, int slot, char **tokens, int ntokens) {
    fprintf(stderr,"Refusal count is: %d\n",config->status.refusals);
    return 0;
}
static int console_mgr_elim(configuration * config, int slot, char **tokens, int ntokens) {
    fprintf(stderr,"Eliminated status is: %d",config->status.eliminated);
    return 0;
}
static int console_mgr_data(configuration * config, int slot, char **tokens, int ntokens) {
    fprintf(stderr,"Received DATA. F:%d R:%d E:%d\n",config->status.faults+config->status.touchs,config->status.refusals,config->status.eliminated);
    return 0;
}
static int console_mgr_reset(configuration * config, int slot, char **tokens, int ntokens) {
    fprintf(stderr,"Received RESET\n");
    return 0;
}
static int console_mgr_help(configuration * config, int slot, char **tokens, int ntokens) {
    return sc_help(config,ntokens,tokens);
}
static int console_mgr_version(configuration * config, int slot, char **tokens, int ntokens) {
    fprintf(stdout,"Console_manager version:%s\n",CONSOLE_MGR_VERSION);
    return 0;
}
static int console_mgr_exit(configuration * config, int slot, char **tokens, int ntokens) {
    fprintf(stderr,"Exit requested");
    return 0; // command results ok, not error
}
static int console_mgr_server(configuration * config, int slot, char **tokens, int ntokens) {
    fprintf(stderr,"Tryng to connect to AgilityContest server at: %s\n",config->ajax_server);
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
static int console_mgr_numero(configuration * config, int slot, char **tokens, int ntokens) {
    fprintf(stderr,"Entering dog number: %d\n",config->status.numero);
    return 0;
}
static int console_mgr_bright(configuration * config, int slot, char **tokens, int ntokens) {
    fprintf(stderr,"Setting display bright: %d\n",config->bright);
    return 0;
}

static int console_mgr_clock(configuration * config, int slot, char **tokens, int ntokens) {
    fprintf(stderr,"Entering clock mode");
    return 0;
}

static int console_mgr_debug(configuration * config, int slot, char **tokens, int ntokens) {
    fprintf(stderr,"New debug level is: %d\n",get_debug_level());
    return 0;
}

static func entries[32]= {
        console_mgr_start,  // { 0, "start",   "Start of course run",             "[miliseconds] {0}"},
        console_mgr_int,    // { 1, "int",     "Intermediate time mark",          "<miliseconds>"},
        console_mgr_stop,   // { 2, "stop",    "End of course run",               "<miliseconds>"},
        console_mgr_fail,   // { 3, "fail",    "Sensor faillure detected",        ""},
        console_mgr_ok,     // { 4, "ok",      "Sensor recovery. Chrono ready",   ""},
        console_mgr_msg,    // { 5, "msg",     "Show message on chrono display",  "<seconds>:<message>"},
        console_mgr_walk,   // { 6, "walk",    "Course walk (0:stop)",            "<seconds> {420}"},
        console_mgr_down,   // { 7, "down",    "Start 15 seconds countdown",      ""},
        console_mgr_fault,  // { 8, "fault",   "Mark fault (+/-/#)",              "< + | - | num >"},
        console_mgr_refusal,// { 9, "refusal", "Mark refusal (+/-/#)",            "< + | - | num >"},
        console_mgr_elim,   // { 10, "elim",    "Mark elimination [+-]",           "[ + | - ] {+}"},
        console_mgr_data,   // { 11, "data",   "Set faults/refusal/disq info",    "<flt>:<reh>:<disq>"},
        console_mgr_reset,  // { 12, "reset",  "Reset chronometer and countdown", "" },
        console_mgr_help,   // { 13, "help",   "show command list",               "[cmd]"},
        console_mgr_version,// { 14, "version","Show software version",           "" },
        console_mgr_exit,   // { 15, "exit",   "End program (from console)",      "" },
        console_mgr_server, // { 16, "server", "Set server IP address",           "<x.y.z.t> {0.0.0.0}" },
        console_mgr_ports,  // { 17, "ports",  "Show available serial ports",     "" },
        console_mgr_config, // { 18, "config", "List configuration parameters",   "" },
        console_mgr_status, // { 19, "status", "Show faults/refusal/elim info",   "" },
        console_mgr_numero, // { 20, "turn",   "Set current dog order number [+-#]", "[ + | - | num ] {+}"},
        console_mgr_bright, // { 21, "bright", "Set display bright level [+-#]",  "[ + | - | num ] {+}"},
        console_mgr_clock,  // { 22, "clock",  "Enter clock mode",                "[ hh:mm:ss ] {current time}"},
        console_mgr_debug,  // { 23, "debug",  "Get/Set debug level",             "[ new_level ]"},
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
        char *p=fgets(&request[offset],1024-offset,stdin);
        if (p==NULL) {
            debug(DBG_TRACE,"Console: received EOF from user input");
            snprintf(request,1024-offset,"exit");  // received eof from stdin -> quit command
            res=strlen(request);
        }
        if ((p=strchr(request, '\n')) != NULL) *p='\0'; //strip newline
        if (strlen(&request[offset])==0) continue; // empty string received
        debug(DBG_TRACE,"Console: sending to local socket: '%s'",request);
        res=send(slot->sock,request,strlen(request),0);
        if (res<0){
            debug(DBG_ERROR,"Console send(): error sending request: %s",strerror(errno));
            continue;
        }
        res=recv(slot->sock,response,1024,0);
        if (res<0) {
            debug(DBG_ERROR,"Console recv(): error waiting response: %s",strerror(errno));
            continue;
        } else {
            response[res] = '\0'; // put eol at end of recvd string
            debug(DBG_NOTICE, "%s main loop command response: %s\n", SC_CONSOLE, response);
        }
        // check for end requested
        if (slot->index<0) {
            debug(DBG_TRACE,"Console: 'exit' command invoked");
            res=-1;
        }
    }
    free(request);
    free(response);
    debug(DBG_TRACE,"Exiting console thread");
    slot->index=-1;
    return &slot->index;
}