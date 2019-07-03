//
// Created by jantonio on 2/06/19.
//

#ifndef SERIALCHRONOMETER_SC_TOOLS_H
#define SERIALCHRONOMETER_SC_TOOLS_H

#include<stdio.h>
#include "../include/sc_config.h"

typedef struct qitem_st {
    time_t expire;
    int index;
    char *msg;
    struct qitem_st *next;
} qitem_t;

typedef struct queue_st {
    char *name;
    int last_index;
    qitem_t *first_out; // first element to fetch on get()
    qitem_t *last_out; // last item inserted in queue with put()
} queue_t;

#ifndef SERIALCHRONOMETER_SC_TOOLS_C
#define EXTERN extern
#else
#define EXTERN
#endif
EXTERN int stripos(char* haystack, char* needle );
EXTERN char **explode(char* line, char separator,int *nelem );
EXTERN long long current_timestamp();
EXTERN char *getSessionName(configuration *config);

/* fifo queue management */
EXTERN queue_t *queue_create(char *name);
EXTERN void queue_destroy(queue_t *queue);
EXTERN qitem_t *queue_put(queue_t*queue, char * msg);
EXTERN char *queue_get(queue_t *queue);
EXTERN char *queue_pick(queue_t *queue,int id);
EXTERN size_t queue_size(queue_t *queue);
EXTERN void queue_expire(queue_t *queue);

#undef EXTERN
#endif //SERIALCHRONOMETER_SC_TOOLS_H
