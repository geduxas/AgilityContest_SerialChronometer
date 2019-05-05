//
// Created by jantonio on 5/05/19.
//

#ifndef AGILITYCONTEST_SERIALCHRONOMETER_MAIN_H
#define AGILITYCONTEST_SERIALCHRONOMETER_MAIN_H

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


EXTERN char *program_name;
#endif //AGILITYCONTEST_SERIALCHRONOMETER_MAIN_H
