//
// Created by jantonio on 7/05/19.
//
#define SERIALCHRONOMETER_PARSER_C

#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "sc_config.h"
#include "serial_mgr.h"
#include "parser.h"

command_t command_list[32]= {
        { 0, "start",   "Start of course run",             "[miliseconds] {0}"},
        { 1, "int",     "Intermediate time mark",          "<miliseconds>"},
        { 2, "stop",    "End of course run",               "<miliseconds>"},
        { 3, "fail",    "Sensor faillure detected",        ""},
        { 4, "ok",      "Sensor recovery. Chrono ready",   ""},
        { 5, "msg",     "Show message on chrono display",  "<message> [seconds] {2}"},
        { 6, "walk",    "Course walk (0:stop)",            "<seconds> {420}"},
        { 7, "down",    "Start 15 seconds countdown",      ""},
        { 8, "fault",   "Mark fault (+/-/#)",              "< + | - | num {+}>"},
        { 9, "refusal", "Mark refusal (+/-/#)",            "< + | - | num {+}>"},
        { 10, "elim",    "Mark elimination [+-]",          "[ + | - ] {+}"},
        { 11, "data",    "Set course fault:ref:disq info", "<faults>:<refulsals>:<disq>"},
        { 12, "reset",  "Reset chronometer and countdown", "" },
        { 13, "help",   "show command list",               "[cmd]"},
        { 14, "version","Show software version",           "" },
        { 15, "exit",   "End program (from console)",      "" },
        { 16, "server", "Set server IP address",           "<x.y.z.t> {0.0.0.0}" },
        { 17, "ports",  "Show available serial ports",     "" },
        { 18, "config", "List configuration parameters",   "" },
        { 19, "status", "Show Fault:Refusal:Elim state",   "" },
        { 20, "turn",   "Set current dog order number [+-#]", "[ + | - | num ] {+}"},
        { 21, "bright", "Set display bright (0..9) [+-#]", "[ + | - | num ] {+} 0:off"},
        { 22, "clock",  "Enter clock mode",                "[ hh:mm:ss ] {current time}"},
        { 23, "debug",  "Get/Set debug level",             "[ new_level ]"},
        { -1, NULL,     "",                                "" }
};

// argv[0]:source argv[1]:command argv[2]:argument 1...
int sc_help( configuration *config, int argc, char *argv[]) {
    if (argc>2) {
        char *cmd=argv[2]; // get command to obtaing help from
        for ( int n=0; command_list[n].cmd;n++) {
            if (strcmp(command_list[n].cmd,cmd)==0) {
                fprintf(stderr,"Descr.:\t%s \nUsage:\t%s %s\n",command_list[n].desc,command_list[n].cmd,command_list[n].args);
                return 0;
            }
        }
        fprintf(stderr,"Command %s not found",cmd);
        return 1;
    } else {
        fprintf(stderr,"List of available commands:\n");
        for ( int n=0; command_list[n].cmd;n++) {
            fprintf(stderr,"\t%s: %s\n",command_list[n].cmd, command_list[n].desc);
        }
    }
    return 0;
}

int sc_exit(configuration *config, int argc, char *argv[]) {
    return -1;
}

int sc_enumerate_ports(configuration *config, int argc, char *argv[]) {
    serial_print_ports(config);
    return 0;
}

int sc_print_configuration(configuration *config, int argc, char *argv[]) {
    print_configuration(config);
    return 0;
}

int sc_print_status(configuration *config, int argc, char *argv[]) {
    print_status(config);
    return 0;
}