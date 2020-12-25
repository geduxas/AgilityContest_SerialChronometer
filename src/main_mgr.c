//
// Created by jantonio on 5/05/19.
//

#define AGILITYCONTEST_SERIALCHRONOMETER_MAIN_MGR_C

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "../include/main.h"
#include "../include/debug.h"
#include "../include/sc_tools.h"
#include "../include/main_mgr.h"
#include "../include/sc_config.h"

/* start [timestamp] */
static int main_mgr_start(configuration * config, int slot, char **tokens, int ntokens) {
    config->status.start_time= (ntokens==2)?0:strtol(tokens[2],NULL,10);
    debug(DBG_TRACE,"%s -> Command: START timestamp:%llu\n",tokens[0],config->status.start_time);
    return 0;
}
static int main_mgr_int(configuration * config, int slot, char **tokens, int ntokens) {
    if (config->status.start_time<0) {
        debug(DBG_ERROR,"INT: chrono is not running");
        return -1;
    }
    if (ntokens==2) {
        debug(DBG_ERROR,"INT: Intermediate timestamp is not provided");
        return -1;
    }
    config->status.int_time= strtol(tokens[2],NULL,10);
    float elapsed=(float)(config->status.int_time - config->status.start_time)/1000.0f;
    debug(DBG_NOTICE,"%s -> Command: INT elapsed time:%f seconds",tokens[0],elapsed);
    return 0;
}
static int main_mgr_stop(configuration * config, int slot, char **tokens, int ntokens) {
    if (config->status.start_time<0) {
        debug(DBG_ERROR,"STOP: chrono is not running");
        return -1;
    }
    if (ntokens==2) {
        debug(DBG_ERROR,"STOP: Final timestamp is not provided");
        return -1;
    }
    config->status.stop_time= strtol(tokens[2],NULL,10);
    float elapsed=(float)(config->status.stop_time - config->status.start_time)/1000.0f;
    debug(DBG_NOTICE,"%s -> Command: STOP elapsed time:%f",tokens[0],elapsed);
    return 0;
}
static int main_mgr_fail(configuration * config, int slot, char **tokens, int ntokens) {
    debug(DBG_INFO,"%s -> Command: FAIL",tokens[0]);
    return 0;
}
static int main_mgr_ok(configuration * config, int slot, char **tokens, int ntokens) {
    debug(DBG_INFO,"%s -> Command: OK",tokens[0]);
    return 0;
}
/* source msg <duration> <message> <...> */
static int main_mgr_msg(configuration * config, int slot, char **tokens, int ntokens) {
    debug(DBG_INFO,"%s -> Command: MSG %s %s (...) ",tokens[0],tokens[2],tokens[3]);
    return 0;
}

static int main_mgr_walk(configuration * config, int slot, char **tokens, int ntokens) {
    int minutes=(ntokens==3)?atoi(tokens[2])/60:7;
    debug(DBG_INFO,"%s -> Command: WALK %d (min)",tokens[0],minutes);
    return 0;
}

static int main_mgr_down(configuration * config, int slot, char **tokens, int ntokens) {
    int seconds=(ntokens==3)?atoi(tokens[2]):15;
    debug(DBG_INFO,"%s -> Command: DOWN %d",tokens[0],seconds);
    return 0;
}

static int main_mgr_fault(configuration * config, int slot, char **tokens, int ntokens) {
    if (ntokens==3) {
        if (strcmp(tokens[2],"+")==0) { // fault +
            config->status.faults++;
        }
        else if (strcmp(tokens[2],"-")==0) { // fault -
            if(config->status.faults > 0) config->status.faults--;
            else if (config->status.touchs>0) config->status.touchs--;
        } else { // faults #absolute
            config->status.faults = atoi(tokens[2])-config->status.touchs;
        }
    } else { // no sufixx: assume increase fault count
        config->status.faults++;
    }
    if (config->status.faults<0) config->status.faults=0;
    if (config->status.touchs<0) config->status.touchs=0;
    debug(DBG_INFO,"%s -> Command: FAULT %d",tokens[0],config->status.faults);
    return 0;
}

static int main_mgr_refusal(configuration * config, int slot, char **tokens, int ntokens) {
    if (ntokens==3) {
        if (strcmp(tokens[2],"+")==0) config->status.refusals++;
        else if (strcmp(tokens[2],"-")==0) config->status.refusals--;
        else config->status.refusals=atoi(tokens[2]);
    } else {
        config->status.refusals++;
    }
    if (config->status.refusals<0) config->status.refusals=0;
    debug(DBG_INFO,"%s -> Command: REFUSAL %d",tokens[0],config->status.refusals);
    return 0;
}

static int main_mgr_elim(configuration * config, int slot, char **tokens, int ntokens) {
    if (ntokens==3) {
        if (strcmp(tokens[2],"+")==0) config->status.eliminated=1;
        else if (strcmp(tokens[2],"-")==0) config->status.eliminated=0;
        else config->status.eliminated= ( atoi(tokens[2])==0)?0:1;
    } else {
        config->status.eliminated=1;
    }
    debug(DBG_INFO,"%s -> Command: ELIM %d",tokens[0],config->status.eliminated);
    return 0;
}
static int main_mgr_data(configuration * config, int slot, char **tokens, int ntokens) {
    if (ntokens==2) {
        debug(DBG_ERROR,"%s -> Command: DATA missing F:R:E values",tokens[0]);
        return -1;
    }
    char *str=strdup(tokens[2]);
    char *pt=strchr(str,':');
    *pt++='\0';
    char *pt2=strchr(pt,':');
    *pt2++='\0';
    config->status.faults = atoi(str) - config->status.touchs;
    config->status.refusals = atoi(pt);
    config->status.eliminated = (atoi(pt2)==0)?0:1;
    debug(DBG_INFO,"%s -> Command: DATA %s",tokens[0],tokens[2]);
    return 0;
}
static int main_mgr_reset(configuration * config, int slot, char **tokens, int ntokens) {
    config->status.start_time=-1L;
    config->status.int_time=0L;
    config->status.stop_time=0L;
    config->status.faults=0;
    config->status.touchs=0;
    config->status.refusals=0;
    config->status.eliminated=0;
    config->status.notpresent=0;
    // DO NOT reset "numero" nor tanda related values
    debug(DBG_INFO,"%s -> Command: RESET",tokens[0]);
    return 0;
}
static int main_mgr_help(configuration * config, int slot, char **tokens, int ntokens) {
    debug(DBG_INFO,"%s -> Command: HELP %s",tokens[0],(ntokens==3)?tokens[2]:"");
    return 0;
}
static int main_mgr_version(configuration * config, int slot, char **tokens, int ntokens) {
    debug(DBG_INFO,"%s -> Command: VERSION %s",tokens[0],MAIN_MGR_VERSION);
    return 0;
}
static int main_mgr_exit(configuration * config, int slot, char **tokens, int ntokens) {
    debug(DBG_INFO,"%s -> Command: EXIT",tokens[0]);
    return 0; // command results ok, not error
}
static int main_mgr_server(configuration * config, int slot, char **tokens, int ntokens) {
    debug(DBG_INFO,"%s -> Command: SERVER %s",tokens[0],tokens[2]);
    return 0;
}
static int main_mgr_ports(configuration * config, int slot, char **tokens, int ntokens) {
    debug(DBG_INFO,"%s -> Command: PORTS",tokens[0]);
    return 0;
}
static int main_mgr_config(configuration * config, int slot, char **tokens, int ntokens) {
    debug(DBG_INFO,"%s -> Command: CONFIG",tokens[0]);
    return 0;
}
static int main_mgr_status(configuration * config, int slot, char **tokens, int ntokens) {
    debug(DBG_INFO,"%s -> Command: STATUS",tokens[0]);
    return 0;
}
static int main_mgr_numero(configuration * config, int slot, char **tokens, int ntokens) {
    if (ntokens==3) {
        if (strcmp(tokens[2],"+")==0) config->status.numero++;
        else if (strcmp(tokens[2],"-")==0) config->status.numero--;
        else config->status.numero=atoi(tokens[2]);
    } else {
        config->status.numero++;
    }
    if (config->status.numero<0) config->status.numero=0;
    debug(DBG_INFO,"%s -> Command: TURN %d",tokens[0],config->status.numero);
    return 0;
}
static int main_mgr_bright(configuration * config, int slot, char **tokens, int ntokens) {
    if (ntokens==3) {
        if (strcmp(tokens[2],"+")==0) config->bright++;
        else if (strcmp(tokens[2],"-")==0) config->bright--;
        else config->bright=atoi(tokens[2]);
    } else {
        config->bright++;
    }
    if (config->bright<0) config->bright=0;
    if (config->bright>9) config->bright=9;
    debug(DBG_INFO,"%s -> Command: BRIGHT %d",tokens[0],config->status.numero);
    return 0;
}
static int main_mgr_clock(configuration * config, int slot, char **tokens, int ntokens) {
    debug(DBG_INFO,"%s -> Command: CLOCK",tokens[0]);
    return 0;
}
static int main_mgr_dorsal(configuration * config, int slot, char **tokens, int ntokens) {
    debug(DBG_INFO,"%s -> Command: DORSAL %s",tokens[0],tokens[2]);
    return 0;
}
static int main_mgr_debug(configuration * config, int slot, char **tokens, int ntokens) {
    if (ntokens==3) { set_debug_level(atoi(tokens[2])); }
    debug(DBG_INFO,"%s -> Command: DEBUG %d",tokens[0],config->status.numero);
    return 0;
}

func main_mgr_entries[32]= {
        main_mgr_start,  // { 0, "start",   "Start of course run",             "[miliseconds] {0}"},
        main_mgr_int,    // { 1, "int",     "Intermediate time mark",          "<miliseconds>"},
        main_mgr_stop,   // { 2, "stop",    "End of course run",               "<miliseconds>"},
        main_mgr_fail,   // { 3, "fail",    "Sensor faillure detected",        ""},
        main_mgr_ok,     // { 4, "ok",      "Sensor recovery. Chrono ready",   ""},
        main_mgr_msg,    // { 5, "msg",     "Show message on chrono display",  "<seconds>:<message>"},
        main_mgr_walk,   // { 6, "walk",    "Course walk (0:stop)",            "<seconds> {420}"},
        main_mgr_down,   // { 7, "down",    "Start 15 seconds countdown",      ""},
        main_mgr_fault,  // { 8, "fault",   "Mark fault (+/-/#)",              "< + | - | num >"},
        main_mgr_refusal,// { 9, "refusal", "Mark refusal (+/-/#)",            "< + | - | num >"},
        main_mgr_elim,   // { 10, "elim",    "Mark elimination [+-]",           "[ + | - ] {+}"},
        main_mgr_data,   // { 11, "data",   "Set faults/refusal/disq info",    "<flt>:<reh>:<disq>"},
        main_mgr_reset,  // { 12, "reset",  "Reset chronometer and countdown", "" },
        main_mgr_help,   // { 13, "help",   "show command list",               "[cmd]"},
        main_mgr_version,// { 14, "version","Show software version",           "" },
        main_mgr_exit,   // { 15, "exit",   "End program (from console)",      "" },
        main_mgr_server, // { 16, "server", "Set server IP address",           "<x.y.z.t> {0.0.0.0}" },
        main_mgr_ports,  // { 17, "ports",  "Show available serial ports",     "" },
        main_mgr_config, // { 18, "config", "List configuration parameters",   "" },
        main_mgr_status, // { 19, "status", "Show faults/refusal/elim info",   "" },
        main_mgr_numero, // { 20, "turn",   "Set current dog order number [+-#]", "[ + | - | num ] {+}"},
        main_mgr_bright, // { 21, "bright", "Set display bright level (0..9) [+-#]","[ + | - | num ] {+}"},
        main_mgr_clock,  // { 22, "clock",  "Enter clock mode",                "[ hh:mm:ss ] {current time}"},
        main_mgr_debug,  // { 23, "debug",  "Get/Set debug level",             "[ new_level ]"},
        main_mgr_dorsal, // { 24, "dorsal",  "get dorsal from qrcode reader",  "[ dorsal # ]"},
        NULL             // { -1, NULL,     "",                                "" }
};
