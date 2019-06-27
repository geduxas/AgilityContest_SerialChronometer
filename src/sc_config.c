//
// Created by jantonio on 27/02/19.
//

#include <string.h>
#include <stdlib.h>

#define SERIALCHRONOMETER_SC_CONFIG_C
#include "sc_config.h"
#include "sc_tools.h"
#include "debug.h"
#include "ini.h"

/**
 * set default options
 * @return 0
 */
configuration * default_options(configuration * config) {
    if (!config) config=calloc(1,sizeof (configuration));
    if (!config) {
        debug(DBG_ALERT,"Cannot allocate space for configuration data");
        return NULL;
    }
#ifdef __WIN32__
    config->osname = strdup("Windows");
    config->logfile = strdup(".\\serial_chrono.log");
#else
    config->osname = strdup("Linux");
    config->logfile = strdup("./serial_chrono.log");
#endif
    config->loglevel = 3;
    config->verbose = 0;
    config->ajax_server = strdup("localhost");
    config->web_port = 8080;
    config->comm_port = NULL; // must be declared on program invocation or ini file
    config->baud_rate = 9600;
    config->ring = 1;
    config->module="generic";
    config->client_name="generic";
    config->serial_port = NULL;
    config->opmode = 0; //bitmask 1:serial 2:console 4:web 8:agilitycontest
    config->local_port = 8880;
    // internal status tracking
    config->status.eliminated=0;
    config->status.faults=0;
    config->status.refusals=0;
    config->status.timestamp=-1L;
    config->status.dorsal=0;
    config->status.elapsed=0.0f;
    return config;
}

void print_status(configuration *config) {
    debug(DBG_DEBUG,"Status info:");
    debug(DBG_DEBUG,"Dorsal %d",      config->status.dorsal);
    debug(DBG_DEBUG,"Timestamp %lld",   config->status.timestamp);
    debug(DBG_DEBUG,"LastTime %f",   config->status.elapsed);
    debug(DBG_DEBUG,"Faults %d",    config->status.faults);
    debug(DBG_DEBUG,"Refusals %d",    config->status.refusals);
    debug(DBG_DEBUG,"Eliminated %d",      config->status.eliminated);
    if (config->opmode & OPMODE_CONSOLE) {
        fprintf(stderr,"Status information:\n");
        fprintf(stderr,"Dorsal %d\n",    config->status.dorsal);
        fprintf(stderr,"Timestamp %lld\n",   config->status.timestamp);
        fprintf(stderr,"LastTime %f\n",   config->status.elapsed);
        if (config->status.timestamp>=0) {
            float elapsed=(float)(current_timestamp()-config->status.timestamp)/1000.0;
            fprintf(stderr,"Elapsed Time %f\n",elapsed);
        }
        fprintf(stderr,"Faults %d\n",    config->status.faults);
        fprintf(stderr,"Refusals %d\n",    config->status.refusals);
        fprintf(stderr,"Eliminated %d\n",      config->status.eliminated);
    }
}

void print_configuration(configuration *config) {
    debug(DBG_DEBUG,"Configuration parameters:");
    debug(DBG_DEBUG,"osname %s",   config->osname);
    debug(DBG_DEBUG,"logfile %s",    config->logfile);
    debug(DBG_DEBUG,"loglevel %d",    config->loglevel);
    debug(DBG_DEBUG,"opmode %d",      config->opmode);
    debug(DBG_DEBUG,"verbose %d",    config->verbose);
    debug(DBG_DEBUG,"ajax_server %s", config->ajax_server);
    debug(DBG_DEBUG,"client_name %s", config->client_name);
    debug(DBG_DEBUG,"module %s",    config->module);
    debug(DBG_DEBUG,"comm_port %s",  config->comm_port);
    debug(DBG_DEBUG,"baud_rate %d",  config->baud_rate);
    debug(DBG_DEBUG,"ring %d",  config->ring);
    debug(DBG_DEBUG,"web port %d",  config->web_port);
    if (config->opmode & OPMODE_CONSOLE) {
        fprintf(stderr,"Configuration parameters:\n");
        fprintf(stderr,"osname %s\n",   config->osname);
        fprintf(stderr,"logfile %s\n",    config->logfile);
        fprintf(stderr,"loglevel %d\n",    config->loglevel);
        fprintf(stderr,"opmode %d\n",      config->opmode);
        fprintf(stderr,"verbose %d\n",    config->verbose);
        fprintf(stderr,"ajax_server %s\n", config->ajax_server);
        fprintf(stderr,"client_name %s\n", config->client_name);
        fprintf(stderr,"module %s\n",  config->module);
        fprintf(stderr,"comm_port %s\n",  config->comm_port);
        fprintf(stderr,"baud_rate %d\n",  config->baud_rate);
        fprintf(stderr,"ring %d\n",  config->ring);
        fprintf(stderr,"web port %d\n",  config->web_port);
    }
}

/**
 * handle configuration file
 * @param data configuration pointer (not used, seems that library is buggy)
 * @param section Section name
 * @param name Entry key
 * @param value Entry value
 * @return 1 on success; 0 on error
 */
static int handler(void * data, const char* section, const char* name, const char* value) {
    configuration *config=(configuration*)data;
#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if      (MATCH("Debug",  "logfile"))     config->logfile = strdup(value);
    else if (MATCH("Debug",  "loglevel"))    config->loglevel = atoi(value) % 9; /* 0..8 */
    else if (MATCH("Debug",  "opmode"))      config->opmode = atoi(value) %  0x1F; /* bitmask */
    else if (MATCH("Debug",  "console"))     config->opmode |= ((atoi(value)!=0)?OPMODE_CONSOLE:0);
    else if (MATCH("Debug",  "verbose"))     config->verbose = ((atoi(value)!=0)?1:0);
    else if (MATCH("Server", "ajax_server")) config->ajax_server = strdup(value); /* def "localhost" */
    else if (MATCH("Server", "client_name")) config->client_name = strdup(value); /* def serial module name */
    else if (MATCH("Server", "ring"))       config->ring = atoi(value); /* default 1 */
    else if (MATCH("Serial", "module"))     config->module = strdup(value);
    else if (MATCH("Serial", "comm_port"))   config->comm_port = strdup(value);
    else if (MATCH("Serial", "baud_rate"))   config->baud_rate = atoi(value); /* def 9600 */
    else if (MATCH("Web", "web_port"))   config->web_port = atoi(value); /* def 8080 */
    else return 0; /* unknown section/name, error */
    return 1;
}

configuration * parse_ini_file(configuration *config, char *filename) {
    if (!config) config=default_options(NULL);
    if (!config) {
        debug(DBG_ALERT,"Cannot retrieve default configuration data");
        return NULL;
    }
    if (ini_parse(filename, handler, config) < 0) {
        debug(DBG_ERROR,"Can't load 'serial_chrono.ini'. Using in-built configuration");
    }
    return config;
}