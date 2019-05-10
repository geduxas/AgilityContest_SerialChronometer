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
    *portlist=NULL;

    enum sp_return error = sp_list_ports(&ports);
    if (error == SP_OK) {
        for (i = 0; ports[i]; i++) {
            char **tmp=realloc(portlist, (i+1) * sizeof(char *));
            if (tmp) {
                portlist=tmp;
                portlist[i] = strdup(sp_get_port_name(ports[i]));
                debug(DBG_INFO,"Found port: '%s'\n", portlist[i] );
                // *(portlist+i+1)=NULL;
            }
        }
        *nports=i;
        sp_free_port_list(ports);
        return portlist;
    } else {
        debug(DBG_INFO,"No serial devices detected\n");
        return NULL;
    }
}