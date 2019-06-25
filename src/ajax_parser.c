//
// Created by jantonio on 25/06/19.
//

#define SERIALCHRONOMETER_AJAX_PARSER_C
#include <stdio.h>
#include <string.h>

#include "sc_config.h"
#include "debug.h"
#include "ajax_parser.h"
#include "jsmn.h"

/**
 * parse a selectring request and extract session for choosen ring
 * @param config configuration parameters
 * @param json_str string to parse from selectring ajax response
 * @param json_str string length ( to avoid re-evaluate strlen )
 * @return session id for selected ring, -1 on error/notfound
 */
int parse_select(configuration *config, char *json_str, size_t json_len){
    char ring[4];
    snprintf(ring,4,"%d",config->ring);
    // get a json parser
    jsmn_parser parser;
    jsmn_init(&parser);

    jsmntok_t tokens[256];
    int ntokens = jsmn_parse(&parser, json_str, json_len, tokens, 256);
    if (ntokens<0) {
        debug(DBG_ERROR,"Error parsing json string %s",json_str);
        return -1;
    }
    for (int i = 0; i != ntokens; ++i)
    {
        jsmntok_t *token = tokens + i;
        char * type = 0;
        switch (token->type)
        {
            case JSMN_PRIMITIVE:
                type = "PRIMITIVE";
                break;
            case JSMN_OBJECT:
                type = "OBJECT";
                break;
            case JSMN_ARRAY:
                type = "ARRAY";
                break;
            case JSMN_STRING:
                type = "STRING";
                break;
            default:
                type = "UNDEF";
        }
#ifdef JSMN_PARENT_LINKS
        printf("node: %4d, %9s, parent: %4d, children: %5d, data:\n%.*s, \n", i, type, token->parent, token->size, token->end-token->start, jsonStr+token->start);
#else
        printf("node: %4d, %9s, children: %5d, data:\n%.*s, \n", i, type, token->size, token->end-token->start, json_str+token->start);
#endif
    }
    return ntokens;
}