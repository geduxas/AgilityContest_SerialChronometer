//
// Created by jantonio on 25/06/19.
//

#ifndef SERIALCHRONOMETER_AJAX_PARSER_H
#define SERIALCHRONOMETER_AJAX_PARSER_H

#include "sc_config.h"

#ifndef SERIALCHRONOMETER_AJAX_PARSER_C
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
#undef EXTERN
#endif //SERIALCHRONOMETER_AJAX_PARSER_H
