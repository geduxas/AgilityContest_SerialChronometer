//
// Created by jantonio on 5-Mar-2020
//
#define AGILITYCONTEST_SERIALCHRONOMETER_NETWORK_MGR_C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>

#include "main.h"
#include "debug.h"
#include "net_mgr.h"
#include "sc_config.h"
#include "sc_sockets.h"
#include "modules.h"


static struct {
    int (*module_init)(configuration *config);
    int (*module_end)();
    int (*module_open)();
    int (*module_close)();
    int (*module_read)(char *buffer,size_t length);
    int (*module_write)(char **tokens,size_t ntokens);
    char *(*module_error)();
} entry_points;

static int net_mgr_start(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_NETWORK,tokens[0])==0) return 0; // to avoid get/put loop
    if (ntokens==2) tokens[ntokens++]="0"; // up to 32 tokens available
    return  entry_points.module_write(tokens,ntokens);
}
static int net_mgr_int(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_NETWORK,tokens[0])==0) return 0; // to avoid get/put loop
    return entry_points.module_write(tokens,ntokens);
}
static int net_mgr_stop(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_NETWORK,tokens[0])==0) return 0; // to avoid get/put loop
    return entry_points.module_write(tokens,ntokens);
}
static int net_mgr_fail(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_NETWORK,tokens[0])==0) return 0; // to avoid get/put loop
    // fail msg is not to be sent to chrono (read only)
    debug(DBG_TRACE,"Serial command 'FAIL' (do not send)");
    return 0;
}
static int net_mgr_ok(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_NETWORK,tokens[0])==0) return 0; // to avoid get/put loop
    // fail msg is not to be sent to chrono (read only)
    debug(DBG_TRACE,"Serial command 'OK' (do not send)");
    return 0;
}
static int net_mgr_msg(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_NETWORK,tokens[0])==0) return 0; // to avoid get/put loop
    return entry_points.module_write(tokens,ntokens);
}
static int net_mgr_walk(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_NETWORK,tokens[0])==0) return 0; // to avoid get/put loop
    if (ntokens==2) tokens[ntokens++]="420"; // up to 32 tokens available
    return entry_points.module_write(tokens,ntokens);
}
static int net_mgr_down(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_NETWORK,tokens[0])==0) return 0; // to avoid get/put loop
    if (ntokens==2) tokens[ntokens++]="15"; // up to 32 tokens available
    return entry_points.module_write(tokens,ntokens);
}
static int net_mgr_fault(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_NETWORK,tokens[0])==0) return 0; // to avoid get/put loop
    if (ntokens==2) tokens[ntokens++]="+"; // up to 32 tokens available
    return entry_points.module_write(tokens,ntokens);
}
static int net_mgr_refusal(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_NETWORK,tokens[0])==0) return 0; // to avoid get/put loop
    if (ntokens==2) tokens[ntokens++]="+"; // up to 32 tokens available
    return entry_points.module_write(tokens,ntokens);
}
static int net_mgr_elim(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_NETWORK,tokens[0])==0) return 0; // to avoid get/put loop
    if (ntokens==2) tokens[ntokens++]="+"; // up to 32 tokens available
    return entry_points.module_write(tokens,ntokens);
}
static int net_mgr_data(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_NETWORK,tokens[0])==0) return 0; // to avoid get/put loop
    return entry_points.module_write(tokens,ntokens);
}
static int net_mgr_reset(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_NETWORK,tokens[0])==0) return 0; // to avoid get/put loop
    return entry_points.module_write(tokens,ntokens);
}
static int net_mgr_exit(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_NETWORK,tokens[0])==0) return 0; // to avoid get/put loop
    debug(DBG_INFO,"Serial manager thread exit requested");
    return -1;
}
static int net_mgr_numero(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_NETWORK,tokens[0])==0) return 0; // to avoid get/put loop
    if (ntokens==2) tokens[ntokens++]="+"; // up to 32 tokens available
    return entry_points.module_write(tokens,ntokens);
}
/* config is used in net_mgr to just call configuration and refresh internal data */
static int net_mgr_config(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_NETWORK,tokens[0])==0) return 0; // to avoid get/put loop
    return entry_points.module_write(tokens,ntokens);
}
/* status is used in net_mgr to just call status and refresh internal data */
static int net_mgr_status(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_NETWORK,tokens[0])==0) return 0; // to avoid get/put loop
    return entry_points.module_write(tokens,ntokens);
}
static int net_mgr_bright(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_NETWORK,tokens[0])==0) return 0; // to avoid get/put loop
    if (ntokens==2) tokens[ntokens++]="+"; // up to 32 tokens available
    return entry_points.module_write(tokens,ntokens);
}
static int net_mgr_clock(configuration * config, int slot, char **tokens, int ntokens) {
    if (strcmp(SC_NETWORK,tokens[0])==0) return 0; // to avoid get/put loop
    return entry_points.module_write(tokens,ntokens);
}

static func entries[32]= {
        net_mgr_start,  // { 0, "start",   "Start of course run",             "[miliseconds] {0}"},
        net_mgr_int,    // { 1, "int",     "Intermediate time mark",          "<miliseconds>"},
        net_mgr_stop,   // { 2, "stop",    "End of course run",               "<miliseconds>"},
        net_mgr_fail,   // { 3, "fail",    "Sensor faillure detected",        ""},
        net_mgr_ok,     // { 4, "ok",      "Sensor recovery. Chrono ready",   ""},
        net_mgr_msg,    // { 5, "msg",     "Show message on chrono display",  "<message> [seconds] {2}"},
        net_mgr_walk,   // { 6, "walk",    "Course walk (0:stop)",            "<seconds> {420}"},
        net_mgr_down,   // { 7, "down",    "Start 15 seconds countdown",      ""},
        net_mgr_fault,  // { 8, "fault",   "Mark fault (+/-/#)",              "< + | - | num >"},
        net_mgr_refusal,// { 9, "refusal", "Mark refusal (+/-/#)",            "< + | - | num >"},
        net_mgr_elim,   // { 10, "elim",    "Mark elimination [+-]",           "[ + | - ] {+}"},
        net_mgr_data,   // { 11, "data",   "Set faults/refusal/disq info",    "<flt>:<reh>:<disq>"},
        net_mgr_reset,  // { 12, "reset",  "Reset chronometer and countdown", "" },
        NULL,           // { 13, "help",   "show command list",               "[cmd]"},
        NULL,           // { 14, "version","Show software version",           "" },
        net_mgr_exit,   // { 15, "exit",   "End program (from console)",      "" },
        NULL,           // { 16, "server", "Set server IP address",           "<x.y.z.t> {0.0.0.0}" },
        NULL,           // { 17, "ports",  "Show available serial ports",     "" },
        net_mgr_config, // { 18, "config", "List configuration parameters",   "" },
        net_mgr_status, // { 19, "status", "Show faults/refusal/elim info",   "" },
        net_mgr_numero, // { 20, "turn",   "Set current dog order number [+-#]", "[ + | - | num ] {+}"},
        net_mgr_bright, // { 21, "bright", "Set display bright level (0..9) [+-#]","[ + | - | num ] {+}"},
        net_mgr_clock,  // { 22, "clock",  "Enter clock mode",                "[ hh:mm:ss ] {current time}"},
        NULL,           // { 23, "debug",  "Get/Set debug level",             "[ new_level ]"},
        NULL            // { -1, NULL,     "",                                "" }
};

void *network_manager_thread(void *arg){

    int slotIndex= * ((int *)arg);
    sc_thread_slot *slot=&sc_threads[slotIndex];
    configuration *config=slot->config;
    slot->entries=entries;
    // clear pointers
    memset(&entry_points,0,sizeof(entry_points));
    // create sock
    char tmpstr[1024];
    snprintf(tmpstr,1024,"%d",config->local_port+config->ring);
    slot->sock=connectUDP("localhost",tmpstr);
    if (slot->sock <0) {
        debug(DBG_ERROR,"NetworkMgr: Cannot create local socket");
        slot->index=-1;
        return NULL;
    }

    // load serial module
#ifdef _WIN32
    snprintf(tmpstr,1024,"%s.dll",config->module);
#else
    snprintf(tmpstr,1024,"%s/%s.so",getcwd(NULL,1024),config->module);
#endif
    debug(DBG_TRACE,"Loading (network) driver library '%s'",config->module);
    void *library=dlopen(tmpstr,RTLD_LAZY|RTLD_LOCAL);
    if (!library) {
        debug(DBG_ERROR,"NetworkMgr: Cannot load network driver dll '%s':%s",config->module,dlerror());
        config->comm_port=NULL;
        slot->index=-1;
        return NULL;
    }
    // verify module
    debug(DBG_TRACE,"verifying network library module");
    entry_points.module_init=dlsym(library,"module_init");
    entry_points.module_end=dlsym(library,"module_end");
    entry_points.module_open=dlsym(library,"module_open");
    entry_points.module_close=dlsym(library,"module_close");
    entry_points.module_read=dlsym(library,"module_read");
    entry_points.module_write=dlsym(library,"module_write");
    entry_points.module_error=dlsym(library,"module_error");
    if (!entry_points.module_init || !entry_points.module_end ||
        !entry_points.module_open || !entry_points.module_close ||
        !entry_points.module_read || !entry_points.module_write || !entry_points.module_error) {
        debug(DBG_ERROR,"NetworkMgr: missing symbol() in module %s",config->module);
        dlclose(library);
        config->comm_port=NULL;
        slot->index=-1;
        return NULL;
    }

    // init module
    debug(DBG_TRACE,"initialize network library module");
    if ( entry_points.module_init(config) < 0) {
        debug(DBG_TRACE,"NetworkMgr: module_init() failed");
        dlclose(library);
        config->comm_ipaddr=NULL;
        slot->index=-1;
        return NULL;
    }

    // open module port
    debug(DBG_TRACE,"opening network library module device");
    if ( entry_points.module_open(config) < 0) {
        entry_points.module_end();
        dlclose(library);
        debug(DBG_TRACE,"NetworkMgr: module_open() failed");
        config->comm_ipaddr=NULL;
        slot->index=-1;
        return NULL;
    }

    // mark thread alive before entering loop
    slot->index=slotIndex;
    int res=0;
    char *p;
    char request[1024];
    sprintf(request,"%s ",SC_NETWORK);
    int offset=strlen(request);
    char response[1024];
    while(res>=0) {
        res=entry_points.module_read(&request[offset],sizeof(request)-offset);
        if (res<0) {
            debug(DBG_ERROR,"NetworkMgr read() returns %d",res);
            res=0;
            continue;
        }
        request[offset+res]='\0';
        if ((p=strchr(request, '\n')) != NULL) *p='\0'; //strip newline
        if (strlen(&request[offset])==0) continue; // empty string received
        debug(DBG_TRACE,"NetworkMgr: sending to local socket: '%s'",request);
        res=send(slot->sock,request,strlen(request),0);
        if (res<0){
            debug(DBG_ERROR,"NetworkMgr send(): error sending request: %s",strerror(errno));
            continue;
        }
        res=recv(slot->sock,response,1024,0);
        if (res<0) {
            debug(DBG_NOTICE,"NetworkMgr recv(): error waiting response: %s",strerror(errno));
            res=0; // not really a fatal error. try to continue
            continue;
        } else {
            response[res]='\0';
            debug(DBG_DEBUG,"NetworkMgr command response: %s\n",response);
        }
        if (slot->index<0) {
            debug(DBG_TRACE,"NetworkMgr thread: 'exit' command invoked");
            res=-1;
        }
    }

    // close port
    entry_points.module_close();
    // deinit module
    entry_points.module_end();
    // unload module
    dlclose(library);
    // exit thread
    debug(DBG_TRACE,"Exiting NetworkMgr thread");
    slot->index=-1;
    return &slot->index;
}
