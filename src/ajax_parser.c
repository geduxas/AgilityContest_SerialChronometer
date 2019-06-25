//
// Created by jantonio on 25/06/19.
//

#define SERIALCHRONOMETER_AJAX_PARSER_C
#include <stdio.h>
#include <string.h>

#include "sc_config.h"
#include "debug.h"
#include "ajax_parser.h"
#include "tiny-json.h"

/**
 * parse a selectring request and extract session for choosen ring
 * @param config configuration parameters
 * @param json_str string to parse from selectring ajax response
 * @param json_str string length ( to avoid re-evaluate strlen )
 * @return session id for selected ring, -1 on error/notfound
 */
int parse_select(configuration *config, char *json_str, size_t json_len){
    char ring[16];
    snprintf(ring,16,"Ring %d",config->ring);

    json_t mem[1024];
    // get root
    json_t const* root = json_create( json_str, mem, sizeof mem / sizeof *mem );
    if ( !root ) {
        debug(DBG_ERROR,"Error parse_select::json_create()");
        return -1;
    }
    // get rows array
    json_t const* rows = json_getProperty( root, "rows" );
    if ( !rows || JSON_ARRAY != json_getType( rows ) ) {
        debug(DBG_ERROR,"parse_select::json_getProperty(rows)");
        return -1;
    }
    // iterate over rows to find session id for provided ring
    json_t const* session;
    for( session = json_getChild( rows ); session != 0; session = json_getSibling( session ) ) {
        if ( JSON_OBJ == json_getType( session ) ) {
            char const* ringNumber = json_getPropertyValue( session, "Nombre" );
            if (strcmp(ringNumber,ring)!=0) continue;
            char const* sessionID=json_getPropertyValue( session, "ID" );
            debug(DBG_INFO,"Found Sesion id %s for %s",sessionID,ring);
            return atoi(sessionID);
        }
    }
    // arriving here means session "Ring X" not found
    debug(DBG_ERROR,"Session '%s' not found in selectring() ajax response",ring);
    return -1;
}

/**
 * Parse a connect request and extract last init eventID and timestamp for choosen ring
 * Also fill working data info
 * @param config configuration parameters
 * @param json_str string to parse
 * @param json_len string length
 */
int parse_connect(configuration *config, char *json_str,size_t json_len) {
    json_t mem[1024];
    // get root
    json_t const* root = json_create( json_str, mem, sizeof mem / sizeof *mem );
    if ( !root ) {
        debug(DBG_ERROR,"Error parse_select::json_create()");
        return -1;
    }
    return 0;
}