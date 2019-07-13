//
// Created by jantonio on 13/07/19.
//

#ifndef SERIALCHRONOMETER_LICENSE_H
#define SERIALCHRONOMETER_LICENSE_H

#include "sc_config.h"

#ifdef __WIN32__
#define LICENSE_FILE "C:\\AgilityContest\\config\\registration.info"
#elif defined __unix__
#define LICENSE_FILE "/var/www/html/AgilityContest/config/registration.info"
#elif defined __APPLE__
#define LICENSE_FILE "/Applications/XAMMP/htdocs/AgilityContest/config/registration.info"
#endif

#ifndef SERIALCHRONOMETER_LICENSE_C
#define EXTERN extern
#else
#define EXTERN
#endif
EXTERN char * getLicenseFromFile(configuration *config);

#undef EXTERN

#endif //SERIALCHRONOMETER_LICENSE_H
