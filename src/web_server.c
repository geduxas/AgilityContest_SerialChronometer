/******************************************************************
*  $Id: http_server.c,v 1.4 2006/07/09 16:24:19 snowdrop Exp $
*
* CSOAP Project:  A http client/server library in C (example)
* Copyright (C) 2003  Ferhat Ayaz
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Library General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Library General Public License for more details.
*
* You should have received a copy of the GNU Library General Public
* License along with this library; if not, write to the
* Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA  02111-1307, USA.
*
* Email: hero@persua.de
******************************************************************/
#define AGILITYCONTEST_SERIALCHRONOMETER_WEB_SERVER_C

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "nanohttp/nanohttp-logging.h"
#include "nanohttp/nanohttp-server.h"
#include "nanohttp/nanohttp-base64.h"

#include "debug.h"
#include "sc_config.h"
#include "sc_tools.h"
#include "web_mgr.h"
#include "web_server.h"
#include "license.h"

#include "main.h"
static configuration *config;

/** not used, just to preserve code from original example */
static int simple_authenticator(hrequest_t *req, const char *user, const char *password) {
	debug(DBG_INFO,"logging in user='%s' password='%s'", user, password);
	if (strcmp(user, "bob")) {
		debug(DBG_NOTICE,"user '%s' unkown", user);
		return 0;
	}
	if (strcmp(password, "builder")) {
		debug(DBG_NOTICE,"wrong password");
		return 0;
	}
	return 1;
}

/** not used, just to preserve code from original example */
static void secure_service(httpd_conn_t *conn, hrequest_t *req) {
	httpd_send_header(conn, 200, "OK");
	hsocket_send(conn->sock,"<html><head><title>Secure ressource!</title></head><body><h1>Authenticated access!!!</h1></body></html>");
}

/** not used, just to preserve code from original example */
static void headers_service(httpd_conn_t *conn, hrequest_t *req) {
    hpair_t *walker;
    httpd_send_header(conn, 200, "OK");
    hsocket_send(conn->sock,"<html><head><title>Request headers</title></head><body><h1>Request headers</h1><ul>");
    for (walker=req->header; walker; walker=walker->next) {
        hsocket_send(conn->sock, "<li>");
        hsocket_send(conn->sock, walker->key);
        hsocket_send(conn->sock, " = ");
        hsocket_send(conn->sock, walker->value);
        hsocket_send(conn->sock, "</li>");
    }
    hsocket_send(conn->sock,"</ul></body></html>");
}

static void getLogo(httpd_conn_t *conn, hrequest_t *req) {
    httpd_add_header(conn,"Content-Type","image/png");
    httpd_send_header(conn, 200, "OK");
    size_t len=0;
    char *logo=getLicenseLogo(&len); // len is zero if data is base64 encoded
    char *data=logo;
    if (len==0) { // data comes base64 encoded
        len=4+strlen(logo)*3/4; // decoded lenght is 3/4 of the encoded one ( plus padding)
        data=calloc(len,sizeof(char));
        base64_decode(logo,data);
    }
    hsocket_nsend (conn->sock,data,len);
    if (data!=logo) free(logo);
    free(data);
}

/*
 * retrieve status from app
 * add events if any
 */
static void readData(httpd_conn_t *conn, hrequest_t *req) {
    extern queue_t *output_queue;
    char line[1024];
    int len=0;
    struct hpair *item=req->query;
    char *id_key=item->key; // should be "ID"
    char *id_value=item->value;
    int cmdid=atoi(id_value);
    len=sprintf(line,"{\"F\":\"%d\",\"R\":\"%d\",\"E\":\"%d\",\"D\":\"%d\"",
            config->status.faults+config->status.touchs,config->status.refusals,config->status.eliminated,config->status.numero);
    // PENDING if event: add it
    // parse htmlquery to extract last commandID
    char *msg=queue_pick(output_queue,(cmdid<=0)?-1:cmdid);
    if (msg) { // add to json data as ",Command":message"
        len += sprintf(line+len,",\"Command\":\"%s\"",msg);
        free(msg); //strdup()'d from queue
    }
    // add cmdid and on startup ring number
    len += sprintf(line+len,",\"ID\":\"%d\"",cmdid);
    if (cmdid==0) {
        len += sprintf(line+len,",\"Ring\":\"%d\"",config->ring);
    }
    // close json data
    len += sprintf(line+len,"}");
    httpd_add_header(conn,"Content-Type","application/json");
    httpd_send_header(conn, 200, "OK");
    hsocket_nsend (conn->sock,line,len);
}

/**
 * get and process commands from browser
 * arguments: ?Command=X&Value=X
 * @param conn
 * @param req
 */
static void writeData(httpd_conn_t *conn, hrequest_t *req) {
    extern queue_t *input_queue;
    struct hpair *item=req->query;
    char *cmd_key=item->key; // should be "Command"
    char *cmd_value=item->value; // command value
    char buffer[128];
    // compose incoming message and enqueue it
    sprintf(buffer,"%s",cmd_value);
    // replace every "+" with space
    for (char *pt=buffer;*pt;pt++) if (*pt=='+') *pt=' ';
    debug(DBG_TRACE,"writeData() received %s",buffer);
    queue_put(input_queue,buffer);
    // send back json OK response
    httpd_add_header(conn,"Content-Type","application/json");
    httpd_send_header(conn, 200, "OK");
    hsocket_send (conn->sock,"{\"Result\":\"OK\"}");
}

static void default_service(httpd_conn_t *conn, hrequest_t *req) {
	char line[1024];
	getcwd(line,1024);
	strcat(line,"/html");
	strcat(line,req->path);
	// DirectoryIndex None
    struct stat statbuf;
    FILE *file=NULL;
    int ok=1;
    if (stat(line, &statbuf) != 0) ok=0;
    else if ( S_ISDIR(statbuf.st_mode) ) ok=0;
    else if ( (file=fopen(line,"rb")) ==NULL ) ok=0;
	if (ok==0) {
		httpd_send_header(conn, 404, "Not found");
		hsocket_send(conn->sock, "<html> <head> <title>Invalid</title> </head> <body> <h1>Invalid or not found</h1> <div>");
		hsocket_send(conn->sock, line);
		hsocket_send(conn->sock, " can not be found" "</div>" "</body>" "</html>");
	} else {
	    // set content-type
	    char *ct="text/html";
        if (strstr(line,".json")!=NULL) ct="application/json";
        else if (strstr(line,".js")!=NULL) ct="text/javascript";
        else if (strstr(line,".css")!=NULL) ct="text/css";
        else if (strstr(line,".png")!=NULL) ct="image/png";
        else if (strstr(line,".jpg")!=NULL) ct="image/jpeg";
        else if (strstr(line,".txt")!=NULL) ct="text/plain";
        httpd_add_header(conn,"Content-Type",ct);
	    httpd_send_header(conn, 200, "OK");
		size_t sz;
		while ((sz = fread (line, 1, sizeof (line), file)) > 0) {
			hsocket_nsend (conn->sock,line,sz);
		}
		fclose(file);
	}
	return;
}


static void root_service(httpd_conn_t *conn, hrequest_t *req) {
    char line[1024];
    getcwd(line,1024);
    strcat(line,"/html/index.html");
    FILE *file=fopen(line,"rb");
    if (!file) {
        httpd_send_header(conn, 404, "Not found");
        hsocket_send(conn->sock, "<html> <head> <title>Entry Page</title> </head> <body> <h1>404 Not Found</h1> <div>");
        hsocket_send(conn->sock, line);
        hsocket_send(conn->sock, " can not be found" "</div>" "</body>" "</html>");
    } else {
        httpd_add_header(conn,"Content-Type","text/html");
        httpd_send_header(conn, 200, "OK");
        size_t sz;
        while ((sz = fread (line, 1, sizeof (line), file)) > 0) {
            hsocket_nsend (conn->sock,line,sz);
        }
        fclose(file);
    }
    return;
}

int end_webServer() {
    httpd_destroy();
    return 0;
}

void *init_webServer(void *arg) {

    int slotIndex= * ((int *)arg);
    sc_thread_slot *slot=&sc_threads[slotIndex];
    config=slot->config;
    slot->entries=NULL; // non sense as this is not a manager thread

    char *buffer=calloc(64,sizeof(char));
    snprintf(buffer,64,"%d",config->ring+config->web_port);
    char *args[4]= {
            "ac_webserver", // program name
            NHTTPD_ARG_PORT, // specify port
            buffer, // port number to listen to
            NULL
    };
	if (httpd_init(4, args)) {
		debug(DBG_ERROR, "Can not init httpd service");
		return NULL;
	}
    if (!httpd_register("/", root_service)) {
        debug(DBG_ERROR, "Can not register service");
        return NULL;
    }
    if (!httpd_register("/readData", readData)) {
        debug(DBG_ERROR, "Can not register service 'readData'");
        return NULL;
    }
    if (!httpd_register("/writeData", writeData)) {
        debug(DBG_ERROR, "Can not register service 'writeData'");
        return NULL;
    }
    if (!httpd_register("/getLogo", getLogo)) {
        debug(DBG_ERROR, "Can not register service 'getLogo'");
        return NULL;
    }
	/*
	if (!httpd_register_secure("/secure", secure_service, simple_authenticator)) {
		debug(DBG_ERROR, "Can not register secure service");
		return -1;
	}
	if (!httpd_register("/headers", headers_service)) {
		debug(DBG_ERROR, "Can not register headers service");
		return -11;
	}*/
	if (!httpd_register_default("/error", default_service)) {
		debug(DBG_ERROR, "Can not register default service");
		return NULL;
	}
    // launch browser if configured to do so
    if (config->fire_browser!=0) {
        debug(DBG_INFO, "Firing up browser....");
#ifdef __WIN32__
        snprintf(buffer,64,"http://localhost:%d/index.html",config->ring+config->web_port);
        ShellExecute(0, 0, buffer, 0, 0 , SW_SHOW );
#else
        snprintf(buffer,64,"xdg-open http://localhost:%d/index.html",config->ring+config->web_port);
        system(buffer);
#endif
    }
    free(buffer);
    // and finally start web server
    // notice that this wont return unless external stop
	if (httpd_run()) {
		debug(DBG_ERROR, "can not run httpd");
		return NULL;
	}
	return 0;
}
