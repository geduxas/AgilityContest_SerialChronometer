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
} command_t;

EXTERN char **tokenize(char *line, int *argc);
EXTERN int freetokens(int argc,char *argv[]);
EXTERN int help( configuration *config, int argc, char *argv[]);
EXTERN int sc_exit( configuration *config, int argc, char *argv[]);
EXTERN int sc_print_configuration( configuration *config, int argc, char *argv[]);
EXTERN int sc_enumerate_ports( configuration *config, int argc, char *argv[]);

#undef EXTERN

#endif //SERIALCHRONOMETER_PARSER_H
