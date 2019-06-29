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