//
// Created by jantonio on 5/05/19.
//

#ifndef SERIALCHRONOMETER_SC_CONFIG_H
#define SERIALCHRONOMETER_SC_CONFIG_H
#ifdef __WIN32__
#define PATHSEP '\\'
#else
#define PATHSEP '/'
#endif

#include "libserialport.h"

#define OPMODE_NORMAL 0
#define OPMODE_TEST 1
#define OPMODE_ENUM 2


typedef struct {
    char *osname;
    char *local_port; // UDP Port to listen data from threads
    // log file
    char *logfile;
    // log level 0:none 1:panic 2:alert 3:error 4:notice 5:info 6:debug 7:trace 8:all
    int loglevel;
    // also send logging to stderr 0:no 1:yes
    int verbose;
    // AgilityContest server parameters
    char *ajax_server;
    // web server parameters
    int web_port;
    // serial port parameters
    int ring;
    char *comm_port;
    int baud_rate;
    int opmode; // OPMODE_ENUM,OPMODE_TEST,OPMODE_NORMAL
    struct sp_port *serial_port; // serial port to be openend

} configuration;

#ifdef SERIALCHRONOMETER_SC_CONFIG_C
#define EXTERN extern
#else
#define EXTERN
#endif

EXTERN configuration *default_options(configuration *config);
EXTERN configuration *parse_ini_file(configuration *config, char *filename);
EXTERN void print_configuration(configuration *config);
#undef EXTERN

#endif //SERIALCHRONOMETER_SC_CONFIG_H
