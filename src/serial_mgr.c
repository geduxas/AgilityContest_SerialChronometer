//
// Created by jantonio on 5/05/19.
//
#define AGILITYCONTEST_SERIALCHRONOMETER_SERIAL_MGR_C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/main.h"
#include "../libserialport/include/libserialport.h"
#include "../include/debug.h"
#include "../include/serial_mgr.h"
#include "../include/sc_config.h"
#include "../include/sc_sockets.h"

static int serial_mgr_start(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int serial_mgr_int(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int serial_mgr_stop(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int serial_mgr_fail(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int serial_mgr_ok(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int serial_mgr_msg(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int serial_mgr_walk(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int serial_mgr_down(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int serial_mgr_fault(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int serial_mgr_refusal(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int serial_mgr_elim(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int serial_mgr_reset(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int serial_mgr_exit(configuration * config, int slot, char **tokens, int ntokens) {
    debug(DBG_INFO,"Serial manager thread exit requested");
    return -1;
}
static int serial_mgr_server(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static func entries[32]= {
        serial_mgr_start,  // { 0, "start",   "Start of course run",             "[miliseconds] {0}"},
        serial_mgr_int,    // { 1, "int",     "Intermediate time mark",          "<miliseconds>"},
        serial_mgr_stop,   // { 2, "stop",    "End of course run",               "<miliseconds>"},
        serial_mgr_fail,   // { 3, "fail",    "Sensor faillure detected",        ""},
        serial_mgr_ok,     // { 4, "ok",      "Sensor recovery. Chrono ready",   ""},
        serial_mgr_msg,    // { 5, "msg",     "Show message on chrono display",  "<message> [seconds] {2}"},
        serial_mgr_walk,   // { 6, "walk",    "Course walk (0:stop)",            "<seconds> {420}"},
        serial_mgr_down,   // { 6, "down",    "Start 15 seconds countdown",      ""},
        serial_mgr_fault,  // { 7, "fault",   "Mark fault (+/-/#)",              "< + | - | num >"},
        serial_mgr_refusal,// { 8, "refusal", "Mark refusal (+/-/#)",            "< + | - | num >"},
        serial_mgr_elim,   // { 9, "elim",    "Mark elimination [+-]",           "[ + | - ] {+}"},
        serial_mgr_reset,  // { 10, "reset",  "Reset chronometer and countdown", "" },
        NULL,            // { 11, "help",   "show command list",               "[cmd]"},
        NULL,            // { 12, "version","Show software version",           "" },
        serial_mgr_exit,   // { 13, "exit",   "End program (from console)",      "" },
        serial_mgr_server, // { 14, "server", "Set server IP address",           "<x.y.z.t> {0.0.0.0}" },
        NULL,            // { 15, "ports",  "Show available serial ports",     "" },
        NULL,            // { 16, "config", "List configuration parameters",   "" },
        NULL             // { -1, NULL,     "",                                "" }
};

void *serial_manager_thread(void *arg){

    int slotIndex= * ((int *)arg);
    sc_thread_slot *slot=&sc_threads[slotIndex];
    configuration *config=slot->config;
    slot->entries=entries;

    // create sock
    char portstr[16];
    snprintf(portstr,16,"%d",config->local_port);
    slot->sock=connectUDP("localhost",portstr);
    if (slot->sock <0) {
        debug(DBG_ERROR,"SerialMgr: Cannot create local socket");
        return NULL;
    }

    char *errmsg=NULL;
    enum sp_return ret=sp_get_port_by_name(config->comm_port,&config->serial_port);
    if (ret == SP_OK) {
        ret = sp_open(config->serial_port,SP_MODE_READ_WRITE);
        if (ret == SP_OK) {
            sp_set_baudrate(config->serial_port, config->baud_rate);
        } else {
            errmsg="Cannot open serial port %s";
        }
    } else {
        errmsg="Cannot locate serial port %s";
    }
    if (errmsg) {
        // on error show message and disable comm port and associated thread handling
        debug(DBG_ERROR,errmsg,config->comm_port);
        if (config->serial_port) {
            sp_free_port(config->serial_port);
            config->serial_port=NULL;
        }
        config->comm_port=NULL;
        return NULL;
    }

    // mark thread alive before entering loop
    slot->index=slotIndex;

    return 0;
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

/**
 * Send data to serial port
 * @param config configuration pointer
 * @param data data
 * @return number of bytes sent or -1 on error
 */
int serial_write(configuration *config, char *data) {

}