//
// Created by jantonio on 5/05/19.
//

#define AGILITYCONTEST_SERIALCHRONOMETER_CONSOLE_MGR_C

#include <stdlib.h>
#include <stdio.h>

#include "../include/main.h"
#include "../include/debug.h"
#include "../include/console_mgr.h"
#include "../include/sc_config.h"
#include "../include/parser.h"

void *console_manager_thread(void *arg){

    int slotIndex= * ((int *)arg);
    sc_thread_slot *slot=&sc_threads[slotIndex];
    configuration *config=slot->config;

    char *buff=calloc(1024,sizeof(char));
    if (!buff) {
        debug(DBG_ERROR,"Cannot enter interactive mode:calloc()");
        return NULL;
    }
    int res=0;
    while(res>=0) {
        fprintf(stdout,"cmd> ");
        char *p=fgets(buff,1023,stdin);
        if (p) res=parse_cmd(config,"console",p);
        else res=-1; // received eof from stdin
    }
    return &slot->index;
}