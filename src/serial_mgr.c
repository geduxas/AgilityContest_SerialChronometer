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

static int serial_write(configuration *config,char *cmd,char *arg1, char *arg2) {
    static char *msg;
    if (!msg) msg=calloc(1024,sizeof(char));
    if (arg2) snprintf(msg,1024,"%s %s %s",cmd,arg1,arg2);
    else if (arg1) snprintf(msg,1024,"%s %s",cmd,arg1);
    else  snprintf(msg,1024,"%s",cmd);
    debug(DBG_TRACE,"Serial msg '%s'",msg);
    strncat(msg,"\n",1024); // add newline
    enum sp_return ret=sp_nonblocking_write(config->serial_port,msg,strlen(msg));
    return ret;
}

static int serial_mgr_start(configuration * config, int slot, char **tokens, int ntokens) {
    if (ntokens==2) return serial_write(config,tokens[1], "0",NULL);
    return serial_write(config,tokens[1],tokens[2],NULL );
}
static int serial_mgr_int(configuration * config, int slot, char **tokens, int ntokens) {
    return serial_write(config,tokens[1],tokens[2],NULL );
}
static int serial_mgr_stop(configuration * config, int slot, char **tokens, int ntokens) {
    return serial_write(config,tokens[1],tokens[2],NULL );
}
static int serial_mgr_fail(configuration * config, int slot, char **tokens, int ntokens) {
    // fail msg is not to be sent to chrono (read only)
    debug(DBG_TRACE,"Serial msg 'FAIL' (do not send)");
    return 0;
}
static int serial_mgr_ok(configuration * config, int slot, char **tokens, int ntokens) {
    // fail msg is not to be sent to chrono (read only)
    debug(DBG_TRACE,"Serial msg 'OK' (do not send)");
    return 0;
}
static int serial_mgr_msg(configuration * config, int slot, char **tokens, int ntokens) {
    // PENDING; properly handle multi word messages
    return 0;
}
static int serial_mgr_walk(configuration * config, int slot, char **tokens, int ntokens) {
    if (ntokens==2) return serial_write(config,tokens[1], "420",NULL); // default 7 minutes
    return serial_write(config,tokens[1],tokens[2],NULL );
}
static int serial_mgr_down(configuration * config, int slot, char **tokens, int ntokens) {
    if (ntokens==2) return serial_write(config,tokens[1], "15",NULL); // default 15 seconds
    return serial_write(config,tokens[1],tokens[2],NULL );
}
static int serial_mgr_fault(configuration * config, int slot, char **tokens, int ntokens) {
    if (ntokens==2) return serial_write(config,tokens[1], "+",NULL); // default increase fault
    return serial_write(config,tokens[1],tokens[2],NULL );
}
static int serial_mgr_refusal(configuration * config, int slot, char **tokens, int ntokens) {
    if (ntokens==2) return serial_write(config,tokens[1], "+",NULL); // default increase refusals
    return serial_write(config,tokens[1],tokens[2],NULL );
}
static int serial_mgr_elim(configuration * config, int slot, char **tokens, int ntokens) {
    if (ntokens==2) return serial_write(config,tokens[1], "+",NULL); // default: eliminate
    return serial_write(config,tokens[1],tokens[2],NULL );
}
static int serial_mgr_data(configuration * config, int slot, char **tokens, int ntokens) {
    return serial_write(config,tokens[1],tokens[2],NULL );
}
static int serial_mgr_reset(configuration * config, int slot, char **tokens, int ntokens) {
    return serial_write(config,tokens[1],NULL,NULL );
}
static int serial_mgr_exit(configuration * config, int slot, char **tokens, int ntokens) {
    debug(DBG_INFO,"Serial manager thread exit requested");
    return -1;
}
static int serial_mgr_numero(configuration * config, int slot, char **tokens, int ntokens) {
    return serial_write(config,tokens[1],NULL,NULL );
}
static int serial_mgr_clock(configuration * config, int slot, char **tokens, int ntokens) {
    if (ntokens==2) return serial_write(config,tokens[1], "",NULL); // default use internal clock time
    return serial_write(config,tokens[1],tokens[2],NULL );
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
        serial_mgr_numero // { 20, "turn",   "Set current dog order number [+-#]", "[ + | - | num ] {+}"},
        serial_mgr_clock,  // { 21, "clock",  "Enter clock mode",                "[ hh:mm:ss ] {current time}"},
        NULL,              // { 22, "debug",  "Get/Set debug level",             "[ new_level ]"},
        NULL               // { -1, NULL,     "",                                "" }
};

void *serial_manager_thread(void *arg){

    int (*module_init)(configuration *config) = NULL;
    int (*module_end)() = NULL;
    int (*module_open)() = NULL;
    int (*module_close)() = NULL;
    int (*module_read)(char *buffer,size_t length) = NULL;
    int (*module_write)(char *buffer,size_t length) = NULL;
    char *(*module_error)() = NULL;

    int slotIndex= * ((int *)arg);
    sc_thread_slot *slot=&sc_threads[slotIndex];
    configuration *config=slot->config;
    slot->entries=entries;

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
    module_init=dlsym(library,"module_init");
    module_end=dlsym(library,"module_end");
    module_open=dlsym(library,"module_open");
    module_close=dlsym(library,"module_close");
    module_read=dlsym(library,"module_read");
    module_write=dlsym(library,"module_write");
    module_error=dlsym(library,"module_error");
    if (!module_init || !module_end || !module_open || !module_close || !module_read || !module_write || !module_error) {
        debug(DBG_ERROR,"SerialMgr: missing symbol() in module %s",config->module);
        dlclose(library);
        config->comm_port=NULL;
        slot->index=-1;
        return NULL;
    }

    // init module
    debug(DBG_TRACE,"initialize library module");
    if ( module_init(config) < 0) {
        debug(DBG_TRACE,"SerialMgr: module_init() failed");
        dlclose(library);
        config->comm_port=NULL;
        slot->index=-1;
        return NULL;
    }

    // open module port
    debug(DBG_TRACE,"openin library module device");
    if ( module_open(config) < 0) {
        module_end();
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
        res=sp_blocking_read(config->serial_port,&request[offset],sizeof(request)-offset,5000); // 5seg timeout
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
    module_close();
    // deinit module
    module_end();
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
