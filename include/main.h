//
// Created by jantonio on 5/05/19.
//

#ifndef AGILITYCONTEST_SERIALCHRONOMETER_MAIN_H
#define AGILITYCONTEST_SERIALCHRONOMETER_MAIN_H

#include <pthread.h>
#include "../include/sc_config.h"

#ifdef AGILITYCONTEST_SERIALCHRONOMETER_MAIN_C
#define EXTERN extern
#else
#define EXTERN
#endif

/* #define min(a,b) ((a)<(b)?(a):(b)) */

#ifdef __WIN32__
// mode for mkdir() has no sense in win32
#define vmkdir(d,m) mkdir((d))
#else
#define vmkdir(d,m) mkdir((d),(m))
#endif

typedef struct {
    char * tname; // thread name
    configuration *config; // pointer to configuration options
    int shouldFinish; // flag to mark thread must die after processing
    pthread_t thread; // where to store pthread_create() info
    int (*sc_thread_entry)(configuration *config);
} sc_thread_slot;

EXTERN int sc_thread_create(sc_thread_slot *slot);
EXTERN int launchAndWait (char *cmd, char *args);
EXTERN void waitForThreads(int numthreads);

EXTERN sc_thread_slot *sc_threads;
EXTERN char *program_name;
#endif //AGILITYCONTEST_SERIALCHRONOMETER_MAIN_H
