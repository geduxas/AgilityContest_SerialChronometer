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
    config->fire_browser = 1; // default is fire up browser
    config->comm_port = NULL; // must be declared on program invocation or ini file
    config->baud_rate = 9600;
    config->ring = 1;
    config->module="generic";
    config->client_name="generic";
    config->serial_port = NULL;
    config->opmode = 0; //bitmask 1:serial 2:console 4:html 8:agilitycontest
    config->local_port = 8880;
    config->license_file = NULL;
    // internal status tracking
    config->status.eliminated=0;
    config->status.faults=0;
    config->status.touchs=0;
    config->status.refusals=0;
    config->status.start_time=-1L;
    config->status.int_time=0L;
    config->status.stop_time=0;
    config->status.numero=0;

    return config;
}

void print_status(configuration *config) {
    debug(DBG_DEBUG,"Status info:");
    debug(DBG_DEBUG,"Numero %d",      config->status.numero);
    debug(DBG_DEBUG,"Faults %d",    config->status.faults+config->status.touchs);
    debug(DBG_DEBUG,"Refusals %d",    config->status.refusals);
    debug(DBG_DEBUG,"Eliminated %d",      config->status.eliminated);
    if (config->opmode & OPMODE_CONSOLE) {
        fprintf(stderr,"Status information:\n");
        fprintf(stderr,"Numero %d\n",    config->status.numero);
        fprintf(stderr,"Faults %d\n",    config->status.faults+config->status.touchs);
        fprintf(stderr,"Refusals %d\n",    config->status.refusals);
        fprintf(stderr,"Eliminated %d\n",      config->status.eliminated);
    }
}

void print_configuration(configuration *config) {
    debug(DBG_DEBUG,"Configuration parameters:");
    debug(DBG_DEBUG,"osname %s",   config->osname);
    debug(DBG_DEBUG,"license_file %s",   config->license_file);
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
    debug(DBG_DEBUG,"html port %d",  config->web_port);
    debug(DBG_DEBUG,"fire browser %d",  config->fire_browser);
    debug(DBG_DEBUG,"local_port %d",  config->local_port);
    if (config->opmode & OPMODE_CONSOLE) {
        fprintf(stderr,"Configuration parameters:\n");
        fprintf(stderr,"osname %s\n",   config->osname);
        fprintf(stderr,"license_file %s\n",   config->license_file);
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
        fprintf(stderr,"html port %d\n",  config->web_port);
        fprintf(stderr,"fire browser %d\n",  config->fire_browser);
        fprintf(stderr,"local_port %d\n",  config->local_port);
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
    if      (MATCH("Debug",  "logfile"))      config->logfile = strdup(value);
    else if (MATCH("Debug",  "loglevel"))     config->loglevel = atoi(value) % 9; /* 0..8 */
    else if (MATCH("Debug",  "opmode"))       config->opmode = atoi(value) %  0x1F; /* bitmask */
    else if (MATCH("Debug",  "verbose"))      config->verbose = ((atoi(value)!=0)?1:0);
    else if (MATCH("Global", "license_file")) config->license_file = strdup(value);    else if (MATCH("Global", "local_port"))   config->local_port =atoi(value);
    else if (MATCH("Global", "console"))      config->opmode |= ((atoi(value)!=0)?OPMODE_CONSOLE:0);
    else if (MATCH("Global", "ring"))         config->ring = atoi(value); /* default 1 */
    else if (MATCH("Server", "ajax_server"))  config->ajax_server = strdup(value); /* def "localhost" */
    else if (MATCH("Server", "client_name"))  config->client_name = strdup(value); /* def serial module name */
    else if (MATCH("Serial", "module"))       config->module = strdup(value);
    else if (MATCH("Serial", "comm_port"))    config->comm_port = strdup(value);
    else if (MATCH("Serial", "baud_rate"))    config->baud_rate = atoi(value); /* def 9600 */
    else if (MATCH("Web",    "web_port"))     config->web_port = atoi(value); /* def 8080 */
    else if (MATCH("Web",    "fire_browser")) config->opmode |= ((atoi(value)!=0)?OPMODE_BROWSER:0);
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