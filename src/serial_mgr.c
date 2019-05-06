//
// Created by jantonio on 5/05/19.
//
#define AGILITYCONTEST_SERIALCHRONOMETER_SERIAL_MGR_C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../libserialport/include/libserialport.h"
#include "../include/debug.h"
#include "../include/serial_mgr.h"
#include "../include/sc_config.h"

int serial_manager_thread(configuration *config){
    return 0;
}

/**
 * Enumerate comm ports and compose a string list
 * @param config configuration data
 * @param nports pointer to return number of ports found
 * @return comm port list, or null if no ports found
 */
char ** serial_ports_enumerate(configuration *config,int *nports){
    int i;
    struct sp_port **ports;

    char **portlist=malloc(sizeof(char*));

    enum sp_return error = sp_list_ports(&ports);
    if (error == SP_OK) {
        for (i = 0; ports[i]; i++) {
            char *port=strdup(sp_get_port_name(ports[i]));
            char **tmp = realloc(portlist, (*nports+1) * sizeof(char(*)));
            if (tmp) {
                portlist=tmp;
                portlist[*nports] = port;
                *nports=1+*nports;
                portlist[*nports] = NULL;
                debug(DBG_INFO,"Found port: '%s'\n", port );
            } else {
                debug(DBG_ERROR,"realloc error\n");
            }
        }
        sp_free_port_list(ports);
        return portlist;
    } else {
        debug(DBG_INFO,"No serial devices detected\n");
        return NULL;
    }
}