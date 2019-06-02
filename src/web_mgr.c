//
// Created by jantonio on 5/05/19.
//

#define AGILITYCONTEST_SERIALCHRONOMETER_WEB_MGR_C

#include "../include/main.h"
#include "../include/web_mgr.h"
#include "../include/sc_config.h"

void *web_manager_thread(void *arg){
    int slotIndex= * ((int *)arg);
    sc_thread_slot *slot=&sc_threads[slotIndex];
    configuration *config=slot->config;

    return &slot->index;
}