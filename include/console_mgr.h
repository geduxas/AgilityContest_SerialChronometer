//
// Created by jantonio on 5/05/19.
//

#ifndef AGILITYCONTEST_SERIALCHRONOMETER_CONSOLE_MGR_H
#define AGILITYCONTEST_SERIALCHRONOMETER_CONSOLE_MGR_H

#include "../include/sc_config.h"

#ifdef  AGILITYCONTEST_SERIALCHRONOMETER_CONSOLE_MGR_C
#define EXTERN extern
#else
#define EXTERN
#endif
#define CONSOLE_MGR_VERSION "1.0.0"
EXTERN void * console_manager_thread(void *config);
#undef EXTERN

#endif //AGILITYCONTEST_SERIALCHRONOMETER_CONSOLE_MGR_H
