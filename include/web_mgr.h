//
// Created by jantonio on 5/05/19.
//

#ifndef AGILITYCONTEST_SERIALCHRONOMETER_WEB_MGR_H
#define AGILITYCONTEST_SERIALCHRONOMETER_WEB_MGR_H

#include "../include/sc_config.h"
#ifdef  AGILITYCONTEST_SERIALCHRONOMETER_WEB_MGR_C
#define EXTERN extern
#else
#define EXTERN
#endif

EXTERN int web_manager_thread(configuration *config);
#endif //AGILITYCONTEST_SERIALCHRONOMETER_WEB_MGR_H
