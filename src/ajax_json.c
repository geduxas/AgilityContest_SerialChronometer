//
// Created by jantonio on 25/06/19.
//

#define SERIALCHRONOMETER_AJAX_JSON_C
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "sc_config.h"
#include "debug.h"
#include "ajax_json.h"
#include "tiny-json.h"

#define MSG_LEN 256

static time_t start_time; // used to handle start/stop events

static char * process_eventData(configuration *config,char const * datastr, int *eventID, time_t *timestamp) {
    char *data=calloc(strlen(datastr),sizeof(char));
    memcpy(data,datastr,strlen(datastr));
    json_t jdata[128];
    json_t const* rdata = json_create( data, jdata, sizeof jdata / sizeof *jdata );
    char const* typestr = json_getPropertyValue( rdata, "Type" );
    char const* tsstr = json_getPropertyValue( rdata, "TimeStamp" );
    if (tsstr) *timestamp=atoll(tsstr);
    char const* valuestr = json_getPropertyValue( rdata, "Value" );
    char const* startstr = json_getPropertyValue( rdata, "start" ); // lowercase
    debug(DBG_INFO,"Event ID:%sType:%s Value:%s",datastr,typestr,valuestr);
    // PENDING: do something usefull with data: fill result array with serial API commands
    char *result=calloc(MSG_LEN,sizeof(char));
    if (!result) {
        debug(DBG_ERROR,"process_eventData::calloc() cannot reserve space for SerialAPI command");
        return NULL;
    }
    if ( strcmp(typestr,"null")==0) { // null event
        // no action
    } else if ( strcmp(typestr,"init")==0) { // connect to session
        // no action. ( PENDING: what about sending reset? )
    } else if ( strcmp(typestr,"login")==0) { // user login
        // no action
    } else if ( strcmp(typestr,"open")==0) { // tanda selection
        // no action
    } else if ( strcmp(typestr,"close")==0) { // tanda exit
        // no action
    } else if ( strcmp(typestr,"salida")==0) { // 15 seconds countdown
        snprintf(result,MSG_LEN,"DOWN 15\n");
    } else if ( strcmp(typestr,"start")==0) { // manual chrono start. Value= timestamp
        start_time=atoll(valuestr);
        snprintf(result,MSG_LEN,"START 0\n");
    } else if ( strcmp(typestr,"stop")==0) { // manual chrono stop. Value=timestamp
        time_t stop_time=atoll(valuestr);
        snprintf(result,MSG_LEN,"STOP %lu\n",stop_time-start_time);
    } else if ( strcmp(typestr,"crono_start")==0) { // electronic chrono start Value=timestamp
        start_time=atoll(valuestr);
        snprintf(result,MSG_LEN,"START 0\n");
    } else if ( strcmp(typestr,"crono_int")==0) { // electronic chrono intermediate time Value=timestamp
        time_t int_time=atoll(valuestr);
        snprintf(result,MSG_LEN,"INT %lu\n",int_time-start_time);
    } else if ( strcmp(typestr,"crono_stop")==0) {  // electronic chrono start Value=timestamp
        time_t stop_time=atoll(valuestr);
        snprintf(result,MSG_LEN,"STOP %lu\n",stop_time-start_time);
    } else if ( strcmp(typestr,"crono_rec")==0) { // course walk. start=seconds
        time_t seconds= (startstr)?atoll(startstr):0L;
        snprintf(result,MSG_LEN,"WALK %lu\n",seconds);
    } else if ( strcmp(typestr,"crono_dat")==0) { // dog data FRE
        char const *flt=json_getPropertyValue( rdata, "Flt" );
        char const *toc=json_getPropertyValue( rdata, "Toc" );
        char const *reh=json_getPropertyValue( rdata, "Reh" );
        char const *eli=json_getPropertyValue( rdata, "Eli" );
        char const *npr=json_getPropertyValue( rdata, "NPr" );
        int f=(strcmp(flt,"-1")==0)?config->status.faults:atoi(flt);
        int t=(strcmp(toc,"-1")==0)?0:atoi(toc);
        int r=(strcmp(reh,"-1")==0)?config->status.refusals:atoi(reh);
        int e=(strcmp(eli,"-1")==0)?config->status.eliminated:atoi(eli);
        snprintf(result,MSG_LEN,"DATA %d:%d:%d\n",f+t,r,e);
    } else if ( strcmp(typestr,"crono_restart")==0) { // swicth crono from manual to electronic
        // no action: previous manual start remains active
    } else if ( strcmp(typestr,"crono_reset")==0) { // reset crono and dog data
        snprintf(result,MSG_LEN,"RESET\n");
    } else if ( strcmp(typestr,"crono_error")==0) { // sensor error
        snprintf(result,MSG_LEN,"FAIL\n");
    } else if ( strcmp(typestr,"crono_ready")==0) { // sensor recovery
        snprintf(result,MSG_LEN,"OK\n");
    } else if ( strcmp(typestr,"llamada")==0) { // Call dog to enter in ring
        // no action
    } else if ( strcmp(typestr,"datos")==0) { // manual dog data
        char const *flt=json_getPropertyValue( rdata, "Flt" );
        char const *toc=json_getPropertyValue( rdata, "Toc" );
        char const *reh=json_getPropertyValue( rdata, "Reh" );
        char const *eli=json_getPropertyValue( rdata, "Eli" );
        char const *npr=json_getPropertyValue( rdata, "NPr" );
        int f=(strcmp(flt,"-1")==0)?config->status.faults:atoi(flt);
        int t=(strcmp(toc,"-1")==0)?0:atoi(toc);
        int r=(strcmp(reh,"-1")==0)?config->status.refusals:atoi(reh);
        int e=(strcmp(eli,"-1")==0)?config->status.eliminated:atoi(eli);
        snprintf(result,MSG_LEN,"DATA %d:%d:%d\n",f+t,r,e);
    } else if ( strcmp(typestr,"aceptar")==0) { // validate dog data
        // no action
    } else if ( strcmp(typestr,"cancelar")==0) { // reset dog data
        // no action
    } else if ( strcmp(typestr,"info")==0) { // informational event. no action required
        debug(DBG_NOTICE,"ajaxmgr: received event info '%s'",valuestr);
    } else if ( strcmp(typestr,"user")==0) { // user defined event
        // no action
    } else if ( strcmp(typestr,"command")==0) { // miscelaneous commands Value
        // only allowed command is Oper:7: Value: seconds:msg
        char const* operstr = json_getPropertyValue( rdata, "Oper" );
        if (strcmp("8",operstr)==0) {
            char *secs=strdup(valuestr);
            char *msg=strchr(secs,':');
            if (msg) *msg++='\0';
            snprintf(result,MSG_LEN,"MSG %s %s\n",secs,msg);
            free(secs);
        }
    } else if ( strcmp(typestr,"camera")==0) { // switch camera sources for session
        // no action
    } else if ( strcmp(typestr,"reconfig")==0) { // server reconfiguration
        // no action
    } else { // unknown event. notify and continue
        debug(DBG_ERROR,"ajaxmgr: received unknown event type '%s'",typestr);
    }
    if (strcmp(result,"")==0) {
        free(result);
        result=NULL;
    }
    return result;
}

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
    int evt_count=0;
    // obtenemos el ultimo event id por el metodo guarro de recorrer la lista y quedarnos con el ultimo evento
    for( json_t const *event = json_getChild( rows ); event != 0; event = json_getSibling( event ) ) {
        if ( JSON_OBJ == json_getType( event ) ) {
            // cogemos el event ID
            char const* eventstr = json_getPropertyValue( event, "ID" );
            debug(DBG_INFO,"Parsing event id %s on ring %d",eventstr,config->ring);
            *evtid=atoi(eventstr);
            // analizamos ahora el campo "Data" para extraer Type,Value y TimeStamp
            char const *data = json_getPropertyValue( event, "Data" );
            if ( !data ) {
                // PENDING: study if exists events with no Data and how to handle them
                debug(DBG_ERROR,"parse_events::json_getPropertyValue('event/Data') cannot get Data for eventid: %s",eventstr);
                continue;
            }
            // field "Data is a string representing a json object
            result[evt_count]=process_eventData(config,data,evtid,timestamp);
            if (result[evt_count]!=NULL) evt_count++;
        }
    }
    return result;
}
