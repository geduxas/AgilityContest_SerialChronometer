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

#define OPMODE_NORMAL 1
#define OPMODE_CONSOLE 2
#define OPMODE_WEB 4
#define OPMODE_SERVER 8
#define OPMODE_FIND 16 /* find serial ports and exit */

typedef struct {
    long long timestamp;
    float elapsed; // last stored elapsed time
    int faults;
    int refusals;
    int eliminated;
    int numero;

} sc_data_t;

typedef struct {
    char *osname; //windows/Linux/Darwin
    int local_port; // UDP Port to listen data from threads

    // debug options
    char *logfile; // log file
    int loglevel;  // log level 0:none 1:panic 2:alert 3:error 4:notice 5:info 6:debug 7:trace 8:all
    int verbose;   // also send logging to stderr 0:no 1:yes

    // AgilityContest server parameters
    char *ajax_server;
    char *client_name;

    // web server parameters
    int web_port;

    // serial port parameters
    char *module; // dll module name ( without .dll/.so )
    int ring;
    char *comm_port;
    int baud_rate;
    int opmode; // OPMODE_ENUM,OPMODE_TEST,OPMODE_NORMAL
    struct sp_port *serial_port; // serial port to be openend

    // chrono status
    sc_data_t status;
} configuration;

#ifdef SERIALCHRONOMETER_SC_CONFIG_C
#define EXTERN extern
#else
#define EXTERN
#endif

EXTERN configuration *default_options(configuration *config);
EXTERN configuration *parse_ini_file(configuration *config, char *filename);
EXTERN void print_configuration(configuration *config);
EXTERN void print_status(configuration *config);
#undef EXTERN

#endif //SERIALCHRONOMETER_SC_CONFIG_H
