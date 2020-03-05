//
// Created by jantonio on 05-Mar-2020
// Copyright 2020 by Juan Antonio Martínez Castaño <info@agilitycontest.es>

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
// and associated documentation files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies
// or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include <stdio.h>
#include <string.h>

#include "modules.h"
#include "sc_config.h"
#include "debug.h"
#include "nanohttp/nanohttp-client.h"

/*

 Datos de cronometro

<?xml version='1.0'?>
<xml>
  <millistime> %u </millistime>
  <tiempoactual> %lu </tiempoactual>
  <cronocorriendo> %d </cronocorriendo>
  <faltas> %d </faltas>
  <rehuses> %d </rehuses>
  <eliminado> %d </eliminado>
  <cuentaresultados> 0 </cuentaresultados>
  <versionresultados> 0 </versionresultados>
</xml>
*/
typedef struct canometroweb_data {
    unsigned int millistime;
    unsigned long tiempoactual;
    int cronocorriendo;
    int faltas;
    int rehuses;
    int eliminado;
    int cuentaresultados;
    int versionresultados;
} canometroweb_data_t;

/*
  Datos de configuracion

 <?xml version='1.0'?>
 <xml>
  <Brightness> "Low" | "High" </Brightness>
  <Precision> "Centis" | "Milis" </Precision>
  <Guardtime> %d (secs) </Guardtime>
  <Walktime> %d (min) </Walktime>
  <Walkstyle> "Numbers" | "Graphic" </Walkstyle>
  <Ring> %d </Ring>
 </xml>

 */
typedef struct canometroweb_config {
    char *brightness;
    char *precision;
    int guardtime; // seconds
    int walktime; // minutes
    char *walkstyle;
    int ring;
} canometroweb_config_t;

static char error_str[1024];
static configuration *config;
static herror_t status;
static canometroweb_data_t cw_data;
static canometroweb_config_t cw_config;

static char *sendrec_error(httpc_conn_t *conn,char *msg) {
    snprintf(error_str,1024,msg, herror_message(status));
    debug(DBG_ERROR,error_str);
    herror_release(status);
    httpc_free(conn);
    return NULL;
}
/**
 * Parses an xml response string with canometer configuration
 * @param pt where to store data. if null create space by calloc
 * @param xml string to be xml parsed
 * @return pointer to readed configuration
 */
static canometroweb_config_t *parse_config_xml(canometroweb_config_t *pt,char *xml) {
    // PENDING: write
    return NULL;
}

/**
 * Parses an xml response string with canometer status/data
 * @param pt where to store data. if null create space by calloc
 * @param xml string to be xml parsed
 * @return pointer to readed data
 */
static canometroweb_data_t *parse_status_xml(canometroweb_data_t *pt,char *xml) {
    // PENDING: write
    return NULL;
}

/**
 *
 * @param page baseurl ( ie: /xml )
 * @param tipo name of parameter to set
 * @param valor value of parameter to set
 * @return returned string from canometro
 *
 * pages: /startstop /xml /config_xml /configuracion_accion /canometro_accion
 *
 * POSTDATA: tipo=<tipo>&valor=<valor>
 * tipo: any of configutacion_accion or
 * F (Fault)
 * R (Eefusal)
 * E (Eliminate )
 * 0 (Reset)
 * W (coursewalk)
 * C (Countdown)
 *
 */
static char *sendrec(char *page,char *tipo,char *valor) {
    // connection related vars
    httpc_conn_t *conn;
    hresponse_t *response;
    hpair_t *pair;

    // data for response
    char result[1024];

    // data for post request
    char url[256];
    char postdata[256];
    memset(url,0,256);
    memset(postdata,0,256);
    snprintf(url,254,"http://%s/%s",config->comm_ipaddr,page);
    if (tipo && valor) {
        snprintf(postdata,255,"tipo=%s&valor=%s",tipo,valor);
    }
    debug(DBG_TRACE,"sendrec() url: '%s' postdata: '%s'",url,postdata);
    // create connection
    if ( ! (conn = httpc_new()) ) {
        debug(DBG_ERROR,"Cannot create nanoHTTP client connection");
        return NULL;
    }

    /* Set header for chunked transport */
    httpc_set_header(conn, HEADER_TRANSFER_ENCODING, TRANSFER_ENCODING_CHUNKED);

    /* POSTing is be done in 3 steps
     1. httpc_post_begin()
     2. http_output_stream_write()
     3. httpc_post_end()
    */
    if ((status = httpc_post_begin(conn, url)) != H_OK) {
        return sendrec_error(conn,"nanoHTTP POST begin failed (%s)");
    }
    if ((status = http_output_stream_write(conn->out, postdata, strlen(postdata))) != H_OK) {
        return sendrec_error(conn,"nanoHTTP send POST data failed (%s)");
    }
    if ((status = httpc_post_end(conn, &response)) != H_OK ) {
        return sendrec_error(conn,"nanoHTTP receive POST response failed (%s)");
    }
    // handle response
    int len=0;
    while (http_input_stream_is_ready(response->in)) {
       len += http_input_stream_read(response->in, &result[len], 1024-len);
       if (len>=1023) break;
     }

    // clean up and return
    hresponse_free(response);
    httpc_free(conn);
    return strdup(result);
}


/* Declare our Add function using the above definitions. */
int ADDCALL module_init(configuration *cfg) {
    config = cfg;
    // initialize
    char *argv[] = {"canometroweb",config->comm_ipaddr, NULL};
    if ((status = httpc_init(2, argv)) != H_OK) {
        snprintf(error_str,1024,"Cannot init nanoHTTP client (%s)", herror_message(status));
        debug(DBG_ERROR,error_str);
        return -1;
    }
    // now check connection against Canometer by mean of trying to receive configuration
    char *check=sendrec("/config_xml",NULL,NULL);
    if(!check) {
        debug(DBG_ERROR,"Cannot contact with Canometer. Abort thread");
        httpc_destroy();
        return -1;
    }
    return 0;
}

int ADDCALL module_end() {
    return 0;
}

int ADDCALL module_open(){
    char *check;
    char ring[4];
    snprintf(ring,4,"%d",config->ring);
    check=sendrec("/configuracion_accion","Ring",ring);
    if(!check) return-1;
    // send reset
    check=sendrec("/canometro_accion","0","0");
    if(!check) return -1;
    // retrieve configuracion
    check=sendrec("/config_xml",NULL,NULL);
    if(!check) return  -1;
    // PENDNG: store configuration
    // retrieve status
    check=sendrec("/xml",NULL,NULL);
    if(!check) return  -1;
    // PENDING: Store status
    return 0;
}

int ADDCALL module_close(){
    debug(DBG_NOTICE,"Closing network chronometer module");
    httpc_destroy();
    return 0;
}

int ADDCALL module_read(char *buffer,size_t length){
    char *received;

    received=sendrec("/xml",NULL,NULL);
    if (!received) {
        debug(DBG_ERROR,"network_read() error %s",error_str);
        snprintf(buffer,length,"");
        return strlen(buffer);
    }
    debug(DBG_TRACE,"canometroweb module_read() received '%s'",received);
    // parse xml and compare with stored data
    canometroweb_data_t *data=parse_status_xml(NULL,received);

    // PENDING: compare data and generate internal commands
    // to report changes in f:t:r return back "data f:t:r" command
    // to report crono count state, evaluate and send proper start/stop comand
    // pending: recognize and parse intermediate time and sensor fail/failbak
    return 0;
}

static int canometro_faltas=0;
static int canometro_rehuses=0;

int ADDCALL module_write(char **tokens, size_t ntokens){

    char *result=NULL;
    char *cmd=tokens[1];

    char page[64];
    char tipo[16];
    char valor[16];
    memset(page,0,64);
    memset(tipo,0,16);
    memset(valor,0,16);

    // { 0, "start",   "Start of course run",             "[miliseconds] {0}"},
    if (strcasecmp("start",cmd)==0) {
        // si el cronometro esta corriendo no hacer nada
        // else mandamos la orden de startstop para indicar arranque manual
        if (cw_data.cronocorriendo==0) {
            snprintf(page,sizeof(page),"/startstop");
        }
    }
    // { 1, "int",     "Intermediate time mark",          "<miliseconds>"},
    // canometro web interface has no "intermediate time command"  but supported in event api protocol :-(
    else if (strcasecmp("int",cmd)==0) { unsuported("canometroweb","int"); }
    // { 2, "stop",    "End of course run",               "<miliseconds>"},
    else if (strcasecmp("stop",cmd)==0) {
        // si crono parado no hacer nada; else mandamos orden de parada manual
        if (cw_data.cronocorriendo==0) {
            snprintf(page,sizeof(page),"/startstop");
        }
    }
    // { 3, "fail",    "Sensor faillure detected",        ""},
    // unsupported in canometroweb chrono but supported in event api protocol :-(
    else if (strcasecmp("fail",cmd)==0) { unsuported("canometroweb","fail");  }
    // { 4, "ok",      "Sensor recovery. Chrono ready",   ""},
    // unsupported in canometroweb chrono but supported in event api protocol :-(
    else if (strcasecmp("ok",cmd)==0) {  unsuported("canometroweb","ok"); }
    // { 5, "msg",     "Show message on chrono display",  "<message> [seconds] {2}"},
        // unsupported in canometroweb chrono but supported in event api protocol :-(
    else if (strcasecmp("msg",cmd)==0) {  unsuported("canometroweb","msg"); }
    // { 6, "walk",    "Course walk (0:stop)",            "<seconds> {420}"},
    else if (strcasecmp("walk",cmd)==0) {
        int minutes=atoi(tokens[2])/60;
        if (minutes==0) { // send reset, to stop coursewalk
            snprintf(page,sizeof(page),"/canometro_accion");
            snprintf(tipo,sizeof(tipo),"0"); // reset cmd
            snprintf(valor,sizeof(valor),"0");
        } else {
            // if current course time differs to provided, set cronometer
            if (cw_config.walktime!=minutes) {
                cw_config.walktime=minutes;
                snprintf(page,sizeof(page),"/configuracion_accion");
                snprintf(tipo,sizeof(tipo),"Walktime");
                snprintf(valor,sizeof(valor),"%d",minutes);
                sendrec(page,tipo,valor);
            }
            // and fireup course walk
            snprintf(page,sizeof(page),"/canometro_accion");
            snprintf(tipo,sizeof(tipo),"W");
            snprintf(valor,sizeof(valor),"0");
        }
    }
    // { 7, "down",    "Start 15 seconds countdown",      ""},
    else if (strcasecmp("down",cmd)==0) {
        snprintf(page,sizeof(page),"/canometro_accion");
        snprintf(tipo,sizeof(tipo),"C");
        snprintf(valor,sizeof(valor),"0");
    }
    // { 8, "fault",   "Mark fault (+/-/#)",              "< + | - | num {+}>"},
    else if (strcasecmp("fault",cmd)==0) {
        int ft=config->status.faults+config->status.touchs;
        snprintf(page,sizeof(page),"/canometro_accion");
        snprintf(tipo,sizeof(tipo),"F");
        snprintf(valor,sizeof(valor),"%d",ft);
    }
    // { 9, "refusal", "Mark refusal (+/-/#)",            "< + | - | num {+}>"},
    else if (strcasecmp("refusal",cmd)==0) {
        snprintf(page,sizeof(page),"/canometro_accion");
        snprintf(tipo,sizeof(tipo),"R");
        snprintf(valor,sizeof(valor),"%d",config->status.refusals);
    }
    // { 10, "elim",    "Mark elimination [+-]",          "[ + | - ] {+}"},
    // PENDING: revise eliminated behavior. if needed, just compare internal and xmldata to handle
    else if (strcasecmp("elim",cmd)==0) {
        snprintf(page,sizeof(page),"/canometro_accion");
        snprintf(tipo,sizeof(tipo),"E");
        snprintf(valor,sizeof(valor),"%d",config->status.eliminated);
    }
    // { 11, "data",    "Set course fault/ref/disq info", "<faults>:<refulsals>:<disq>"},
    else if (strcasecmp("data",cmd)==0) {
        int ft=config->status.faults+config->status.touchs;
        snprintf(page,sizeof(page),"/canometro_accion");
        snprintf(tipo,sizeof(tipo),"F");
        snprintf(valor,sizeof(valor),"%d",ft);
        sendrec(page,tipo,valor);
        snprintf(tipo,sizeof(tipo),"R");
        snprintf(valor,sizeof(valor),"%d",config->status.refusals);
        sendrec(page,tipo,valor);
        // PENDING: revise eliminated behavior.
        // if needed, just compare internal and xmldata to handle
        snprintf(tipo,sizeof(tipo),"E");
        snprintf(valor,sizeof(valor),"%d",config->status.eliminated);
    }
    // { 12, "reset",  "Reset chronometer and countdown", "" },
    else if (strcasecmp("reset",cmd)==0) {
        snprintf(page,sizeof(page),"/canometro_accion");
        snprintf(tipo,sizeof(tipo),"0"); // reset cmd
        snprintf(valor,sizeof(valor),"0");
    }
    // { 13, "help",   "show command list",               "[cmd]"},
    //  useless outside console
    else if (strcasecmp("help",cmd)==0) {  unsuported("canometroweb","help");  }
    // { 14, "version","Show software version",           "" },
    //  useless outside console
    else if (strcasecmp("version",cmd)==0) {  unsuported("canometroweb","version");  }
    // { 15, "exit",   "End program (from console)",      "" },
    // useless outside console
    else if (strcasecmp("exit",cmd)==0) {  unsuported("canometroweb","exit");  }
    // { 16, "server", "Set server IP address",           "<x.y.z.t> {0.0.0.0}" },
    //  useless outside console
    else if (strcasecmp("server",cmd)==0) { unsuported("canometroweb","server"); }
    // { 17, "ports",  "Show available serial ports",     "" },
    //  useless outside console
    else if (strcasecmp("ports",cmd)==0) { unsuported("canometroweb","ports"); }
    // { 18, "config", "List configuration parameters",   "" },
    //  not real use, just to force read an parse canometer configuration
    else if (strcasecmp("config",cmd)==0) {
        char *result=sendrec("/config_xml",NULL,NULL);
        if (!result) {
            debug(DBG_ERROR,"cannot retrieve canometroweb configuration");
            return -1;
        }
        parse_config_xml(&cw_config,result);
        free(result);
        return 0;
    }
    // { 19, "status", "Show Fault/Refusal/Elim state",   "" },
    //  not real use, just to force read an parse canometer data
    else if (strcasecmp("status",cmd)==0) {
        result=sendrec("/xml",NULL,NULL);
        if (!result) {
            debug(DBG_ERROR,"cannot retrieve canometroweb status");
            return -1;
        }
        parse_status_xml(&cw_data,result);
        free(result);
        return 0;
    }
    // { 20, "turn",   "Set current dog order number [+-#]", "[ + | - | num ] {+}"},
    //  useless outside console
    else if (strcasecmp("turn",cmd)==0) { unsuported("canometroweb","turn"); }
    // { 21, "clock",  "Enter clock mode",                "[ hh:mm:ss ] {current time}"},
    // PENDING: ask Galican if supported
    else if (strcasecmp("clock",cmd)==0) { unsuported("canometroweb","clock"); }
    // { 22, "debug",  "Get/Set debug level",             "[ new_level ]"},
    // only for console
    // { -1, NULL,     "",                                "" }
    else {
        // arriving here means unrecognized or not supported command.
        debug(DBG_NOTICE,"Unrecognized command '%s'",tokens[1]);
        return 0;
    }

    // so that's allmost all done, just sending command to serial port....
    // notice "blocking" mode. needed as canometroweb does not support full duplex communications
    debug(DBG_TRACE,"calling to sendrec(), command:'%s' page:'%s' tipo:'%s' valor:'%s'",tokens[1],page,tipo,valor);
    if (strcmp(tipo,"")==0) result=sendrec(page,NULL,NULL);
    else result=sendrec(page, tipo,valor);
    if (!result) {
        debug(DBG_ERROR,"call to sendrec() failed, command:'%s' page:'%s' tipo:'%s' valor:'%s'",tokens[1],page,tipo,valor);
        return -1;
    }
    free(result);
    return 0;
}

char * ADDCALL module_error() {
    return error_str;
}