//
// Created by jantonio on 2/06/19.
//
#include <stdio.h>
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