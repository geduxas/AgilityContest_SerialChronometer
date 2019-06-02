//
// Created by jantonio on 5/05/19.
//

#define AGILITYCONTEST_SERIALCHRONOMETER_AJAX_MGR_C

#include <main.h>
#include "../include/ajax_mgr.h"
#include "../include/sc_config.h"

void *ajax_manager_thread(void *arg){
    int slotIndex= * ((int *)arg);
    sc_thread_slot *slot=&sc_threads[slotIndex];
    configuration *config=slot->config;
    return &slot->index;
}
