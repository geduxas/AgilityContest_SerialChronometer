//
// Created by jantonio on 5/05/19.
//

#ifndef AGILITYCONTEST_SERIALCHRONOMETER_WEB_SERVER_H
#define AGILITYCONTEST_SERIALCHRONOMETER_WEB_SERVER_H

#include "../include/sc_config.h"

#ifdef  AGILITYCONTEST_SERIALCHRONOMETER_WEB_SERVER_C
#define EXTERN extern
#else
#define EXTERN
#endif

EXTERN int init_webServer(configuration *config);
#undef EXTERN

#endif //AGILITYCONTEST_SERIALCHRONOMETER_WEB_SERVER_H
