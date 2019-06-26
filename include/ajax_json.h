//
// Created by jantonio on 25/06/19.
//

#ifndef SERIALCHRONOMETER_AJAX_JSON_H
#define SERIALCHRONOMETER_AJAX_JSON_H

#include <stdlib.h>
#include "sc_config.h"

#ifndef SERIALCHRONOMETER_AJAX_JSON_C
#define EXTERN extern
#else
#define EXTERN
#endif

/**
 * parse a selectring request and extract session for choosen ring
 * @param config configuration parameters
 * @param json_str string to parse
 * @param json_len string length
 */
EXTERN int parse_select(configuration *config, char *json_str,size_t json_len);

/**
 * parse a connect request and extract last init eventID and timestamp for choosen ring
 * @param config configuration parameters
 * @param json_str string to parse
 * @param json_len string length
 */
EXTERN int parse_connect(configuration *config, char *json_str,size_t json_len);


/**
 * parse getEvents request
 *
 * process each received event for requested session/ring
 * Generate serial_chrono API request according received data
 * extract last init eventID and timestamp for choosen ring
 * @param config configuration parameters
 * @param json_str string to parse
 * @param json_len string length
 * @param evtid pointer to store last event id
 * @param timestamp pointer to store last event timestamp
 * @return list of serial chrono API commands to be executed
 */
EXTERN char **parse_events(configuration *config, char *json_str,size_t json_len, int *evtid, time_t *timestamp);

#undef EXTERN
#endif //SERIALCHRONOMETER_AJAX_JSONR_H
