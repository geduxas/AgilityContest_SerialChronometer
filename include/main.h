//
// Created by jantonio on 5/05/19.
//

#ifndef AGILITYCONTEST_SERIALCHRONOMETER_MAIN_H
#define AGILITYCONTEST_SERIALCHRONOMETER_MAIN_H

#include <pthread.h>
#include "../include/sc_config.h"

/* #define min(a,b) ((a)<(b)?(a):(b)) */

#ifdef __WIN32__
// mode for mkdir() has no sense in win32
#define vmkdir(d,m) mkdir((d))
#else
#define vmkdir(d,m) mkdir((d),(m))
#endif

typedef struct {
    int index; // thread index
    char * tname; // thread name
    configuration *config; // pointer to configuration options
    int shouldFinish; // flag to mark thread must die after processing
    pthread_t thread; // where to store pthread_create() info
    void *(*handler)(void *config); // entry point
    int (*parser)(void *config,int slot,char *tokens[],int ntokens); // entry point for command parser
    int sock; // socket to write data into
} sc_thread_slot;

#ifdef AGILITYCONTEST_SERIALCHRONOMETER_MAIN_C
#define EXTERN extern
#else
#define EXTERN
#endif

EXTERN int sc_thread_create(int index,char *name, configuration *config,void *(*handler)(void *config));
EXTERN int launchAndWait (char *cmd, char *args);
EXTERN void waitForThreads(int numthreads);

EXTERN sc_thread_slot *sc_threads;
EXTERN char *program_name;

#undef EXTERN

#endif //AGILITYCONTEST_SERIALCHRONOMETER_MAIN_H
