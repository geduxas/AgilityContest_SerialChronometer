//
// Created by jantonio on 5/05/19.
//

#define AGILITYCONTEST_SERIALCHRONOMETER_AJAX_MGR_C
#include <stdio.h>
#include <unistd.h>

#include "../include/main.h"
#include "../include/debug.h"
#include "../include/ajax_mgr.h"
#include "../include/sc_config.h"
#include "../include/sc_sockets.h"

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
        ajax_mgr_dorsal,   // { 19, "turn",   "Set current dog order number [+-#]", "[ + | - | num ] {+}"},
        NULL,            // { 20, "clock",  "Enter clock mode",                "[ hh:mm:ss ] {current time}"},
        NULL,            // { 21, "debug",  "Get/Set debug level",             "[ new_level ]"},
        NULL             // { -1, NULL,     "",                                "" }
};

void *ajax_manager_thread(void *arg){
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

    // mark thread alive before entering loop
    slot->index=slotIndex;
    int res=0;
    while(res>=0) {
        sleep(1);
        if (slot->index<0) {
            debug(DBG_TRACE,"Ajax thread: 'exit' command invoked");
            res=-1;
        }
    }
    debug(DBG_TRACE,"Exiting ajax thread");
    slot->index=-1;
    return &slot->index;
}
