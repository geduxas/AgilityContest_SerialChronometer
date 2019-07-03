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
queue_t *queue_create(char *name) {
    queue_t *q=calloc(1,sizeof(queue_t));
    if (!q) return NULL;
    q->name=strdup(name);
    return q;
}
void queue_destroy(queue_t *q) {
    if (!q) return;
    qitem_t *pt=q->first_out;
    while(pt) {
        qitem_t *cur=pt;
        free(pt->msg);
        pt=pt->next;
        free(cur);
    }
    free(q->name); // strdup()'d
    free(q);
}

qitem_t *queue_put(queue_t *q,char * msg) {
    if (!q) return NULL;
    if (!msg) return NULL;
    // fprintf(stderr,"(%s) queue_put\n",q->name);
    qitem_t *item=calloc(1,sizeof(qitem_t));
    if (!item) return NULL;
    char *m=strdup(msg); // duplicate string to keep queue without external manipulation
    if (!m) {
        free(item);
        return NULL;
    }
    item->index=q->last_index++;
    item->msg=m;
    item->expire=15000+current_timestamp(); // mark expire in 15 seconds
    item->next=q->last_out;
    q->last_out=item;
    if (!item->next) q->first_out=item; // on empty queue set first to out on new item
    // fprintf(stderr,"(%s) queue put index:%d msg:'%s'\n",q->name,item->index,item->msg);
    return item;
}

char *queue_get(queue_t *q) {
    if (!q) return NULL; // no queue
    if (!q->first_out) return NULL; // empty queue
    // fprintf(stderr,"(%s) queue_get\n",q->name);
    qitem_t *item=q->first_out;
    char *msg=q->first_out->msg;
    q->first_out=q->first_out->next;
    if (!q->first_out) q->last_out=NULL; // on no more elements in queue clean last_out pointer
    // fprintf(stderr,"(%s)queue get index:%d msg:'%s'\n",q->name,item->index,item->msg);
    free(item);
    return msg;
}

// pick requested element from queue. DO NOT remove
// when id=-1 get *first item
// else search for first id greater or equal than requested
char *queue_pick(queue_t *q,int id) {
    if (!q) return NULL; // null
    if (!q->first_out) return NULL; // empty
    // if id < 0 fetch first item to get out
    if ( id<0 ) {
        return strdup(q->first_out->msg);
    }
    // else iterate
    qitem_t *nearest= q->last_out;
    if (nearest->index<id) return NULL; // first element is already older than requested
    for (qitem_t *pt=q->last_out; pt ; pt=pt->next) {
        if (pt->index >= id) nearest=pt;
    }
    // fprintf(stderr,"queue pick(%d) returns index:%d msg:'%s'\n",id,nearest->index,nearest->msg);
    return strdup(nearest->msg);
}

// indexes are allways correlative, so just substract
size_t queue_size(queue_t *q) {
    if (!q) return 0;
    size_t count=0;
    qitem_t *item=q->first_out;
    if (!q->first_out) return 0;
    return 1 + q->last_out->index - q->first_out->index;
}

void queue_expire(queue_t *q) {
    if (!q) return;
    qitem_t *item=q->first_out;
    // retrieve last item
    while ( item ) {
        // not expired, return
        if (item->expire > current_timestamp()) return;
        // fprintf(stderr,"(%s) expire index:%d\n",q->name,item->index);
        // expired: remove content and go for next element
        char *msg= queue_get(q);
        if (msg) free(msg); // don't need message: remove it
        item = q->first_out;
    }
}