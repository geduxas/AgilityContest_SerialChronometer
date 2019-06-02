//
// Created by jantonio on 5/05/19.
//

#define AGILITYCONTEST_SERIALCHRONOMETER_WEB_MGR_C

#include "../include/debug.h"
#include "../include/main.h"
#include "../include/web_mgr.h"
#include "../include/sc_config.h"
#include "../include/sc_sockets.h"

void *web_manager_thread(void *arg){
    int slotIndex= * ((int *)arg);
    sc_thread_slot *slot=&sc_threads[slotIndex];
    configuration *config=slot->config;

    // create sock
    slot->sock=connectUDP("localhost",config->local_port);
    if (slot->sock <0) {
        debug(DBG_ERROR,"SerialMgr: Cannot create local socket");
        return NULL;
    }

    return &slot->index;
}