//
// Created by jantonio on 5/05/19.
//

#define _GNU_SOURCE

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>
#ifdef __WIN32__
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#endif

#define AGILITYCONTEST_SERIALCHRONOMETER_MAIN_C

#include "sc_tools.h"
#include "sc_sockets.h"
#include "getopt.h"
#include "ini.h"

#include "libserialport.h"
#include "main_mgr.h"
#include "web_mgr.h"
#include "ajax_mgr.h"
#include "serial_mgr.h"
#include "net_mgr.h"
#include "console_mgr.h"
#include "qrcode_mgr.h"
#include "debug.h"
#include "sc_config.h"
#include "main.h"
#include "parser.h"
#include "license.h"


int sc_thread_create(int index,char *name,configuration *config,void *(*handler)(void *config)) {
    char buff[32];
    memset(buff,0,32);
    sc_thread_slot *slot=&sc_threads[index];
    slot->index=index;
    slot->tname=strdup(name);
    if (strlen(name)>16)slot->tname[15]='\0'; // stupid mingw has no strndup(name,len)
    slot->config=config;
    slot->handler=handler;
    int res=pthread_create( &slot->thread, NULL, slot->handler, &slot->index);
    if (res<0) {
        debug(DBG_ERROR,"Cannot create and start posix thread");
        slot->index=-1;
        return -1;
    }
#ifndef __APPLE__
    res=pthread_setname_np(slot->thread,slot->tname);
    if (res!=0) debug(DBG_NOTICE,"Cannot set thread name: %s",slot->tname);
#else
    sleep(1); // let thread set itself their name
#endif
    res=pthread_getname_np(slot->thread,buff,32);
    debug(DBG_TRACE,"getname returns %d thread name:%s",res,buff);
    return 0;
}

/**
 * Print usage
 * @return 0
 */
static int usage() {
    fprintf(stderr,"%s command line options:\n",program_name);
    fprintf(stderr,"Serial parameters:\n");
    fprintf(stderr,"\t -m module   || --module=module_name Chronometer module to be used. Default \"generic\"\n");
    fprintf(stderr,"\t -i ipaddr   || --ipaddr=com_ipaddr IP address for networked chronometers (required on net chrono)\n");
    fprintf(stderr,"\t -d commport || --commport=com_port Communication port to attach to (required on serial chrono) \n");
    fprintf(stderr,"\t -b baudrate || --baudrate=baudrate Set baudrate for comm port. Defaults 9600\n");
    fprintf(stderr,"\t -Q qrport   || --qrport=baudrate   Port to read QRCode dorsal entry info. Default: none\n");
    fprintf(stderr,"Web interface:\n");
    fprintf(stderr,"\t -w webport || --webport=web_port   Where to listen for html interface. 0:disable . Default 8080\n");
    fprintf(stderr,"AgilityContest  interface:\n");
    fprintf(stderr,"\t -s ipaddr  || --ac_server=ipaddr   Location (IP) of AgilityContest server.\n");
    fprintf(stderr,"                                      Values: \"none\":disable - \"find\":search - Default: \"localhost\"\n");
    fprintf(stderr,"\t -n name    || --ac_name=name       Chrono name sent to AgilityContest. Defaults to 'module_name@ring'\n");
    fprintf(stderr,"\t -r ring    || --ring=ring_number   Tell server which ring to attach chrono. Default \"1\"\n");
    fprintf(stderr,"Debug options:\n");
    fprintf(stderr,"\t -D level   || --loglevel=level     Set debug/logging level 0:none thru 8:all. Defaults to 3:error\n");
    fprintf(stderr,"\t -L file    || --logfile=filename   Set log file. Defaults to \"stderr\"\n");
    fprintf(stderr,"\t -c         || --console            open cmdline console and enter in interactive (no-daemon) mode\n");
    fprintf(stderr,"\t -v         || --verbose            Send debug to stderr console\n");
    fprintf(stderr,"\t -q         || --quiet              Do not send debug log to console\n");
    fprintf(stderr,"Additional options:\n");
    fprintf(stderr,"\t -f         || --find-ports         Show available , non-busy comm ports and exit\n");
    fprintf(stderr,"\t -h  || -?  || --help               Display this help and exit\n");
    return 0;
}


/**
 * Parse command line options.
 * Use a minimal library. long options not supported
 * @param config pointer to currenc configuration options
 * @param argc cmd line argument count
 * @param argv cmd line argument pointers
 * @return 0 on success; -1 on error
 */
static int parse_cmdline(configuration *config, int argc,  char * const argv[]) {
    int option=0;
    int opt_index=0;
    static struct option long_options[] =
    {
            {"module", required_argument, NULL, 'm'},
            {"ipaddr", required_argument, NULL, 'i'},
            {"commport", required_argument, NULL, 'd'},
            {"qrport", required_argument, NULL, 'Q'},
            {"baudrate", required_argument, NULL, 'b'},
            {"webport", required_argument, NULL, 'w'},
            {"ac_server", required_argument, NULL, 's'},
            {"ac_name", required_argument, NULL, 'n'},
            {"ring", required_argument, NULL, 'r'},
            {"loglevel", required_argument, NULL, 'D'},
            {"logfile", required_argument, NULL, 'L'},
            {"verbose", no_argument, NULL, 'v'},
            {"quiet", no_argument, NULL, 'q'},
            {"find_ports", no_argument, NULL, 'f'},
            {"help", no_argument, NULL, 'h'},
            {NULL, 0, NULL, 0}
    };
    while ((option = getopt_long(argc, argv,"i:m:n:d:Q:w:s:L:D:b:r:vqhcf",long_options,&opt_index)) != -1) {
        switch (option) {
            case 'm' : config->module = strdup(optarg);     break;
            case 'i' : config->comm_ipaddr = strdup(optarg);  break;
            case 'd' : config->comm_port = strdup(optarg);  break;
            case 'Q' : config->qrcomm_port = strdup(optarg);  break;
            case 'b' : config->baud_rate = atoi(optarg);    break; // pending: check valid baudrate
            case 'w' : config->web_port = atoi(optarg);     break; // port used will be web_port+ring
            case 's' : config->ajax_server = strdup(optarg);break;
            case 'n' : config->client_name = strdup(optarg);break;
            case 'r' : config->ring = atoi(optarg);         break;
            case 'D' : config->loglevel = atoi(optarg)%9;   break;
            case 'L' : config->logfile = strdup(optarg);    break;
            case 'v' : config->verbose = 1; break;
            case 'q' : config->verbose = 0; break; // no console output
            case 'c' : config->opmode |= OPMODE_CONSOLE;    break; // test serial port
            case 'f' : config->opmode = OPMODE_FIND;        break; // find serial ports
            case 'h' :
            case '?' : usage(); exit(0);
            default: return -1;
        }
    }
    return 0;
}

int main (int argc, char *argv[]) {
#ifdef __WIN32__
    WORD versionWanted = MAKEWORD(1, 1);
    WSADATA wsaData;
    WSAStartup(versionWanted, &wsaData);
#endif
    program_name=argv[0];

    configuration *config;
    // early init debug with default values
    debug_init(NULL);

    // parse default options
    debug(DBG_TRACE,"Setting defaults");
    config=default_options(NULL);
    if (!config) { return 1; }

    // parse configuration file options
    debug(DBG_TRACE,"Parsing configuration file");
    config=parse_ini_file(config,"serial_chrono.ini");
    if (!config) { return 1; }

    // parse command line options
    if ( parse_cmdline(config,argc,argv)<0) {
        debug(DBG_ERROR,"error parsing cmd line options");
        usage();
        return 1;
    }

    // re-enable debug, with final options
    if ( debug_init(config)!=0 ) {
        debug(DBG_ALERT,"Cannot create log file. Using stderr");
    }

    // fix client name to replace non alphanum chars to '_'
    for (char *p=config->client_name; *p; p++) if (!isalnum(*p) || !isascii(*p) ) *p='_';
    print_configuration(config);

    // if opmode==enumerate, do nothing but search and print available serial ports
    if (config->opmode & OPMODE_FIND) {
        serial_print_ports(config);
        return 0;
    }

    // retrieve license file and data
    if (readLicenseFromFile(config)<0) {
        debug(DBG_ERROR,"Error in handle of license file");
        return 1;
    }
    char *serial=getLicenseItem("serial");
    char *club=getLicenseItem("club");
    char *options=getLicenseItem("options");
    debug(DBG_INFO,"License number:'%s' Registerd to:'%s' permissions:'%s'",serial,club,options);
    long lic_options=strtol(options,NULL,2);
    if ( (lic_options & 0x00000840L) == 0) {
        debug(DBG_ERROR,"Current license does not support Chronometer operations");
        return 1;
    }
    if(serial) free(serial);
    if(club) free(club);
    if(options) free(options);

    // start requested threads
    // we need 5+1 threads (console,serial,ajax,web) managers plus webserver
    sc_threads = calloc(6,sizeof(sc_thread_slot));
    if (!sc_threads) {
        debug(DBG_ERROR,"Error allocating thread data space");
        return 1;
    }
    for (int n=0;n<6;n++) sc_threads[n].index=-1;

    // Thread 0: interactive console
    if (config->opmode & OPMODE_CONSOLE) {
        debug(DBG_TRACE,"Starting interactive console thread");
        sc_thread_create(0,SC_CONSOLE,config,console_manager_thread);
    }
    // thread 1: recepcion de datos por puerto serie
    if ( (config->comm_port != (char*)NULL )  && (strcmp(config->module,"canometroweb")!=0) ) {
        config->opmode |= OPMODE_NORMAL;
        debug(DBG_TRACE,"Starting comm port receiver thread");
        sc_thread_create(1,SC_SERIAL,config,serial_manager_thread);
    }
    //  on networked chronometers replace serial with network thread
    if ((config->comm_ipaddr != (char*)NULL ) && (strcmp(config->module,"canometroweb")==0) )  {
        config->opmode |= OPMODE_NORMAL;
        debug(DBG_TRACE,"Starting comm port (networked) receiver thread");
        sc_thread_create(1,SC_NETWORK,config,network_manager_thread);
    }
    // thread 2: gestion de mini-servidor we 0b
    if (config->web_port !=0 ) {
        config->opmode |= OPMODE_WEB;
        debug(DBG_TRACE,"Starting internal html server thread");
        sc_thread_create(2,SC_WEBSRV,config,web_manager_thread);
    }
    // thread 3: comunicaciones ajax con servidor AgilityContest
    if (strcasecmp("none",config->ajax_server)==0) config->ajax_server=NULL;
    if ( (lic_options & 0x00000040L) == 0) {
        debug(DBG_NOTICE,"Current license does not support AgilityContest event API connection");
        config->ajax_server=NULL;
    }
    if (config->ajax_server!= (char*)NULL) {
        config->opmode |= OPMODE_SERVER;
        debug(DBG_TRACE,"Starting AgilityContest event listener thread");
        sc_thread_create(3,SC_AJAXSRV,config,ajax_manager_thread);
    }
    // thread 4 QRCode Dorsal reader
    if ( strcasecmp(config->qrcomm_port,"none")!=0 )  {
        config->opmode |= OPMODE_QRCODE;
        debug(DBG_TRACE,"Starting qrcode reader receiver thread");
        sc_thread_create(4,SC_QRCODE,config,qrcode_manager_thread);
    }
    // ok. start socket server on port "base"+"ring" // to allow multiple instances

    // create sock
    char portstr[16];
    snprintf(portstr,16,"%d",config->local_port+config->ring);
    int sock = passiveUDP(portstr);
    if (sock < 0) {
        debug(DBG_ERROR,"Could not create socket to listen for commands");
        return -1;
    }


    // run until exit command received
    int loop=1;
    while (loop) {
        char *response="OK";
        // socket address used to store client address
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);
        memset(&client_address,0,client_address_len);
        char buffer[500];
        int ntokens=0;
        // read content into buffer from an incoming client
        int len = recvfrom(sock, buffer, sizeof(buffer), 0,(struct sockaddr *)&client_address,&client_address_len);
        if( len<0) {
            debug(DBG_ERROR,"recvfrom error: %s",strerror(errno));
            continue;
        }
        // inet_ntoa prints user friendly representation of the ip address
        buffer[len] = '\0';
        // debug(DBG_TRACE,"Main loop: received: '%s'",buffer);
        // tokenize received message
        char **tokens=explode(buffer,' ',&ntokens);
        if (!tokens) {
            debug(DBG_ERROR,"Cannot tokenize received data '%s'",buffer);
            response="ERROR";
            goto free_and_response;
        }
        if (ntokens<2) {
            debug(DBG_ERROR,"Invalid message length received data '%s'",buffer);
            response="ERROR";
            goto free_and_response;
        }
        if (strcmp(tokens[1],"")==0) { // empty command
            debug(DBG_INFO,"received empty command from '%s'",tokens[0]);
            response="OK"; // empty command is ok ( i.e: just return on console
            goto free_and_response;
        }
        // convert command into lowercase
        for(char *pt=tokens[1];*pt;pt++) *pt=tolower(*pt);
        // search command from list to retrieve index
        int index=-1;
        for (int n=0;command_list[n].index>=0;n++) {
            // debug(DBG_TRACE,"Check command '%s' against index %d -> '%s'",tokens[1],index,command_list[index].cmd);
            if (strcmp(command_list[n].cmd,tokens[1])==0) { index=n; break; }
        }
        if (command_list[index].index<0) {
            debug(DBG_ERROR,"Unknown command received: '%s' from %s\n", buffer,tokens[0]);
            response="ERROR";
            goto free_and_response;
        }
        debug(DBG_TRACE,"Received command %d -> '%s' from %s\n", index, buffer,tokens[0]);
        // send received data to main control mgr
        if (main_mgr_entries[index]!=NULL) {
            // if function pointer is not null fire up code
            func handler=main_mgr_entries[index];
            int res=handler(config,-1,tokens,ntokens); // slot is not used in main controller thread
            if (res<0) {
                debug(DBG_ERROR,"Error sending command: '%s' from %s to main mgr\n", buffer,tokens[0]);
                // en caso de error no continuamos:
                response="main_mgr ERROR";
                goto free_and_response;
            }
        }
        // send received data to every active threads
        int alive=0;
        for (int n=0;n<4;n++) { // notice: thread 5 is not a manager, but webserver task,
            if (sc_threads[n].index<0) {
                debug(DBG_TRACE,"Thread %d is not active",n);
                continue; // skip non active threads
            }
            if (sc_threads[n].entries == NULL) {
                debug(DBG_TRACE,"Thread %d has no function entry points",n);
                continue; // no function pointers declared for current thread
            }
            // invoke parser on thread
            if (sc_threads[n].entries[index]) {
                // if function pointer is not null fire up code
                func handler=sc_threads[n].entries[index];
                int res=handler(config,n,tokens,ntokens);
                if (res<0) {
                    debug(DBG_ERROR,"Error sending command: '%s' from %s to %s\n", buffer,tokens[0],sc_threads[n].tname);
                    response="ERROR";
                }
            }
            alive++; // increase alive threads counter
        }
        // if no alive thread or data includes "exit" command, ask for end loop
        if (alive==0) {
            debug(DBG_INFO,"No alive threads");
            loop=0;
        }
        if (stripos(buffer,"exit")>=0) {
            debug(DBG_INFO,"Received exit command");
            loop=0;
        }
free_and_response:
        // liberate space reserved from tokenizer
        free(tokens);
        // send response content back to the client
        sendto(sock, response, strlen(response), 0, (struct sockaddr *)&client_address,client_address_len);
    }

    // arriving here means mark end of threads and wait them to die
    debug(DBG_TRACE,"Waiting for threads to exit");

}