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

static char error_str[1024];

static configuration *config;
static herror_t status;

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


static int sendrec_error() {
    return -1;
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
 * tipo: any of configutation action or
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

    // create connection
    if ( ! (conn = httpc_new()) ) {
        fprintf(stderr, "Cannot create nanoHTTP client connection\n");
        return NULL;
    }

    /* Set header for chunked transport */
    httpc_set_header(conn, HEADER_TRANSFER_ENCODING, TRANSFER_ENCODING_CHUNKED);

    /* POSTing will be done in 3 steps
     1. httpc_post_begin()
     2. http_output_stream_write()
     3. httpc_post_end()
    */
    if ((status = httpc_post_begin(conn, url)) != H_OK) {
        fprintf(stderr, "nanoHTTP POST begin failed (%s)\n", herror_message(status));
        herror_release(status);
        httpc_free(conn);
        return NULL;
    }

    if ((status = http_output_stream_write(conn->out, postdata, strlen(postdata))) != H_OK) {
        fprintf(stderr, "nanoHTTP send POST data failed (%s)\n", herror_message(status));
        herror_release(status);
        httpc_free(conn);
        return NULL;
    }

    if ((status = httpc_post_end(conn, &response)) != H_OK ) {
        fprintf(stderr, "nanoHTTP receive POST response failed (%s)\n", herror_message(status));
        herror_release(status);
        httpc_free(conn);
        return NULL;
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
    char *check=sendrec("/configuracion_accion",NULL,NULL);
    if(!check) {
        httpc_destroy();
        snprintf(error_str,1024,"Cannot contact with Canometer (%s) ", herror_message(status));
        debug(DBG_ERROR,error_str);
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
    if(!check) return sendrec_error();
    // send reset
    check=sendrec("/canometro_accion","0","0");
    if(!check) return sendrec_error();
    // retrieve configuracion
    check=sendrec("/config_xml",NULL,NULL);
    if(!check) return sendrec_error();
    // PENDNG: store configuration
    // retrieve status
    check=sendrec("/xml",NULL,NULL);
    if(!check) return sendrec_error();
    // PENDING: Store status
    return 0;
}

int ADDCALL module_close(){
    return 0;
}

int ADDCALL module_read(char *buffer,size_t length){
    int ret=1;
    static char *inbuff=NULL;
    if (inbuff==NULL) inbuff=malloc(1024*sizeof(char));
    memset(inbuff,0,1024*sizeof(char));

    do {
        // perform a GET /xml operation
    } while(ret==0);
    if (ret <0 ) {
        debug(DBG_ERROR,"network_read() error %s",error_str);
        snprintf(buffer,length,"");
        return strlen(buffer);
    };
    debug(DBG_TRACE,"module_read() received '%s'",inbuff);
    // At this moment, Digican only sends 2 comands "START" and "PARAR". So just a simple parser
    if (strncmp(inbuff,"START",5)==0) {
        snprintf(buffer,length,"start 0\n");
    }
    else if (strncmp(inbuff,"PARAR",5)==0) {
        int elapsed= 10* atoi(inbuff+7);
        snprintf(buffer,length,"stop %d\n",elapsed);
    }
    // PENDING: parse and handle additional comands sent from digican chrono ( ask oitoinnova )
    else snprintf(buffer,length,"");
    debug(DBG_TRACE,"module_read() sending to serial manager '%s'",buffer);
    return strlen(buffer);
}

static int canometro_faltas=0;
static int canometro_rehuses=0;

int ADDCALL module_write(char **tokens, size_t ntokens){
    static char *buffer=NULL;
    if (buffer==NULL) buffer=malloc(1024*sizeof(char));
    memset(buffer,0,1024);
    char *cmd=tokens[1];

    // { 0, "start",   "Start of course run",             "[miliseconds] {0}"},
    if (strcasecmp("start",cmd)==0) {
        sprintf(buffer,"START$");
    }
    // { 1, "int",     "Intermediate time mark",          "<miliseconds>"},
    else if (strcasecmp("int",cmd)==0) {
        // on intermediate time cannot provide timestamp to digican, so just call to show intermediate time
        // this may lead in some precission errors due to communications lag
        sprintf(buffer,"TEMPI$");
    }
    // { 2, "stop",    "End of course run",               "<miliseconds>"},
    else if (strcasecmp("stop",cmd)==0) {
        // digican uses cents of seconds, so remove last digit
        long cents=(config->status.stop_time - config->status.start_time)/10;
        sprintf(buffer,"PARAR%04d$",(int)cents);
    }
    // { 3, "fail",    "Sensor faillure detected",        ""},
    // unsupported in digican chrono
    else if (strcasecmp("fail",cmd)==0) { unsuported("digican","fail");  }
    // { 4, "ok",      "Sensor recovery. Chrono ready",   ""},
    // unsupported in digican chrono
    else if (strcasecmp("ok",cmd)==0) {  unsuported("digican","ok"); }
    // { 5, "msg",     "Show message on chrono display",  "<message> [seconds] {2}"},
        // unsupported in digican chrono
    else if (strcasecmp("msg",cmd)==0) {  unsuported("digican","msg"); }
    // { 6, "walk",    "Course walk (0:stop)",            "<seconds> {420}"},
    else if (strcasecmp("walk",cmd)==0) {
        // PENDING: ask digican how to provide course walk duration in seconds ( or minutes )
        sprintf(buffer,"RECON$");
    }
    // { 7, "down",    "Start 15 seconds countdown",      ""},
    else if (strcasecmp("walk",cmd)==0) {
        // PENDING: Ask digican how to provide countdown duration instead of 15 seconds
        sprintf(buffer,"INICI$");
    }
    /*
     * In digican chrono we cannot provide absolute values for faults and refusals, so need
     * to track an inner count and sincronize it with master data
     * so the way to work is:
     * On reset,reloj and so clear variables
     */
    // { 8, "fault",   "Mark fault (+/-/#)",              "< + | - | num {+}>"},
    else if (strcasecmp("fault",cmd)==0) {
        int len=0;
        int ft=config->status.faults+config->status.touchs;
        if (canometro_faltas < config->status.faults) {
            for (;canometro_faltas < ft ; canometro_faltas++) len+=sprintf(buffer+len,"FALT+$");
        } else {
            for (;canometro_faltas>ft;canometro_faltas--) len+=sprintf(buffer+len,"FALT-$");
        }
    }
    // { 9, "refusal", "Mark refusal (+/-/#)",            "< + | - | num {+}>"},
    else if (strcasecmp("refusal",cmd)==0) {
        int len=0;
        if (canometro_rehuses < config->status.refusals) {
            for (;canometro_rehuses < config->status.refusals; canometro_rehuses++) len+=sprintf(buffer+len,"REHU+$");
        } else {
            for (;canometro_rehuses > config->status.refusals; canometro_rehuses--) len+=sprintf(buffer+len,"REHU-$");
        }
    }
    // { 10, "elim",    "Mark elimination [+-]",          "[ + | - ] {+}"},
    else if (strcasecmp("elim",cmd)==0) {
        // digican has just "fire up" eliminated. no way to remove mark :-(
        sprintf(buffer,"ELIMI$");
    }
    // { 11, "data",    "Set course fault/ref/disq info", "<faults>:<refulsals>:<disq>"},
    else if (strcasecmp("data",cmd)==0) {
        int len=0;
        int ft=config->status.faults+config->status.touchs;
        if (canometro_faltas < ft) {
            for (;canometro_faltas <ft;canometro_faltas++) len+=sprintf(buffer+len,"FALT+$");
        } else {
            for (;canometro_faltas>ft;canometro_faltas--) len+=sprintf(buffer+len,"FALT-$");
        }
        if (canometro_rehuses < config->status.refusals) {
            for (;canometro_rehuses < config->status.refusals; canometro_rehuses++) len+=sprintf(buffer+len,"REHU+$");
        } else {
            for (;canometro_rehuses > config->status.refusals; canometro_rehuses--) len+=sprintf(buffer+len,"REHU-$");
        }
        if (config->status.eliminated!=0) len+=sprintf(buffer+len,"ELIMI$");
    }
    // { 12, "reset",  "Reset chronometer and countdown", "" },
    else if (strcasecmp("reset",cmd)==0) {
        canometro_faltas=0;
        canometro_rehuses=0;
        sprintf(buffer,"RESET$");
    }
    // { 13, "help",   "show command list",               "[cmd]"},
    //  useless outside console
    else if (strcasecmp("help",cmd)==0) {  unsuported("digican","help");  }
    // { 14, "version","Show software version",           "" },
    //  useless outside console
    else if (strcasecmp("version",cmd)==0) {  unsuported("digican","version");  }
    // { 15, "exit",   "End program (from console)",      "" },
    // useless outside console
    else if (strcasecmp("exit",cmd)==0) {  unsuported("digican","exit");  }
    // { 16, "server", "Set server IP address",           "<x.y.z.t> {0.0.0.0}" },
    //  useless outside console
    else if (strcasecmp("server",cmd)==0) { unsuported("digican","server"); }
    // { 17, "ports",  "Show available serial ports",     "" },
    //  useless outside console
    else if (strcasecmp("ports",cmd)==0) { unsuported("digican","ports"); }
    // { 18, "config", "List configuration parameters",   "" },
    //  useless outside console
    else if (strcasecmp("config",cmd)==0) {  unsuported("digican","config"); }
    // { 19, "status", "Show Fault/Refusal/Elim state",   "" },
    //  useless outside console
    else if (strcasecmp("status",cmd)==0) {  unsuported("digican","status"); }
    // { 20, "turn",   "Set current dog order number [+-#]", "[ + | - | num ] {+}"},
    //  useless outside console
    else if (strcasecmp("turn",cmd)==0) { unsuported("digican","turn"); }
    // { 21, "clock",  "Enter clock mode",                "[ hh:mm:ss ] {current time}"},
    // PENDING: ask oito-innova how to specify current time
    else if (strcasecmp("clock",cmd)==0) {
        // PENDING: check if setting hh/mm is available in digican crono
        canometro_faltas=0;
        canometro_rehuses=0;
        sprintf(buffer,"RELOJ$");
    }
    // { 22, "debug",  "Get/Set debug level",             "[ new_level ]"},
    // only for console
    // { -1, NULL,     "",                                "" }
    else {
        // arriving here means unrecognized or not supported command.
        debug(DBG_NOTICE,"Unrecognized command '%s'",tokens[1]);
        return 0;
    }

    // so that's allmost all done, just sending command to serial port....
    // notice "blocking" mode. needed as digican does not support full duplex communications
    debug(DBG_TRACE,"module_write(), send: '%s'",buffer);

    // todo: write send request
    return 0;
}

char * ADDCALL module_error() {
    return error_str;
}