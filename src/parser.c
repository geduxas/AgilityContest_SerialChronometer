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

static command_t command_list[]= {
        { "start",  "Start of course run",              },
        { "int",    "Intermediate time mark",           },
        { "stop",   "End of course run",                },
        { "fail",   "Sensor faillure detected",         },
        { "ok",     "Sensor recovery. Chrono ready",    },
        { "msg",    "Show message on chrono display",   },
        { "down",   "Start 15 seconds countdown",       },
        { "fault",  "Mark fault (+/-/#)",               },
        { "refusal","Mark refusal (+/-/#)",             },
        { "elim",   "Mark elimination [+-]",            },
        { "reset",  "Reset chronometer and countdown",  },
        { "help",   "show command list",                },
        { "exit",   "End program (from console)",       },
        { "server", "Set server IP address",            },
        { "ports",  "Show available serial ports",      },
        { "config", "List configuration parameters",    },
        { NULL,     "",                                 }
};

char **tokenize(char *line, int *argc) {
    char *buff = calloc(1024, sizeof(char));
    *argc = 0;
    char **argv = malloc(sizeof(char *));
    *argv = NULL;
    char *to = buff;
    for (char *pt = line; *pt; pt++) {
        if (!isspace(*pt)) {
            // si no es espacio en blanco lo metemos en el token actual
            *to++ = tolower(*pt);
            continue;
        }
        // si ya hay algun caracter en el buffer, significa que tenemos token
        if (to != &buff[0]) {
            char **tmp = realloc(argv, (1 + *argc) * sizeof(char *));
            // si realloc funciona rellenamos entrada con datos nuevos
            if (tmp) {
                argv[*argc] = strdup(buff);
                *argc = 1+ *argc;
                // argv[*argc]=NULL;
                memset(buff, 0, 1024);
                to = buff;
            }
        }
    }
    free(buff);
    return argv; // *argc contiene el numero de tokens
}

// libera el array de tokens generado por el tokenizer
int freetokens(int argc,char *argv[]) {
    for ( int n=0;n<argc; n++) {
        if (argv[n]) free(argv[n]);
    }
    free(argv);
    return 0;
}


int help( configuration *config, int argc, char *argv[]) {
    if (argc!=1) {
        char *cmd=argv[1]; // get command to obtaing help fromquit
        for ( int n=0; command_list[n].cmd;n++) {
            if (strcmp(command_list[n].cmd,cmd)==0) {
                fprintf(stderr,"\t%s: %s\n",command_list[n].cmd, command_list[n].desc);
                return 0;
            }
        }
        fprintf(stderr,"Command %s not found",cmd);
        return 1;
    } else {
        fprintf(stderr,"List of available commands:");
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
