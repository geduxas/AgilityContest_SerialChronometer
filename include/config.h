//
// Created by jantonio on 5/05/19.
//

#ifndef SERIALCHRONOMETER_CONFIG_H
#define SERIALCHRONOMETER_CONFIG_H
#ifdef __WIN32__
#define PATHSEP '\\'
#else
#define PATHSEP '/'
#endif

typedef struct {
    char *osname;
    // log file
    char *logfile;
    // log level 0:none 1:panic 2:alert 3:error 4:notice 5:info 6:debug 7:trace 8:all
    int loglevel;
    // also send logging to stderr 0:no 1:yes
    int verbose;
    // AgilityContest server parameters
    char *ajax_server;
    int ajax_port;
    // web server parameters
    int web_port;
    // serial port parameters
    char *comm_port;
    int baud_Rate;
} configuration;

extern configuration *default_options(configuration *config);
extern configuration *parse_ini_file(configuration *config, char *filename);
extern void printConfiguration(configuration *config);

#endif //SERIALCHRONOMETER_CONFIG_H
