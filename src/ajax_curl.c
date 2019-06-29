//
// Created by jantonio on 26/06/19.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define SERIALCHRONOMETER_AJAX_CURL_C

#include "debug.h"
#include "sc_config.h"
#include "sc_tools.h"
#include "ajax_curl.h"
#include "ajax_json.h"

#define URL_BUFFSIZE 2018
// PENDING rework to be retrieved from configuration
#define BASE_URL "agility"

// listado de sesiones disponibles
// https://{ajax_server}/{baseurl}/ajax/database/sessionFunctions.php
//      ?Operation=selectring
static char sc_selecturl[URL_BUFFSIZE];

// conectarse a una sesion
// https://{ajax_server}/{baseurl}/ajax/database/eventFunctions.php
//      ?Operation=connect&Session={ID}&SessionName={name}
static char sc_connecturl[URL_BUFFSIZE];

// obtener eventos
// https://{ajax_server}/{baseurl}/ajax/database/eventFunctions.php
//      ?Operation=getEvents&Session={ID}&TimeStamp={timestamp}&SessionName={name}
static char sc_geteventurl[URL_BUFFSIZE];

// enviar evento
// https://{ajax_server}/{baseurl}/ajax/database/eventFunctions.php
//      ?Operation={chrono_op}&Session={ID}&TimeStamp={timestamp}&source={source}&SessionName={name}&Value={value}
//      &Faltas={f}&Tocados={t}&Rehuses={r}&Eliminado={e}&NoPresentado={n}
// PENDING: study if need additional info ( prueba,jornada, manga, perro, etc... ) o se obtiene de la sesion
static char sc_puteventurl[URL_BUFFSIZE];

/****************** guarrerias usadas para almacenar la respuesta de curl en un string */
struct string {
    char *ptr;
    size_t len;
};

static void init_string(struct string *s) {
    s->len = 0;
    s->ptr = malloc(s->len+1);
    if (s->ptr == NULL) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    s->ptr[0] = '\0';
}

static void end_string(struct string *s) {
    s->len=0;
    free(s->ptr);
    s->ptr=NULL;
}

static size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
    size_t new_len = s->len + size*nmemb;
    s->ptr = realloc(s->ptr, new_len+1);
    if (s->ptr == NULL) {
        fprintf(stderr, "realloc() failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(s->ptr+s->len, ptr, size*nmemb);
    s->ptr[new_len] = '\0';
    s->len = new_len;

    return size*nmemb;
}

/******************************** llamadas al servidor ********************/

void ajax_find_server(configuration *config) {
    // obtenemos las interfaces "localhost" y de red con dirección IPv4
    // en cada interfaz iteramos
    // si es localhost, solo se itera la 127.0.0.1
    // PENDING. esta funcion implica implementar "ifconfig", lo cual es demasiado lio para una primera version
}

/**
 * config->ajax_server contains url
// https://{ajax_server}/{baseurl}/ajax/database/sessionFunctions.php
//      ?Operation=selectring
 * @return
 */
int ajax_connect_server (configuration *config) {

    CURL *curl=curl_easy_init();
    struct string s;
    init_string(&s);

    snprintf(sc_selecturl,URL_BUFFSIZE,"https://%s/%s/ajax/database/sessionFunctions.php?Operation=selectring",config->ajax_server,BASE_URL);
    debug(DBG_TRACE,"Connecting server at '%s'",config->ajax_server);
    curl_easy_setopt(curl, CURLOPT_URL, sc_selecturl);
    /* example.com is redirected, so we tell libcurl to follow redirection */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_CAINFO, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

    /* Perform the request, res will get the return code */
    int res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK) {
        debug(DBG_ERROR, "curl_easy_perform(select) failed: %s", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return -1;
    }
    debug(DBG_DEBUG,"SelectRing() returns: \n%s",s.ptr);

    // now, get sessionID for current ring from json response
    int ses= parse_select(config,s.ptr,s.len);
    if (ses<0) {
        debug(DBG_ERROR, "There is no session ID for ring: %s", config->ring);
    }
    /* always cleanup */
    end_string(&s);
    curl_easy_cleanup(curl);
    return ses;
}

// conectarse a una sesion
// https://{ajax_server}/{baseurl}/ajax/database/eventFunctions.php
//      ?Operation=connect&Session={ID}&SessionName={name}
int ajax_open_session(configuration *config) {

    CURL *curl=curl_easy_init();
    struct string s;
    init_string(&s);

    char *baseurl="agility"; // PENDING define in configuration
    snprintf(sc_connecturl,
             URL_BUFFSIZE,
             "https://%s/%s/ajax/database/eventFunctions.php?Operation=connect&Session=%d&SessionName=%s",
             config->ajax_server,BASE_URL,config->status.sessionID,getSessionName(config));
    debug(DBG_TRACE,"ConnectSession: %s",sc_connecturl);
    curl_easy_setopt(curl, CURLOPT_URL, sc_connecturl);
    /* example.com is redirected, so we tell libcurl to follow redirection */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_CAINFO, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

    /* Perform the request, res will get the return code */
    int res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK) {
        debug(DBG_ERROR, "curl_easy_perform(connect) failed: %s", curl_easy_strerror(res));
        return -1;
    }
    debug(DBG_TRACE,"ConnectSession returns: \n%s",s.ptr);

    // now, get last init event ID for current ring from json response
    int evtid= parse_connect(config,s.ptr,s.len);
    if (evtid<0) {
        debug(DBG_ERROR, "Error retrieving last event id for ring %d", config->ring);
    }
    if (evtid==0) {
        debug(DBG_NOTICE, "There are no init events for ring %d", config->ring);
        debug(DBG_NOTICE, "Retrying in 5econds");
    }
    /* always cleanup */
    curl_easy_cleanup(curl);
    end_string(&s);
    debug(DBG_DEBUG,"Last init event id for ring %d is %d",config->ring,evtid);
    return evtid;
}

// obtener eventos
// https://{ajax_server}/{baseurl}/ajax/database/eventFunctions.php
//      ?Operation=getEvents&Session={sessionID}&ID={evtid}&TimeStamp={timestamp}&Source=chrono&Name={clientname}&SessionName={sname}
// static char sc_geteventurl[URL_BUFFSIZE];
char ** ajax_wait_for_events(configuration *config, int *evtid, time_t *timestamp) {

    CURL *curl=curl_easy_init();
    struct string s;
    init_string(&s);

    char *baseurl="agility"; // PENDING define in configuration
    snprintf(sc_geteventurl,
             URL_BUFFSIZE,
             "https://%s/%s/ajax/database/eventFunctions.php?Operation=getEvents&Session=%d&ID=%d&TimeStamp=%lu&Source=chrono&Name=%s&SessionName=%s",
             config->ajax_server,BASE_URL,config->status.sessionID,*evtid,*timestamp,config->client_name,getSessionName(config));
    debug(DBG_TRACE,"getEvents: %s",sc_geteventurl);
    curl_easy_setopt(curl, CURLOPT_URL, sc_geteventurl);
    /* example.com is redirected, so we tell libcurl to follow redirection */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_CAINFO, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

    /* Perform the request, res will get the return code */
    int res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK) {
        debug(DBG_ERROR, "curl_easy_perform(getEvents) failed: %s", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return NULL;
    }
    // debug(DBG_TRACE,"getEvents() returns: \n%s",s.ptr);

    // retrieve number of events
    char ** cmds = parse_events(config,s.ptr,s.len,evtid,timestamp);
    if (cmds==NULL) {
        debug(DBG_ERROR, "parseEvents() on ring %d failed", config->ring);
    }
    /* always cleanup */
    curl_easy_cleanup(curl);
    end_string(&s);
    debug(DBG_DEBUG,"Last event id/timestamp for ring %d is %d/%lu",config->ring,*evtid,*timestamp);
    return cmds;
}

// enviar evento
// https://{ajax_server}/{baseurl}/ajax/database/eventFunctions.php
//      ?Operation={chrono_op}&Session={ID}&TimeStamp={timestamp}&source={source}&SessionName={name}&Value={value}
//      &Faltas={f}&Tocados={t}&Rehuses={r}&Eliminado={e}&NoPresentado={n}
// PENDING: study if need additional info ( prueba,jornada, manga, perro, etc... ) o se obtiene de la sesion
// static char sc_puteventurl[URL_BUFFSIZE];
int ajax_put_event(configuration *config, char *type, sc_extra_data_t *data,int flag) {

    // prepare buffer for response
    struct string s;
    init_string(&s);

    // compose GET request
    size_t len=0;
    len=sprintf(sc_puteventurl,"https://%s/%s/ajax/database/eventFunctions.php",config->ajax_server,BASE_URL);
    len+=sprintf(sc_puteventurl+len,"?Operation=chronoEvent");
    len+=sprintf(sc_puteventurl+len,"&Session=%d",config->status.sessionID);
    len+=sprintf(sc_puteventurl+len,"&Source=chrono");
    len+=sprintf(sc_puteventurl+len,"&Name=%s",config->client_name); // beware of spaces and special chars
    len+=sprintf(sc_puteventurl+len,"&SessionName=%s",getSessionName(config)); // beware of spaces and special chars
    len+=sprintf(sc_puteventurl+len,"&Type=%s",type);
    len+=sprintf(sc_puteventurl+len,"&TimeStamp=%lu",time(NULL));
    if(flag!=0) {
        len+=sprintf(sc_puteventurl+len,"&Prueba=%d",config->status.prueba);
        len+=sprintf(sc_puteventurl+len,"&Jornada=%d",config->status.jornada);
        len+=sprintf(sc_puteventurl+len,"&Manga=%d",config->status.manga);
        len+=sprintf(sc_puteventurl+len,"&Tanda=%d",config->status.tanda);
        len+=sprintf(sc_puteventurl+len,"&Numero=%d",config->status.numero);
        len+=sprintf(sc_puteventurl+len,"&Dorsal=%d",config->status.dorsal);
        len+=sprintf(sc_puteventurl+len,"&Perro=%d",config->status.perro);
        len+=sprintf(sc_puteventurl+len,"&Equipo=%d",config->status.equipo);
        len+=sprintf(sc_puteventurl+len,"&Faltas=%d",config->status.faults);
        len+=sprintf(sc_puteventurl+len,"&Tocados=0"); // internally added to faults
        len+=sprintf(sc_puteventurl+len,"&Rehuses=%d",config->status.refusals);
        len+=sprintf(sc_puteventurl+len,"&Eliminado=%d",config->status.eliminated);
        len+=sprintf(sc_puteventurl+len,"&Nopresentado=%d",config->status.notpresent);
    }
    if (data!=NULL) {
        len+=sprintf(sc_puteventurl+len,"&Value=%s",data->value);
        len+=sprintf(sc_puteventurl+len,"&Tiempo=%lu",data->tiempo);
        len+=sprintf(sc_puteventurl+len,"&Oper=%s",data->oper);
        len+=sprintf(sc_puteventurl+len,"&stop=%lu",data->stop);
        len+=sprintf(sc_puteventurl+len,"&start=%lu",data->start);
    }

    /* doc states that curl_easy_perform is not re-entrant, so create a curl handler on every send event */
    CURL *curl = curl_easy_init();
    if(!curl) {
        debug(DBG_ERROR,"Cannot initialize curl handler to put event");
        return -1;
    }
    debug(DBG_TRACE,"putEvent: %s",sc_puteventurl);
    curl_easy_setopt(curl, CURLOPT_URL, sc_puteventurl);
    /* example.com is redirected, so we tell libcurl to follow redirection */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_CAINFO, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

    /* Perform the request, res will get the return code */
    int res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK) {
        debug(DBG_ERROR, "curl_easy_perform(putEvent) failed: %s", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return -1;
    }
    // debug(DBG_TRACE,"getEvents() returns: \n%s",s.ptr);
    // check response for errors. Just try to locate "errorMsg" in response string
    /* cleanup */
    curl_easy_cleanup(curl);
    end_string(&s);
    return 0;
}
