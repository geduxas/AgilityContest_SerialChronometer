//
// Created by jantonio on 5/05/19.
//
#define AGILITYCONTEST_SERIALCHRONOMETER_SERIAL_MGR_C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/main.h"
#include "../libserialport/include/libserialport.h"
#include "../include/debug.h"
#include "../include/serial_mgr.h"
#include "../include/sc_config.h"
#include "../include/sc_sockets.h"

void *serial_manager_thread(void *arg){

    int slotIndex= * ((int *)arg);
    sc_thread_slot *slot=&sc_threads[slotIndex];
    configuration *config=slot->config;

    // create sock
    char portstr[16];
    snprintf(portstr,16,"%d",config->local_port);
    slot->sock=connectUDP("localhost",portstr);
    if (slot->sock <0) {
        debug(DBG_ERROR,"SerialMgr: Cannot create local socket");
        return NULL;
    }

    char *errmsg=NULL;
    enum sp_return ret=sp_get_port_by_name(config->comm_port,&config->serial_port);
    if (ret == SP_OK) {
        ret = sp_open(config->serial_port,SP_MODE_READ_WRITE);
        if (ret == SP_OK) {
            sp_set_baudrate(config->serial_port, config->baud_rate);
        } else {
            errmsg="Cannot open serial port %s";
        }
    } else {
        errmsg="Cannot locate serial port %s";
    }
    if (errmsg) {
        // on error show message and disable comm port and associated thread handling
        debug(DBG_ERROR,errmsg,config->comm_port);
        if (config->serial_port) {
            sp_free_port(config->serial_port);
            config->serial_port=NULL;
        }
        config->comm_port=NULL;
        return NULL;
    }
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

void serial_print_ports(configuration *config) {
    int nports=0;
    char **ports= serial_ports_enumerate(config,&nports);
    if (nports==0) {
        fprintf(stdout,"No available COMM ports found:\n");
    } else {
        fprintf(stdout,"List of available COMM ports:\n");
        for (int i=0;i<nports;i++) {
            fprintf(stdout,"%s\n",ports[i]);
            free(ports[i]);
        }
        free(ports);
    }
}

/**
 * Send data to serial port
 * @param config configuration pointer
 * @param data data
 * @return number of bytes sent or -1 on error
 */
int serial_write(configuration *config, char *data) {

}