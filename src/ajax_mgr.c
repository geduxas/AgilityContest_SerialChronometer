//
// Created by jantonio on 5/05/19.
//

#define AGILITYCONTEST_SERIALCHRONOMETER_AJAX_MGR_C
#include <stdio.h>

#include "../include/main.h"
#include "../include/debug.h"
#include "../include/ajax_mgr.h"
#include "../include/sc_config.h"
#include "../include/sc_sockets.h"

void *ajax_manager_thread(void *arg){
    int slotIndex= * ((int *)arg);
    sc_thread_slot *slot=&sc_threads[slotIndex];
    configuration *config=slot->config;

    // create sock
    char portstr[16];
    snprintf(portstr,16,"%d",config->local_port);
    slot->sock=connectUDP("localhost",portstr);
    if (slot->sock <0) {
        debug(DBG_ERROR,"AjaxMgr: Cannot create local socket");
        return NULL;
    }

    return &slot->index;
}
