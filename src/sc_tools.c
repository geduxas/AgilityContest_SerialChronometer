//
// Created by jantonio on 2/06/19.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>

#define SERIALCHRONOMETER_SC_TOOLS_C

#include "../include/sc_config.h"
#include "sc_tools.h"

int stripos(char *haystack, char *needle) {
    char *ptr1, *ptr2, *ptr3;
    if( haystack == NULL || needle == NULL)  return -1;
    for (ptr1 = haystack; *ptr1; ptr1++)    {
        for (ptr2 = ptr1, ptr3 = needle;
            *ptr3 && (toupper(*ptr2) == toupper(*ptr3));
            ptr2++, ptr3++
        ); // notice ending loop
        if (!*ptr3) return ptr1 - haystack + 1;
    }
    return -1;
}

char **explode(char *line, char separator,int *nelem) {
    char *buff = calloc(1+strlen(line), sizeof(char));
    strncpy(buff,line,strlen(line)); // copy string into working space
    char **res=calloc(32,sizeof(char *));
    *nelem=0;
    char *from=buff;
    for (char *to = buff; *to; to++) { // while not end of string
        if (*to != separator) continue; // not separator; next char
        *to = '\0';
        res[*nelem] = from;
        // point to next token to explode (or null at end)
        from = to+1;
        *nelem= 1+*nelem;
        if (*nelem>32) return res;
    }
    // arriving here means end of line. store and return
    res[*nelem]=from;
    *nelem= 1+*nelem;
    return res;
}

/* get current timestamp in miliseconds */
long long current_timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}

char * getSessionName(configuration *config) {
    static char *name=NULL;
    if (!name) name =calloc(1024,sizeof(char));
    snprintf(name,1024,"chrono:%d:0:0:%s",config->status.sessionID,config->client_name);
    return name;
}

/*************************************** fifo queue management */
queue_t *queue_create() {
    return calloc(1,sizeof(queue_t));
}
void queue_destroy(queue_t *q) {
    if (!q || !q->first) return;
    while (q->first!=q->last) {
        free(q->first->msg);
        q->first=q->first->next;
        free(q->first);
    }
}
qitem_t *queue_put(queue_t *q,char * msg) {
    qitem_t *item=calloc(1,sizeof(qitem_t));
    if (!item) return NULL;
    char *m=strdup(msg); // duplicate string to keep queue without external manipulation
    if (!m) {
        free(item);
        return NULL;
    }
    item->msg=m;
    item->expire=15000+current_timestamp(); // mark expire in 15 seconds
    if (!q->last) { // empty queue
        item->id=0;
        q->first=item;
        q->last=item;
    } else {
        item->id=1+q->last->id;
        q->last->next=item;
        q->last=item;
    }
    return item;
}

char *queue_get(queue_t *q) {
    char *msg=NULL;
    qitem_t *item=q->first;
    if (!item) return NULL;
    msg=item->msg;
    q->first=item->next;
    if (!q->first) q->last=NULL; // last item
    free(item);
    return msg;
}

// pick requested element from queue. DO NOT remove
// when id=-1 get *first item
// else search for first id greater or equal than requested
char *queue_pick(queue_t *q,int id) {
    qitem_t *last=NULL;
    if (!q->last) return NULL;
    if (id<0) return strdup(q->first->msg); // use strdup to avoid external manipulation
    for (qitem_t *pt=q->last; pt ; pt=pt->next) {
        last=pt;
        if (last->id<id) return NULL;
        if (last->id == id) return strdup(last->msg);
    }
    if (last) return strdup(last->msg);
    return NULL;
}

size_t queue_size(queue_t *q) {
    if (!q) return 0;
    size_t count=0;
    qitem_t *item=q->first;
    if (!q->first) return 0;
    return 1 + q->last->id - q->first->id;
}

void queue_expire(queue_t *q) {
    if (!q) return;
    qitem_t *item=q->first;
    // retrieve last item
    while ( item ) {
        // not expired, return
        if (item->expire > current_timestamp()) return;
        // expired: remove content and go for next element
        char *msg= queue_get(q);
        if (msg) free(msg); // don't need message: remove it
        item = q->first;
    }
}