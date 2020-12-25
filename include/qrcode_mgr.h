//
// Created by jantonio on 5/05/19.
//

#ifndef AGILITYCONTEST_SERIALCHRONOMETER_QRCODE_MGR_H
#define AGILITYCONTEST_SERIALCHRONOMETER_QRCODE_MGR_H

#include "../include/sc_config.h"
#ifdef  AGILITYCONTEST_SERIALCHRONOMETER_QRCODE_MGR_C
#define EXTERN extern
#else
#define EXTERN
#endif

EXTERN void *qrcode_manager_thread(void *config);

#undef EXTERN

#endif //AGILITYCONTEST_SERIALCHRONOMETER_QRCODE_MGR_H
