//
// Created by jantonio on 5/05/19.
//

#define AGILITYCONTEST_SERIALCHRONOMETER_AJAX_MGR_C
#include <stdio.h>
#include <unistd.h>
#include <curl/curl.h>

#include "../include/main.h"
#include "../include/debug.h"
#include "../include/ajax_mgr.h"
#include "../include/sc_config.h"
#include "../include/sc_sockets.h"

/*
# Request to server are made by sending json request to:
#
# http://ip.addr.of.server/base_url/ajax/database/eventFunctions.php
#
# Parameter list
# Operation=chronoEvent
# Type= one of : ( from ajax/database/Eventos.php )
#               'crono_start'   // Arranque Crono electronico
#               'crono_int'     // Tiempo intermedio Crono electronico
#               'crono_stop'    // Parada Crono electronico
#               'crono_rec'     // comienzo/fin del reconocimiento de pista
#               'crono_dat'     // Envio de Falta/Rehuse/Eliminado desde el crono (need extra data. see below)
#               'crono_reset'   // puesta a cero del contador
#               'crono_error'   // sensor error detected (Value=1) or solved (Value=0)
#               'crono_ready'   // chrono synced and listening (Value=1) or disabled (Value=0)
# Session= Session ID to join. You should select it from retrieved list of available session ID's from server
# Source= Chronometer ID. should be in form "chrono_sessid"
# Value= start/stop/int: time of event detection
#                 error: 1: detected 0:solved
# Timestamp= time mark of last event parsed as received from server
# example:
# ?Operation=chronoEvent&Type=crono_rec&TimeStamp=150936&Source=chrono_2&Session=2&Value=150936

*/
#define URL_BUFFSIZE 2018

/* manual states that curl handler is not re-entrant, so create/destroy handler on every putEvent() */
static CURL *curl; // used in select/connect/getEvents

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

static int ajax_mgr_start(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_int(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_stop(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_fail(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_ok(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_msg(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_walk(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_down(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_fault(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_refusal(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_elim(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_reset(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_exit(configuration * config, int slot, char **tokens, int ntokens) {
    debug(DBG_INFO,"Ajax Manager Thread exit requested");
    return -1;
}
static int ajax_mgr_server(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static int ajax_mgr_dorsal(configuration * config, int slot, char **tokens, int ntokens) {
    return 0;
}
static func entries[32]= {
        ajax_mgr_start,  // { 0, "start",   "Start of course run",             "[miliseconds] {0}"},
        ajax_mgr_int,    // { 1, "int",     "Intermediate time mark",          "<miliseconds>"},
        ajax_mgr_stop,   // { 2, "stop",    "End of course run",               "<miliseconds>"},
        ajax_mgr_fail,   // { 3, "fail",    "Sensor faillure detected",        ""},
        ajax_mgr_ok,     // { 4, "ok",      "Sensor recovery. Chrono ready",   ""},
        ajax_mgr_msg,    // { 5, "msg",     "Show message on chrono display",  "<message> [seconds] {2}"},
        ajax_mgr_walk,   // { 6, "walk",    "Course walk (0:stop)",            "<seconds> {420}"},
        ajax_mgr_down,   // { 7, "down",    "Start 15 seconds countdown",      ""},
        ajax_mgr_fault,  // { 8, "fault",   "Mark fault (+/-/#)",              "< + | - | num >"},
        ajax_mgr_refusal,// { 9, "refusal", "Mark refusal (+/-/#)",            "< + | - | num >"},
        ajax_mgr_elim,   // { 10, "elim",    "Mark elimination [+-]",           "[ + | - ] {+}"},
        ajax_mgr_reset,  // { 11, "reset",  "Reset chronometer and countdown", "" },
        NULL,            // { 12, "help",   "show command list",               "[cmd]"},
        NULL,            // { 13, "version","Show software version",           "" },
        ajax_mgr_exit,   // { 14, "exit",   "End program (from console)",      "" },
        ajax_mgr_server, // { 15, "server", "Set server IP address",           "<x.y.z.t> {0.0.0.0}" },
        NULL,            // { 16, "ports",  "Show available serial ports",     "" },
        NULL,            // { 17, "config", "List configuration parameters",   "" },
        NULL,            // { 18, "status", "Show Fault/Refusal/Elim state",   "" },
        ajax_mgr_dorsal, // { 19, "turn",   "Set current dog order number [+-#]", "[ + | - | num ] {+}"},
        NULL,            // { 20, "clock",  "Enter clock mode",                "[ hh:mm:ss ] {current time}"},
        NULL,            // { 21, "debug",  "Get/Set debug level",             "[ new_level ]"},
        NULL             // { -1, NULL,     "",                                "" }
};

static void find_server() {
    // obtenemos las interfaces "localhost" y de red con dirección IPv4
    // en cada interfaz iteramos
    // si es localhost, solo se itera la 127.0.0.1
    // PENDING. esta funcion implica implementar "ifconfig", lo cual es demasiado lio para una primera version
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

/**
 * config->ajax_server contains url
// https://{ajax_server}/{baseurl}/ajax/database/sessionFunctions.php
//      ?Operation=selectring
 * @return
 */
static int connect_server (configuration *config) {

    struct string s;
    init_string(&s);

    char *baseurl="agility"; // PENDING define in configuration
    snprintf(sc_selecturl,URL_BUFFSIZE,"https://%s/%s/ajax/database/sessionFunctions.php?Operation=selectring",config->ajax_server,baseurl);
    debug(DBG_TRACE,"Connecting server at '%s'",config->ajax_server);
    curl_easy_setopt(curl, CURLOPT_URL, sc_selecturl);
    /* example.com is redirected, so we tell libcurl to follow redirection */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

    /* Perform the request, res will get the return code */
    int res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK) {
        debug(DBG_ERROR, "curl_easy_perform() failed: %s", curl_easy_strerror(res));
        return -1;
    }
    debug(DBG_INFO,"Curl call returns: \n%s",s.ptr);

    /* always cleanup */
    curl_easy_cleanup(curl);
    return 0;
}

static int wait_for_event() {
    return 0;
}

static int parse_event(int evtnum) {
    return 0;
}

static int put_event(configuration *config, char *type, char *value) {
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

void *ajax_manager_thread(void *arg){
    int slotIndex= * ((int *)arg);
    sc_thread_slot *slot=&sc_threads[slotIndex];
    configuration *config=slot->config;
    slot->entries = entries;

    // create sock for local communicacion
    char portstr[16];
    snprintf(portstr,16,"%d",config->local_port+config->ring);
    slot->sock=connectUDP("localhost",portstr);
    if (slot->sock <0) {
        debug(DBG_ERROR,"AjaxMgr: Cannot create local socket");
        return NULL;
    }

    // if ajax server is "find", or "0.0.0.0" try to locate before entering loop
    if (strcasecmp(config->ajax_server,"find")==0 || strcasecmp(config->ajax_server,"0.0.0.0")==0) {
        debug(DBG_INFO,"Trying to locate AgilityContest server IP address");
    }

    // initialize curl
    curl = curl_easy_init();
    if(!curl) {
        debug(DBG_ERROR,"Cannot initialize curl subsystem");
        return NULL;
    }
    if (connect_server(config)<0) {
        debug(DBG_ERROR,"Cannot connect AgilityContest server at '%s'",config->ajax_server);
        return NULL;
    }

    // mark thread alive before entering loop
    slot->index=slotIndex;
    int res=0;
    while(res>=0) {
        res=wait_for_event();
        if (res<0) {
            debug(DBG_NOTICE,"WaitForEvent failed. retrying");
            res=0;
            // fallback in next "if"
        }
        if (res==0) {
            debug(DBG_NOTICE,"No events. wait and retry");
            sleep(5);
            continue;
        } else {
            for (int n=0;n<res;n++) {
                res=parse_event(n);
            }
        }
        // finally check for exit request
        if (slot->index<0) {
            debug(DBG_TRACE,"Ajax thread: 'exit' command invoked");
            res=-1;
        }
    }
    debug(DBG_TRACE,"Exiting ajax thread");

    curl_easy_cleanup(curl);
    slot->index=-1;
    return &slot->index;
}
