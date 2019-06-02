//
// Created by jantonio on 27/02/19.
//

#include <string.h>
#include <stdlib.h>

#define SERIALCHRONOMETER_SC_CONFIG_C
#include "sc_config.h"
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
    config->verbose = 1;
    config->ajax_server = strdup("localhost");
    config->web_port = 8080;
    config->comm_port = NULL; // must be declared on program invocation or ini file
    config->baud_rate = 9600;
    config->opmode = OPMODE_NORMAL; // 0:normal 1: test port 2: enumerate ports
    config->serial_port = NULL;
    config->local_port="8887";
    return config;
}

void print_configuration(configuration *config) {
    debug(DBG_DEBUG,"Configuration parameters:");
    debug(DBG_DEBUG,"osname %s",   config->osname);
    debug(DBG_DEBUG,"logfile %s",    config->logfile);
    debug(DBG_DEBUG,"loglevel %d",    config->loglevel);
    debug(DBG_DEBUG,"opmode %d",      config->opmode);
    debug(DBG_DEBUG,"verbose %d",    config->verbose);
    debug(DBG_DEBUG,"ajax_server %s", config->ajax_server);
    debug(DBG_DEBUG,"comm_port %s",  config->comm_port);
    debug(DBG_DEBUG,"baud_rate %d",  config->baud_rate);
    debug(DBG_DEBUG,"web port %d",  config->web_port);
    if (config->opmode==OPMODE_TEST) {
        fprintf(stderr,"Configuration parameters:\n");
        fprintf(stderr,"osname %s\n",   config->osname);
        fprintf(stderr,"logfile %s\n",    config->logfile);
        fprintf(stderr,"loglevel %d\n",    config->loglevel);
        fprintf(stderr,"opmode %d\n",      config->opmode);
        fprintf(stderr,"verbose %d\n",    config->verbose);
        fprintf(stderr,"ajax_server %s\n", config->ajax_server);
        fprintf(stderr,"comm_port %s\n",  config->comm_port);
        fprintf(stderr,"baud_rate %d\n",  config->baud_rate);
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
    else if (MATCH("Debug",  "opmode"))      config->opmode = atoi(value) % 4; /* 0..3 */
    else if (MATCH("Debug",  "verbose"))      config->opmode = (atoi(value)!=0)?1:0;
    else if (MATCH("Config", "ajax_server"))   config->ajax_server = strdup(value); /* def "localhost" */
    else if (MATCH("Config", "comm_port"))   config->comm_port = strdup(value);
    else if (MATCH("Config", "baud_rate"))   config->baud_rate = atoi(value); /* def 9600 */
    else if (MATCH("Config", "web_port"))   config->web_port = atoi(value); /* def 8080 */
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