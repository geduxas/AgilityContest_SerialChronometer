//
// Created by jantonio on 7/05/19.
//

#ifndef SERIALCHRONOMETER_PARSER_H
#define SERIALCHRONOMETER_PARSER_H

#include "sc_config.h"

#ifdef SERIALCHRONOMETER_PARSER_C
#define EXTERN extern
#else
#define EXTERN
#endif

typedef struct command_st {
    char *cmd; // command name
    char *desc; // command description
    // what to do when executing from serial input thread
    int (*serial_func)(configuration *config, int argc, char *argv[]);
    // what to do when executing from web server thread
    int (*web_func)(configuration *config, int argc, char *argv[]);
    // what to do when executing from ajax event reader thread
    int (*ajax_func)(configuration *config, int argc, char *argv[]);
    // what to do when executing from console (test mode)
    int (*console_func)(configuration *config, int argc, char *argv[]);
} command_t;

EXTERN int parse_cmd(configuration *config, char *tname, char *line);
#undef EXTERN

#endif //SERIALCHRONOMETER_PARSER_H
