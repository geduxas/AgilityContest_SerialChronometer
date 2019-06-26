//
// Created by jantonio on 25/06/19.
//

#define SERIALCHRONOMETER_AJAX_JSON_C
#include <stdio.h>
#include <string.h>

#include "sc_config.h"
#include "debug.h"
#include "ajax_json.h"
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
 * @return last "init" event id for selected ring, 0 on no events; -1 on error
 */
int parse_connect(configuration *config, char *json_str,size_t json_len) {
    json_t mem[1024];
    // get root
    json_t const* root = json_create( json_str, mem, sizeof mem / sizeof *mem );
    if ( !root ) {
        debug(DBG_ERROR,"Error parse_connect::json_create()");
        return -1;
    }

    // check for session active
    char const* total=json_getPropertyValue( root, "total" );
    if (! total) {
        debug(DBG_ERROR,"parse_connect::getPropertyValue(total): invalid received json response %s",json_str);
        return -1;
    }
    if (strcmp("0",total)==0) {
        debug(DBG_NOTICE,"parse_connect::getPropertyValue(total): no 'init' event registered (yet) on ring %d",config->ring);
        return 0;
    }
    // obtenemos el ultimo event id por el metodo guarro de recorrer la lista y quedarnos con el ultimo evento
    // get rows array
    json_t const* rows = json_getProperty( root, "rows" );
    if ( !rows || JSON_ARRAY != json_getType( rows ) ) {
        debug(DBG_ERROR,"parse_connect::json_getProperty(rows)");
        return -1;
    }
    // iterate over rows to find session id for provided ring
    int eventID=0;
    json_t const* event;
    for( event = json_getChild( rows ); event != 0; event = json_getSibling( event ) ) {
        if ( JSON_OBJ == json_getType( event ) ) {
            char const* eventstr = json_getPropertyValue( event, "ID" );
            debug(DBG_INFO,"Found 'init' event id %s for ring %d",eventstr,config->ring);
            eventID=atoi(eventstr);
        }
    }
    return eventID;
}


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
 * @return list of serial chrono API commands to be executed. null on error, or empty array if no data to send
 */
char **parse_events(configuration *config, char *json_str,size_t json_len, int *evtid, time_t *timestamp) {
    // dirty way to count tokens:
    size_t count=0;
    for (char *pt=json_str;*pt;pt++) if (*pt==':') count++;
    json_t mem[count];
    // get root token
    json_t const* root = json_create( json_str, mem, sizeof mem / sizeof *mem );
    if ( !root ) {
        debug(DBG_ERROR,"Error parse_events::json_create()");
        return NULL;
    }

    // check for session active
    char const* total=json_getPropertyValue( root, "total" );
    if (! total) {
        debug(DBG_ERROR,"parse_events::getPropertyValue(total): invalid received json response %s",json_str);
        return NULL;
    }
    if (strcmp("0",total)==0) {
        debug(DBG_NOTICE,"parse_events::getPropertyValue('total'): no events received since last call on ring %d",config->ring);
    }
    // get rows array
    json_t const* rows = json_getProperty( root, "rows" );
    if ( !rows || JSON_ARRAY != json_getType( rows ) ) {
        debug(DBG_ERROR,"parse_events::json_getProperty('rows')");
        return NULL;
    }
    // creamos un array de comandos de longitud total+1
    char **result= calloc(1+atoi(total),sizeof(char*));
    if (!result) {
        debug(DBG_ERROR,"parse_events::calloc(numrows): cannot allocate space for response");
        return NULL;
    }
    // obtenemos el ultimo event id por el metodo guarro de recorrer la lista y quedarnos con el ultimo evento
    json_t const* event;
    for( event = json_getChild( rows ); event != 0; event = json_getSibling( event ) ) {
        if ( JSON_OBJ == json_getType( event ) ) {
            // cogemos el event ID
            char const* eventstr = json_getPropertyValue( event, "ID" );
            debug(DBG_INFO,"Parsing event id %s on ring %d",eventstr,config->ring);
            // analizamos ahora el campo "Data" para extraer Type,Value y TimeStamp
            char const *data = json_getPropertyValue( event, "Data" );
            if ( !data ) {
                debug(DBG_ERROR,"parse_events::json_getPropertyValue('event/Data') cannot get Data for eventid: %s",eventstr);
                return NULL;
            }
            // field "Data is a string representing a json object. So need to re-parse
            json_t jdata[128];
            json_t const* rdata = json_create( data, jdata, sizeof mem / sizeof *jdata );
            char const* typestr = json_getPropertyValue( rdata, "Type" );
            char const* tsstr = json_getPropertyValue( rdata, "TimeStamp" );
            char const* valuestr = json_getPropertyValue( rdata, "Value" );
            debug(DBG_INFO,"Event ID:%s TimeStamp:%s Type:%s Value:%s",eventstr,tsstr,typestr,valuestr);

            // PENDING: do something usefull with data: fill result array with serial API commands

            // assign values
            *timestamp=atoll(tsstr);
            *evtid=atoi(eventstr);
        }
    }
    return result;
}