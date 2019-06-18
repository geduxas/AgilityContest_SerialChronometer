//
// Created by jantonio on 7/05/19.
//
#define SERIALCHRONOMETER_PARSER_C

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
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
        { 6, "down",    "Start 15 seconds countdown",      ""},
        { 7, "fault",   "Mark fault (+/-/#)",              "< + | - | num >"},
        { 8, "refusal", "Mark refusal (+/-/#)",            "< + | - | num >"},
        { 9, "elim",    "Mark elimination [+-]",           "[ + | - ] {+}"},
        { 10, "reset",  "Reset chronometer and countdown", "" },
        { 11, "help",   "show command list",               "[cmd]"},
        { 12, "version","Show software version",           "" },
        { 13, "exit",   "End program (from console)",      "" },
        { 14, "server", "Set server IP address",           "<x.y.z.t> {0.0.0.0}" },
        { 15, "ports",  "Show available serial ports",     "" },
        { 16, "config", "List configuration parameters",   "" },
        { -1, NULL,     "",                                "" }
};

// argv[0]:source argv[1]:command argv[2]:argument 1...
int sc_help( configuration *config, int argc, char *argv[]) {
    if (argc>2) {
        char *cmd=argv[2]; // get command to obtaing help from
        for ( int n=0; command_list[n].cmd;n++) {
            if (strcmp(command_list[n].cmd,cmd)==0) {
                fprintf(stderr,"\t%s: %s\n",command_list[n].cmd, command_list[n].desc);
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
