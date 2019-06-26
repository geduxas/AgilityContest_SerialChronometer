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
#include "ajax_curl.h"
#include "ajax_json.h"

#define URL_BUFFSIZE 2018
// PENDING rework to be retrieved from configuration
#define BASE_URL "agility"

static char SessionName[1024]; // // source:ringsessid:view:mode:sessionaname

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

static char * getSessionName(configuration *config, int sessionID) {
    static char *name=NULL;
    if (!name) name =calloc(1024,sizeof(char));
    snprintf(name,1024,"chrono:%d:0:0:%s",sessionID,config->client_name);
    return name;
}

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
        return -1;
    }
    debug(DBG_DEBUG,"SelectRing() returns: \n%s",s.ptr);

    // now, get sessionID for current ring from json response
    int sessid= parse_select(config,s.ptr,s.len);
    if (sessid<0) {
        debug(DBG_ERROR, "There is no session ID for ring: %s", config->ring);
        return -1;
    }
    /* always cleanup */
    end_string(&s);
    curl_easy_cleanup(curl);
    return sessid;
}

// conectarse a una sesion
// https://{ajax_server}/{baseurl}/ajax/database/eventFunctions.php
//      ?Operation=connect&Session={ID}&SessionName={name}
int ajax_open_session(configuration *config,int sessionid) {

    CURL *curl=curl_easy_init();
    struct string s;
    init_string(&s);

    char *baseurl="agility"; // PENDING define in configuration
    snprintf(sc_connecturl,
             URL_BUFFSIZE,
             "https://%s/%s/ajax/database/eventFunctions.php?Operation=connect&Session=%d&SessionName=%s",
             config->ajax_server,BASE_URL,sessionid,getSessionName(config,sessionid));
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

    // now, get sessionID for current ring from json response
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
char ** ajax_wait_for_events(configuration *config, int sessionid, int *evtid, time_t *timestamp) {

    CURL *curl=curl_easy_init();
    struct string s;
    init_string(&s);

    char *baseurl="agility"; // PENDING define in configuration
    snprintf(sc_geteventurl,
             URL_BUFFSIZE,
             "https://%s/%s/ajax/database/eventFunctions.php?Operation=getEvents&Session=%d&ID=%d&TimeStamp=%lu&Source=chrono&Name=%s&SessionName=%s",
             config->ajax_server,BASE_URL,sessionid,*evtid,*timestamp,config->client_name,getSessionName(config,sessionid));
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
int ajax_put_event(configuration *config, char *type, char *value) {
    /* doc states that curl_easy_perform is not re-entrant, so create a curl handler on every send event */
    CURL *sc = curl_easy_init();
    if(!sc) {
        debug(DBG_ERROR,"Cannot initialize curl handler to put event");
        return -1;
    }


    /* cleanup */
    curl_easy_cleanup(sc);
    return 0;
}
