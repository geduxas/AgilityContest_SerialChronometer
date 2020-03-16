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
#include <unistd.h>
#include <semaphore.h>
#include <curl/curl.h>
#include "modules.h"
#include "sc_config.h"
#include "debug.h"
#include "libcsoap/soap-client.h"

#ifdef __MINGW__
#define sem_init    CreateSemaphore
#define sem_wait    WaitForSingleObject
#define sem_post    ReleaseSemaphore
#define sem_destroy CloseHandle
#endif
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
// variables to store received data and status from canometer in sendrec operations
static canometroweb_data_t cw_data;
static canometroweb_config_t cw_config;
 // semaphore used to block sendrec while data is being sent or receive,
 // to make sure that any operation from/to chrono is atomic
static sem_t sem;

/**
 * Parses an xml response string with canometer configuration
 * @param pt where to store data. if null create space by calloc
 * @param xml string to be xml parsed
 * @return pointer to readed configuration
 */
static canometroweb_config_t *parse_config_xml(canometroweb_config_t *pt,char *xml) {
    xmlNode *root=NULL;
    xmlChar *attr=NULL;
    xmlDoc *doc=NULL;
    if (!pt) pt=calloc(1,sizeof(canometroweb_config_t));
    if (!pt)  { debug(DBG_ERROR,"parse_config::calloc()"); return NULL; };
    doc=xmlReadMemory(xml, strlen(xml), "cpmfog.xml", NULL, 0);
    if (!doc) { debug(DBG_ERROR,"XML Parser: cannot compose tree"); return NULL; }
    root = xmlDocGetRootElement(doc);
    if (root == NULL) { debug(DBG_ERROR,"XML: Empty document received"); return NULL; }
    // iterate inside "<xml> node
    for (xmlNode *cur_node = root->xmlChildrenNode; cur_node; cur_node = cur_node->next) {
        xmlChar *value;
        if(cur_node->type != XML_ELEMENT_NODE ) continue;
        attr=xmlNodeListGetString(doc,cur_node->xmlChildrenNode,1);
        // debug(DBG_TRACE,"node name:%s value:%s",cur_node->name,attr);
        if (!strcmp(cur_node->name,"Brighness")) { xmlFree(pt->brightness); pt->brightness=(char*)attr; }
        else if (!strcmp(cur_node->name,"Precision")) { xmlFree(pt->precision); pt->precision=(char*)attr; }
        else if (!strcmp(cur_node->name,"Guardtime")) { pt->ring=atoi((char*)attr); xmlFree(attr); }
        else if (!strcmp(cur_node->name,"Walktime")) { pt->ring=atoi((char*)attr); xmlFree(attr); }
        else if (!strcmp(cur_node->name,"Walkstyle")) { xmlFree(pt->walkstyle); pt->walkstyle=(char*)attr; }
        else if (!strcmp(cur_node->name,"Ring")) { pt->ring=atoi((char*)attr); xmlFree(attr); }
    }
    xmlFreeDoc(doc);
    return pt;
}

/**
 * Parses an xml response string with canometer status/data
 * @param pt where to store data. if null create space by calloc
 * @param xml string to be xml parsed
 * @return pointer to readed data
 */
static canometroweb_data_t *parse_status_xml(canometroweb_data_t *pt,char *xml) {
    xmlNode *root=NULL;
    xmlChar *attr=NULL;
    xmlDoc *doc=NULL;
    if (!pt) pt=calloc(1,sizeof(canometroweb_data_t));
    if (!pt)  { debug(DBG_ERROR,"parse_data::calloc()"); return NULL; };
    debug(DBG_TRACE,"before: %lu",pt->millistime);
    doc=xmlReadMemory(xml, strlen(xml), "data.xml", NULL, 0);
    if (!doc) { debug(DBG_ERROR,"XML Parser: cannot compose tree"); return NULL; }
    root = xmlDocGetRootElement(doc);
    if (root == NULL) { debug(DBG_ERROR,"XML: Empty document received"); return NULL; }
    // iterate inside "<xml> node
    for (xmlNode *cur_node = root->xmlChildrenNode ; cur_node; cur_node = cur_node->next) {
        xmlChar *value;
        if(cur_node->type != XML_ELEMENT_NODE ) continue;
        attr=xmlNodeListGetString(doc,cur_node->xmlChildrenNode,1);
        // debug(DBG_TRACE,"node name:%s value:%s",cur_node->name,attr);
        if (!strcmp(cur_node->name,"millistime")) { pt->millistime=atoi((char*)attr); xmlFree(attr); }
        else if (!strcmp(cur_node->name,"tiempoactual")) { pt->tiempoactual=atol((char*)attr); xmlFree(attr); }
        else if (!strcmp(cur_node->name,"cronocorriendo")) { pt->cronocorriendo=atoi((char*)attr); xmlFree(attr); }
        else if (!strcmp(cur_node->name,"faltas")) { pt->faltas=atoi((char*)attr); xmlFree(attr); }
        else if (!strcmp(cur_node->name,"rehuses")) { pt->rehuses=atoi((char*)attr); xmlFree(attr); }
        else if (!strcmp(cur_node->name,"eliminado")) { pt->eliminado=atoi((char*)attr); xmlFree(attr); }
        else if (!strcmp(cur_node->name,"cuentaresultados")) { pt->cuentaresultados=atoi((char*)attr); xmlFree(attr); }
        else if (!strcmp(cur_node->name,"versionresultados")) { pt->versionresultados=atoi((char*)attr); xmlFree(attr); }
    }
    xmlFreeDoc(doc);
    debug(DBG_TRACE,"after: %lu",pt->millistime);
    return pt;
}

/* methods to handle callback for curl request */
struct string {
    char *ptr;
    size_t len;
};

static void init_string(struct string *s) {
    s->len = 0;
    s->ptr = malloc(s->len+1); // notice: assume won't fail. should be checked
    s->ptr[0] = '\0';
}

static size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s) {
    size_t new_len = s->len + size*nmemb;
    s->ptr = realloc(s->ptr, new_len+1); // notice: assume won't fail. should be checked
    memcpy(s->ptr+s->len, ptr, size*nmemb);
    s->ptr[new_len] = '\0';
    s->len = new_len;
    return size*nmemb;
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
static char *sendrec(char *page,char *tipo,char *valor,int wantresponse) {
    // connection related vars
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;
    // data for response
    char result[1024];

    // data for post request
    char tmp[256]; // to evaluate temporary header data
    char url[256];
    char postdata[256];
    memset(tmp,0,sizeof(tmp));
    memset(url,0,sizeof(url));
    memset(postdata,0,sizeof(postdata));
    memset(result,0,sizeof(result));
    snprintf(url,254,"http://%s/%s",config->comm_ipaddr,page);
    if (tipo && valor) {
        snprintf(postdata,255,"tipo=%s&valor=%s",tipo,valor);
    }
    debug(DBG_TRACE,"sendrec() url: '%s' postdata: '%s'",url,postdata);
    curl = curl_easy_init();

    if (curl) {
        struct string s;
        init_string(&s);
        headers = curl_slist_append(headers, "Content-Type: text/plain");
        headers = curl_slist_append(headers, "Accept: application/xml");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata); /* data goes here */
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(postdata));
        if (wantresponse) {
            /* we want to use our own read function */
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        }
        res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return s.ptr; // insted of strdup() + free() just return evaluated string
    }
    // arriving here means error
    debug(DBG_ERROR,"sendrec::curl_init() failed");
    return NULL;
}

/* Declare our Add function using the above definitions. */
int ADDCALL module_init(configuration *cfg) {
    config = cfg;
    /* initialize semaphores */
    sem_init(&sem,0,1);
    /* In windows, this will init the winsock stuff */
    CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
    /* Check for errors */
    if(res != CURLE_OK) {
        debug(DBG_ERROR, "curl_global_init() failed: %s\n", curl_easy_strerror(res));
        return -11;
    }
    // check connection against Canometer by mean of trying to receive configuration
    char *check=sendrec("config_xml",NULL,NULL,1);
    if(!check) {
        debug(DBG_ERROR,"Cannot contact with Canometer. Abort thread");
        return -1;
    }
    // finally prepare soap subsystem
    status = soap_client_init_args(0, NULL); // currently no arguments required
    if (status != H_OK) {
        debug(DBG_ERROR,"SoapInit() %s():%s [%d]", herror_func(status), herror_message(status),  herror_code(status));
        herror_release(status);
        return -1;
    }
    return 0;
}

int ADDCALL module_end() {
    curl_global_cleanup();
    sem_destroy(&sem);
    return 0;
}

int ADDCALL module_open(){
    char *check;
    char ring[4];
    snprintf(ring,4,"%d",config->ring);
    check=sendrec("configuracion_accion","Ring",ring,0);
    if(!check) return-1;
    // send reset
    check=sendrec("canometro_accion","0","0",0);
    if(!check) return -1;
    // retrieve configuracion
    check=sendrec("config_xml",NULL,NULL,1);
    if(!check) return  -1;
    parse_config_xml(&cw_config,check);
    // retrieve status
    check=sendrec("xml",NULL,NULL,1);
    if(!check) return  -1;
    parse_status_xml(&cw_data,check);
    return 0;
}

int ADDCALL module_close(){
    debug(DBG_NOTICE,"Closing network chronometer module");
    return 0;
}

int ADDCALL module_read(char *buffer,size_t length){
    char *received;
    unsigned int mask=0;

    debug(DBG_TRACE,"module_read() enter");
    sem_wait(&sem);
    received=sendrec("xml",NULL,NULL,1);
    if (!received) {
        debug(DBG_ERROR,"network_read() error %s",error_str);
        sem_post(&sem);
        return 0; /* strlen(buffer); */
    }
    debug(DBG_TRACE,"canometroweb module_read() received '%s'",received);
    // parse xml and compare with stored data
    canometroweb_data_t *data=parse_status_xml(NULL,received);
    if (!data) {
        debug(DBG_ERROR,"canometroweb module_read() parsexml failed '%s'",received);
        sem_post(&sem);
        debug(DBG_TRACE,"module_read(data) exit");
        return -1;
    }
    free(received); // no longer needed

    snprintf(buffer,length,""); // default: no new data

    // compare data and generate internal commands
    // notice that cannot generate more than one message at a time,
    // so if several parameters change, parse them in next iteration


    // handle start/stop data
    // create a bitmap with current and received status
    mask=0;
    //        stored            received
    //lcorriento lactual ccorriendo cactual
    mask |= (cw_data.cronocorriendo==0)?8:0;
    mask |= (cw_data.tiempoactual==0)?0:4;
    mask |= (data->cronocorriendo==0)?2:0;
    mask |= (data->tiempoactual==0)?0:1;
    switch(mask) {
        case 0x00:
        case 0x01:
            //     0        0        0         0   => parado sin cambios                ignorar
            //     0        0        0         y   => invalido                          ignorar
            break;
        case 0x02:
        case 0x03:
            //     0        0        1         0   => justo ahora comienza a correr     START 0
            //     0        0        1         y   => ha comenzado a correr             START 0 (¿guardar tiempo?)
            snprintf(buffer,length,"START 0");
            break;
        case 0x04:
            //     0        x        0         0   => reset tras parada                 RESET
            snprintf(buffer,length,"RESET");
            break;
        case 0x05:
            //     0        x        0         y   => invalido: sincronizar             ignore
            break;
        case 0x06:
        case 0x07:
            //     0        x        1         0   => justo ahora empieza a correr tras parada     START 0
            //     0        x        1         y   => ha empezado a correr tras parada             START 0
            snprintf(buffer,length,"START 0");
            break;
        case 0x08:
            //     1        0        0         0   => reset                             RESET
            snprintf(buffer,length,"RESET");
            break;
        case 0x09:
        case 0x0a:
        case 0x0b:
            //     1        0        0         y   => invalido                          ignorar
            //     1        0        1         0   => no debería poder ocurrir          ignorar
            //     1        0        1         y   => comenzo a correr en anterior poll ignorar
            break;
        case 0x0c:
            //     1        x        0         0   => recibido reset mientras en marcha RESET
            snprintf(buffer,length,"RESET");
            break;
        case 0x0d:
            //     1        x        0         y   => parada crono                      STOP Y
            snprintf(buffer,length,"STOP %lu",data->tiempoactual);
            break;
        case 0x0e:
            //     1        x        1         0   => restart crono                     START 0
            snprintf(buffer,length,"START 0");
            break;
        case 0x0f:
            //     1        x        1         y   => si x!=y crono corriendo           ignore
            //                                     => si x==y marca tiempo intermedio   INT Y
            if (cw_data.tiempoactual==data->tiempoactual) snprintf(buffer,length,"INT %lu",data->tiempoactual);
            break;
        default:
            debug(DBG_ERROR,"invalid state mask %d",mask);
    }
    // store data into local variables
    cw_data.tiempoactual=data->tiempoactual;
    cw_data.cronocorriendo=data->cronocorriendo;
    // if need to generate command, just do it and return
    if (strlen(buffer)!=0) return strlen(buffer);

    // arriving here means that now comes handling of f:t:r data
    if ( (cw_data.faltas != data->faltas) || (cw_data.rehuses != data->rehuses) || (cw_data.eliminado!=data->eliminado) ) {
        cw_data.faltas=data->faltas;
        cw_data.rehuses=data->rehuses;
        cw_data.eliminado=data->eliminado;
        free(data);
        snprintf(buffer,length,"DATA %d:%d:%d",cw_data.faltas,cw_data.rehuses,cw_data.eliminado);
        sem_post(&sem);
        return strlen(buffer);
    }
    usleep(500000); // wait 0.5segs
    // to report crono count state, evaluate and send proper start/stop comand
    // pending: recognize and parse intermediate time and sensor fail/failbak
    sem_post(&sem);
    debug(DBG_TRACE,"module_read(crono) exit");
    return 0;
}

int ADDCALL module_write(char **tokens, size_t ntokens){

    char *result=NULL;
    char *cmd=tokens[1];

    char page[64];
    char tipo[16];
    char valor[16];
    memset(page,0,64);
    memset(tipo,0,16);
    memset(valor,0,16);

    sem_wait(&sem); // make sure that do not conflict with receiver

    debug(DBG_TRACE,"module_write() enter");
    // { 0, "start",   "Start of course run",             "[miliseconds] {0}"},
    if (strcasecmp("start",cmd)==0)  {
        // si el cronometro esta corriendo no hacer nada
        // else mandamos la orden de startstop para indicar arranque manual
        if (cw_data.cronocorriendo==0) {
            snprintf(page,sizeof(page),"startstop");
        }
    }
    // { 1, "int",     "Intermediate time mark",          "<miliseconds>"},
    // canometro web interface has no "intermediate time command"  but supported in event api protocol :-(
    else if (strcasecmp("int",cmd)==0) { unsuported("canometroweb","int"); }
    // { 2, "stop",    "End of course run",               "<miliseconds>"},
    else if (strcasecmp("stop",cmd)==0) {
        // si crono parado no hacer nada; else mandamos orden de parada manual
        if (cw_data.cronocorriendo==0) {
            snprintf(page,sizeof(page),"startstop");
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
            snprintf(page,sizeof(page),"canometro_accion");
            snprintf(tipo,sizeof(tipo),"0"); // reset cmd
            snprintf(valor,sizeof(valor),"0");
        } else {
            // if current course time differs to provided, set cronometer
            if (cw_config.walktime!=minutes) {
                cw_config.walktime=minutes;
                snprintf(page,sizeof(page),"configuracion_accion");
                snprintf(tipo,sizeof(tipo),"Walktime");
                snprintf(valor,sizeof(valor),"%d",minutes);
                sendrec(page,tipo,valor,0);
            }
            // and fireup course walk
            snprintf(page,sizeof(page),"canometro_accion");
            snprintf(tipo,sizeof(tipo),"W");
            snprintf(valor,sizeof(valor),"0");
        }
    }
    // { 7, "down",    "Start 15 seconds countdown",      ""},
    else if (strcasecmp("down",cmd)==0) {
        snprintf(page,sizeof(page),"canometro_accion");
        snprintf(tipo,sizeof(tipo),"C");
        snprintf(valor,sizeof(valor),"0");
    }
    // { 8, "fault",   "Mark fault (+/-/#)",              "< + | - | num {+}>"},
    else if (strcasecmp("fault",cmd)==0) {
        int ft=config->status.faults+config->status.touchs;
        snprintf(page,sizeof(page),"canometro_accion");
        snprintf(tipo,sizeof(tipo),"F");
        snprintf(valor,sizeof(valor),"%d",ft);
    }
    // { 9, "refusal", "Mark refusal (+/-/#)",            "< + | - | num {+}>"},
    else if (strcasecmp("refusal",cmd)==0) {
        snprintf(page,sizeof(page),"canometro_accion");
        snprintf(tipo,sizeof(tipo),"R");
        snprintf(valor,sizeof(valor),"%d",config->status.refusals);
    }
    // { 10, "elim",    "Mark elimination [+-]",          "[ + | - ] {+}"},
    // PENDING: revise eliminated behavior. if needed, just compare internal and xmldata to handle
    else if (strcasecmp("elim",cmd)==0) {
        snprintf(page,sizeof(page),"canometro_accion");
        snprintf(tipo,sizeof(tipo),"E");
        snprintf(valor,sizeof(valor),"%d",config->status.eliminated);
    }
    // { 11, "data",    "Set course fault/ref/disq info", "<faults>:<refulsals>:<disq>"},
    else if (strcasecmp("data",cmd)==0) {
        int ft=config->status.faults+config->status.touchs;
        snprintf(page,sizeof(page),"canometro_accion");
        snprintf(tipo,sizeof(tipo),"F");
        snprintf(valor,sizeof(valor),"%d",ft);
        sendrec(page,tipo,valor,0);
        snprintf(tipo,sizeof(tipo),"R");
        snprintf(valor,sizeof(valor),"%d",config->status.refusals);
        sendrec(page,tipo,valor,0);
        // PENDING: revise eliminated behavior.
        // if needed, just compare internal and xmldata to handle
        snprintf(tipo,sizeof(tipo),"E");
        snprintf(valor,sizeof(valor),"%d",config->status.eliminated);
    }
    // { 12, "reset",  "Reset chronometer and countdown", "" },
    else if (strcasecmp("reset",cmd)==0) {
        snprintf(page,sizeof(page),"canometro_accion");
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
        char *result=sendrec("config_xml",NULL,NULL,1);
        if (!result) {
            debug(DBG_ERROR,"cannot retrieve canometroweb configuration");
            sem_post(&sem); // release lock
            return -1;
        }
        parse_config_xml(&cw_config,result);
        free(result);
        debug(DBG_TRACE,"module_write(config) exit");
        sem_post(&sem); // release lock
        return 0;
    }
    // { 19, "status", "Show Fault/Refusal/Elim state",   "" },
    //  not real use, just to force read an parse canometer data
    else if (strcasecmp("status",cmd)==0) {
        result=sendrec("xml",NULL,NULL,1);
        if (!result) {
            debug(DBG_ERROR,"cannot retrieve canometroweb status");
            sem_post(&sem); // release lock
            return -1;
        }
        parse_status_xml(&cw_data,result);
        free(result);
        debug(DBG_TRACE,"module_write(status) exit");
        sem_post(&sem); // release lock
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
        sem_post(&sem); // release lock
        return 0;
    }

    // so that's allmost all done, just sending command to serial port....
    // notice "blocking" mode. needed as canometroweb does not support full duplex communications
    debug(DBG_TRACE,"calling to sendrec(), command:'%s' page:'%s' tipo:'%s' valor:'%s'",tokens[1],page,tipo,valor);
    if (strcmp(tipo,"")==0)result=sendrec(page,NULL,NULL,1);
    else result=sendrec(page,tipo,valor,0);
    if (!result) {
        debug(DBG_ERROR,"call to sendrec() failed, command:'%s' page:'%s' tipo:'%s' valor:'%s'",tokens[1],page,tipo,valor);
        sem_post(&sem); // release lock
        return -1;
    }
    debug(DBG_TRACE,"module_write(%s) exit",tokens[1]);
    free(result);
    sem_post(&sem); // release lock
    return 0;
}

char * ADDCALL module_error() {
    return error_str;
}