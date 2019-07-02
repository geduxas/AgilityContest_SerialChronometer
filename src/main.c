//
// Created by jantonio on 5/05/19.
//
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

#include "sc_tools.h"
#include "sc_sockets.h"
#include "getopt.h"
#include "ini.h"

#include "libserialport.h"
#include "main_mgr.h"
#include "web_mgr.h"
#include "ajax_mgr.h"
#include "serial_mgr.h"
#include "console_mgr.h"
#include "debug.h"
#include "sc_config.h"
#include "main.h"
#include "parser.h"


int sc_thread_create(int index,char *name,configuration *config,void *(*handler)(void *config)) {
    sc_thread_slot *slot=&sc_threads[index];
    slot->index=index;
    slot->tname=name;
    slot->config=config;
    slot->handler=handler;
    int res=pthread_create( &slot->thread, NULL, slot->handler, &slot->index);
    if (res<0) {
        debug(DBG_ERROR,"Cannot create and start posix thread");
        slot->index=-1;
        return -1;
    }
    return 0;
}

/**
 * Print usage
 * @return 0
 */
static int usage() {
    fprintf(stderr,"%s command line options:\n",program_name);
    fprintf(stderr,"Serial parameters:\n");
    fprintf(stderr,"\t -m module  || --module=module_name Serial comm module to be used. Default \"generic\"\n");
    fprintf(stderr,"\t -d comport || --device=com_port    Communication port to attach to (required) \n");
    fprintf(stderr,"\t -b baud    || --baud=baudrate      Set baudrate for comm port. Defaults 9600\n");
    fprintf(stderr,"Web interface:\n");
    fprintf(stderr,"\t -w webport || --port=web_port      Where to listen for html interface. 0:disable . Default 8080\n");
    fprintf(stderr,"AgilityContest  interface:\n");
    fprintf(stderr,"\t -s ipaddr  || --server=ip_address  Location (IP) of AgilityContest server.\n");
    fprintf(stderr,"                                      Values: \"none\":disable - \"find\":search - Default: \"localhost\"\n");
    fprintf(stderr,"\t -n name    || --client_name=name   chrono name sent to AgilityContest. Defaults to 'module_name@ring'\n");
    fprintf(stderr,"\t -r ring    || --ring=ring_number   Tell server which ring to attach chrono. Default \"1\"\n");
    fprintf(stderr,"Debug options:\n");
    fprintf(stderr,"\t -D level   || --debuglog=level     Set debug/logging level 0:none thru 8:all. Defaults to 3:error\n");
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
    while ((option = getopt(argc, argv,"m:n:d:w:s:L:D:b:r:vqhcf")) != -1) {
        switch (option) {
            case 'm' : config->module = strdup(optarg);     break;
            case 'n' : config->client_name = strdup(optarg);break;
            case 'd' : config->comm_port = strdup(optarg);  break;
            case 'r' : config->ring = atoi(optarg);         break;
            case 'w' : config->web_port = atoi(optarg);     break; // port used will be web_port+ring
            case 's' : config->ajax_server = strdup(optarg);break;
            case 'L' : config->logfile = strdup(optarg);    break;
            case 'D' : config->loglevel = atoi(optarg)%9;   break;
            case 'b' : config->baud_rate = atoi(optarg);    break; // pending: check valid baudrate
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

    // start requested threads
    sc_threads = calloc(4,sizeof(sc_thread_slot));
    if (!sc_threads) {
        debug(DBG_ERROR,"Error allocating thread data space");
        return 1;
    }
    for (int n=0;n<4;n++) sc_threads[n].index=-1;
    // Thread 0: interactive console
    if (config->opmode & OPMODE_CONSOLE) {
        debug(DBG_TRACE,"Starting interactive console thread");
        sc_thread_create(0,SC_CONSOLE,config,console_manager_thread);
    }
    // thread 1: recepcion de datos por puerto serie
    if (config->comm_port != (char*)NULL ) {
        config->opmode |= OPMODE_NORMAL;
        debug(DBG_TRACE,"Starting comm port receiver thread");
        sc_thread_create(1,SC_SERIAL,config,serial_manager_thread);
    }
    // thread 2: gestion de mini-servidor we 0b
    if (config->web_port !=0 ) {
        config->opmode |= OPMODE_WEB;
        debug(DBG_TRACE,"Starting internal html server thread");
        sc_thread_create(2,SC_WEBSRV,config,web_manager_thread);
    }
    // thread 3: comunicaciones ajax con servidor AgilityContest
    if (strcasecmp("none",config->ajax_server)==0) config->ajax_server=NULL;
    if (config->ajax_server!= (char*)NULL) {
        config->opmode |= OPMODE_SERVER;
        debug(DBG_TRACE,"Starting AgilityContest event listener thread");
        sc_thread_create(3,SC_AJAXSRV,config,ajax_manager_thread);
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
        // search command from list to retrieve index
        int index=-1;
        for (int n=0;command_list[n].index>=0;n++) {
            // debug(DBG_TRACE,"Check command '%s' against index %d -> '%s'",tokens[1],index,command_list[index].cmd);
            if (stripos(command_list[n].cmd,tokens[1])>=0) { index=n; break; }
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
        for (int n=0;n<4;n++) {
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