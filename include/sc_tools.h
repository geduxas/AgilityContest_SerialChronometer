//
// Created by jantonio on 2/06/19.
//

#ifndef SERIALCHRONOMETER_SC_TOOLS_H
#define SERIALCHRONOMETER_SC_TOOLS_H

#include<stdio.h>
#include "../include/sc_config.h"

#ifndef SERIALCHRONOMETER_SC_TOOLS_C
#define EXTERN extern
#else
#define EXTERN
#endif
EXTERN int stripos(char* haystack, char* needle );
EXTERN char **explode(char* line, char separator,int *nelem );

#undef EXTERN
#endif //SERIALCHRONOMETER_SC_TOOLS_H
