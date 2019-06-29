//
// Created by jantonio on 5/05/19.
//
#define AGILITYCONTEST_SERIALCHRONOMETER_SERIAL_MGR_C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>

#include "main.h"
#include "../libserialport/include/libserialport.h"
#include "debug.h"
#include "serial_mgr.h"
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

static int serial_mgr_start(configuration * config, int slot, char **tokens, int ntokens) {
    if (ntokens==2) tokens[ntokens++]="0"; // up to 32 tokens available
    return  entry_points.module_write(tokens,ntokens);
}
static int serial_mgr_int(configuration * config, int slot, char **tokens, int ntokens) {
    return entry_points.module_write(tokens,ntokens);
}
static int serial_mgr_stop(configuration * config, int slot, char **tokens, int ntokens) {
    return entry_points.module_write(tokens,ntokens);
}
static int serial_mgr_fail(configuration * config, int slot, char **tokens, int ntokens) {
    // fail msg is not to be sent to chrono (read only)
    debug(DBG_TRACE,"Serial command 'FAIL' (do not send)");
    return 0;
}
static int serial_mgr_ok(configuration * config, int slot, char **tokens, int ntokens) {
    // fail msg is not to be sent to chrono (read only)
    debug(DBG_TRACE,"Serial command 'OK' (do not send)");
    return 0;
}
static int serial_mgr_msg(configuration * config, int slot, char **tokens, int ntokens) {
    return entry_points.module_write(tokens,ntokens);
}
static int serial_mgr_walk(configuration * config, int slot, char **tokens, int ntokens) {
    if (ntokens==2) tokens[ntokens++]="420"; // up to 32 tokens available
    return entry_points.module_write(tokens,ntokens);
}
static int serial_mgr_down(configuration * config, int slot, char **tokens, int ntokens) {
    if (ntokens==2) tokens[ntokens++]="15"; // up to 32 tokens available
    return entry_points.module_write(tokens,ntokens);
}
static int serial_mgr_fault(configuration * config, int slot, char **tokens, int ntokens) {
    if (ntokens==2) tokens[ntokens++]="+"; // up to 32 tokens available
    return entry_points.module_write(tokens,ntokens);
}
static int serial_mgr_refusal(configuration * config, int slot, char **tokens, int ntokens) {
    if (ntokens==2) tokens[ntokens++]="+"; // up to 32 tokens available
    return entry_points.module_write(tokens,ntokens);
}
static int serial_mgr_elim(configuration * config, int slot, char **tokens, int ntokens) {
    if (ntokens==2) tokens[ntokens++]="+"; // up to 32 tokens available
    return entry_points.module_write(tokens,ntokens);
}
static int serial_mgr_data(configuration * config, int slot, char **tokens, int ntokens) {
    return entry_points.module_write(tokens,ntokens);
}
static int serial_mgr_reset(configuration * config, int slot, char **tokens, int ntokens) {
    return entry_points.module_write(tokens,ntokens);
}
static int serial_mgr_exit(configuration * config, int slot, char **tokens, int ntokens) {
    debug(DBG_INFO,"Serial manager thread exit requested");
    return -1;
}
static int serial_mgr_numero(configuration * config, int slot, char **tokens, int ntokens) {
    if (ntokens==2) tokens[ntokens++]="+"; // up to 32 tokens available
    return entry_points.module_write(tokens,ntokens);
}
static int serial_mgr_clock(configuration * config, int slot, char **tokens, int ntokens) {
    return entry_points.module_write(tokens,ntokens);
}

static func entries[32]= {
        serial_mgr_start,  // { 0, "start",   "Start of course run",             "[miliseconds] {0}"},
        serial_mgr_int,    // { 1, "int",     "Intermediate time mark",          "<miliseconds>"},
        serial_mgr_stop,   // { 2, "stop",    "End of course run",               "<miliseconds>"},
        serial_mgr_fail,   // { 3, "fail",    "Sensor faillure detected",        ""},
        serial_mgr_ok,     // { 4, "ok",      "Sensor recovery. Chrono ready",   ""},
        serial_mgr_msg,    // { 5, "msg",     "Show message on chrono display",  "<message> [seconds] {2}"},
        serial_mgr_walk,   // { 6, "walk",    "Course walk (0:stop)",            "<seconds> {420}"},
        serial_mgr_down,   // { 7, "down",    "Start 15 seconds countdown",      ""},
        serial_mgr_fault,  // { 8, "fault",   "Mark fault (+/-/#)",              "< + | - | num >"},
        serial_mgr_refusal,// { 9, "refusal", "Mark refusal (+/-/#)",            "< + | - | num >"},
        serial_mgr_elim,   // { 10, "elim",    "Mark elimination [+-]",           "[ + | - ] {+}"},
        serial_mgr_data,   // { 11, "data",   "Set faults/refusal/disq info",    "<flt>:<reh>:<disq>"},
        serial_mgr_reset,  // { 12, "reset",  "Reset chronometer and countdown", "" },
        NULL,              // { 13, "help",   "show command list",               "[cmd]"},
        NULL,              // { 14, "version","Show software version",           "" },
        serial_mgr_exit,   // { 15, "exit",   "End program (from console)",      "" },
        NULL,              // { 16, "server", "Set server IP address",           "<x.y.z.t> {0.0.0.0}" },
        NULL,              // { 17, "ports",  "Show available serial ports",     "" },
        NULL,              // { 18, "config", "List configuration parameters",   "" },
        NULL,              // { 19, "status", "Show faults/refusal/elim info",   "" },
        serial_mgr_numero, // { 20, "turn",   "Set current dog order number [+-#]", "[ + | - | num ] {+}"},
        serial_mgr_clock,  // { 21, "clock",  "Enter clock mode",                "[ hh:mm:ss ] {current time}"},
        NULL,              // { 22, "debug",  "Get/Set debug level",             "[ new_level ]"},
        NULL               // { -1, NULL,     "",                                "" }
};

void *serial_manager_thread(void *arg){

    int slotIndex= * ((int *)arg);
    sc_thread_slot *slot=&sc_threads[slotIndex];
    configuration *config=slot->config;
    slot->entries=entries;
    // clear pointers
    memset(&entry_points,0,sizeof(entry_points));
    // create sock
    char tmpstr[1024];
    snprintf(tmpstr,1024,"%d",config->local_port);
    slot->sock=connectUDP("localhost",tmpstr);
    if (slot->sock <0) {
        debug(DBG_ERROR,"SerialMgr: Cannot create local socket");
        slot->index=-1;
        return NULL;
    }

    // load serial module
#ifdef _WIN32
    snprintf(tmpstr,1024,"%s.dll",config->module);
#else
    snprintf(tmpstr,1024,"%s/%s.so",getcwd(NULL,1024),config->module);
#endif
    debug(DBG_TRACE,"Loading driver library '%s'",config->module);
    void *library=dlopen(tmpstr,RTLD_LAZY|RTLD_LOCAL);
    if (!library) {
        debug(DBG_ERROR,"SerialMgr: Cannot load serial driver dll '%s':%s",config->module,dlerror());
        config->comm_port=NULL;
        slot->index=-1;
        return NULL;
    }
    // verify module
    debug(DBG_TRACE,"verifying library module");
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
        debug(DBG_ERROR,"SerialMgr: missing symbol() in module %s",config->module);
        dlclose(library);
        config->comm_port=NULL;
        slot->index=-1;
        return NULL;
    }

    // init module
    debug(DBG_TRACE,"initialize library module");
    if ( entry_points.module_init(config) < 0) {
        debug(DBG_TRACE,"SerialMgr: module_init() failed");
        dlclose(library);
        config->comm_port=NULL;
        slot->index=-1;
        return NULL;
    }

    // open module port
    debug(DBG_TRACE,"openin library module device");
    if ( entry_points.module_open(config) < 0) {
        entry_points.module_end();
        dlclose(library);
        debug(DBG_TRACE,"SerialMgr: module_open() failed");
        config->comm_port=NULL;
        slot->index=-1;
        return NULL;
    }

    // mark thread alive before entering loop
    slot->index=slotIndex;
    int res=0;
    char *p;
    char request[1024];
    sprintf(request,"%s ",SC_SERIAL);
    int offset=strlen(request);
    char response[1024];
    while(res>=0) {
        res=entry_points.module_read(&request[offset],sizeof(request)-offset);
        if (res<0) {
            debug(DBG_ERROR,"Serial read() returns %d",res);
            res=0;
        }
        request[res]='\0';
        if ((p=strchr(request, '\n')) != NULL) *p='\0'; //strip newline
        if (strlen(request)==0) continue; // empty string received
        debug(DBG_TRACE,"Serial: sending to local socket: '%s'",request);
        res=send(slot->sock,request,strlen(request),0);
        if (res<0){
            debug(DBG_ERROR,"Serial send(): error sending request: %s",strerror(errno));
            continue;
        }
        res=recv(slot->sock,response,1024,0);
        if (res<0) {
            debug(DBG_ERROR,"Serial recv(): error waiting response: %s",strerror(errno));
            continue;
        } else {
            response[res]='\0';
            fprintf(stdout,"Serial command response: %s\n",response);
        }
        if (slot->index<0) {
            debug(DBG_TRACE,"Serial thread: 'exit' command invoked");
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
    debug(DBG_TRACE,"Exiting serial thread");
    slot->index=-1;
    return &slot->index;
}

/**
 * Enumerate comm ports and compose a string list
 * @param config configuration data
 * @param nports pointer to return number of ports found
 * @return comm port list, or null if no ports found
 */
char ** serial_ports_enumerate(configuration *config,int *nports){
    int i;
    struct sp_port **ports;

    char **portlist=malloc(sizeof(char*));
    *portlist=NULL;

    enum sp_return error = sp_list_ports(&ports);
    if (error == SP_OK) {
        for (i = 0; ports[i]; i++) {
            char **tmp=realloc(portlist, (i+1) * sizeof(char *));
            if (tmp) {
                portlist=tmp;
                portlist[i] = strdup(sp_get_port_name(ports[i]));
                debug(DBG_INFO,"Found port: '%s'\n", portlist[i] );
                // *(portlist+i+1)=NULL;
            }
        }
        *nports=i;
        sp_free_port_list(ports);
        return portlist;
    } else {
        debug(DBG_INFO,"No serial devices detected\n");
        return NULL;
    }
}

void serial_print_ports(configuration *config) {
    int nports=0;
    char **ports= serial_ports_enumerate(config,&nports);
    if (nports==0) {
        fprintf(stdout,"No available COMM ports found:\n");
    } else {
        fprintf(stdout,"List of available COMM ports:\n");
        for (int i=0;i<nports;i++) {
            fprintf(stdout,"%s\n",ports[i]);
            free(ports[i]);
        }
        free(ports);
    }
}
