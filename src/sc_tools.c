//
// Created by jantonio on 2/06/19.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SERIALCHRONOMETER_SC_TOOLS_C

#include "../include/sc_config.h"
#include "sc_tools.h"

int stripos(char* haystack, char* needle ) {
    int offset=-1;
    char*needle1;
    int pos = 0, temp;
    if(haystack && needle && *needle!='\0') {
        while(*haystack != '\0') {
            needle1 = needle;
            pos++;
            temp = 0;
            if((*haystack == *needle1) || (*haystack == *needle1 - 32) || (*haystack == *needle1 + 32)) {
                offset = pos;
                needle1++;
                haystack++;
                while(*needle1 != '\0') {
                    if((*haystack == *needle1) || (*haystack == *needle1 - 32) || (*haystack == *needle1 + 32)) {
                        haystack++;
                        needle1++;
                        pos++;
                    } else {
                        temp = 1;
                        offset = -1;
                        break;
                    }
                }
            }
            if(offset > 0) {
                return offset;
            }
            if(temp != 1)
                haystack++;
        }
    }
    return offset;
}

char **explode(char *line, char separator,int *nelem) {
    char *buff = calloc(1+strlen(line), sizeof(char));
    strncpy(buff,line,strlen(line)); // copy string into working space
    char **res=NULL;
    *nelem=0;
    char *from=buff;
    for (char *to = buff; *to; to++) { // while not end of string
        if (*to != separator) continue; // not separator; next char
        *to = '\0';
        res[*nelem] = from;
        from = to+1; // point to next token to explode (or null at end)
        (*nelem)++;
        res = realloc(res, (*nelem) * sizeof(char *));
    }
    return res;
}
