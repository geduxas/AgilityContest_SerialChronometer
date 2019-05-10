//
// Created by jantonio on 5/05/19.
//

#ifndef AGILITYCONTEST_SERIALCHRONOMETER_SERIAL_MGR_H
#define AGILITYCONTEST_SERIALCHRONOMETER_SERIAL_MGR_H


#include "../include/sc_config.h"
#ifdef  AGILITYCONTEST_SERIALCHRONOMETER_SERIAL_MGR_C
#define EXTERN extern
#else
#define EXTERN
#endif

EXTERN int serial_manager_thread(configuration *config);
EXTERN char ** serial_ports_enumerate(configuration *config, int *nports);
EXTERN void serial_print_ports(configuration *config);

#undef EXTERN

#endif //AGILITYCONTEST_SERIALCHRONOMETER_SERIAL_MGR_H
