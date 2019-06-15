//
// Created by jantonio on 5/05/19.
//

#define AGILITYCONTEST_SERIALCHRONOMETER_CONSOLE_MGR_C

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "../include/main.h"
#include "../include/sc_sockets.h"
#include "../include/debug.h"
#include "../include/console_mgr.h"
#include "../include/sc_config.h"
#include "../include/parser.h"

void *console_manager_thread(void *arg){

    int slotIndex= * ((int *)arg);
    sc_thread_slot *slot=&sc_threads[slotIndex];
    configuration *config=slot->config;

    // create sock
    char portstr[16];
    snprintf(portstr,16,"%d",config->local_port);
    slot->sock=connectUDP("localhost",portstr);
    if (slot->sock <0) {
        debug(DBG_ERROR,"Console: Cannot create local socket");
        return NULL;
    }
    // create input buffer
    char *request=calloc(1024,sizeof(char));
    char *response=calloc(1024,sizeof(char));
    if (!request || !response) {
        debug(DBG_ERROR,"Console: Cannot enter interactive mode:calloc()");
        return NULL;
    }
    // loop until end requested
    int res=0;
    sprintf(request,"console ");
    while(res>=0) {
        fprintf(stdout,"cmd> ");
        char *p=fgets(&request[8],1000,stdin);
        if (p) {
            write(slot->sock,request,strlen(request));
            res=read(slot->sock,response,1024);
        }
        else res=-1; // received eof from stdin
    }
    return &slot->index;
}