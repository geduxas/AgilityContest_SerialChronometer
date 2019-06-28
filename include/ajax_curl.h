//
// Created by jantonio on 26/06/19.
//

#ifndef SERIALCHRONOMETER_AJAX_CURL_H
#define SERIALCHRONOMETER_AJAX_CURL_H

#include <stdlib.h>
#include "sc_config.h"

#ifndef SERIALCHRONOMETER_AJAX_CURL_C
#define EXTERN extern
#else
#define EXTERN
#endif

EXTERN void ajax_find_server(configuration *config);
EXTERN int ajax_connect_server (configuration *config);
EXTERN int ajax_open_session(configuration *config);
EXTERN char ** ajax_wait_for_events(configuration *config, int *evtid, time_t *timestamp);
EXTERN int ajax_put_event(configuration *config, char *type, char *value);

#undef EXTERN
#endif //SERIALCHRONOMETER_AJAX_CURL_H
