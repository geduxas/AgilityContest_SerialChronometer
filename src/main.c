//
// Created by jantonio on 5/05/19.
//
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "../include/sc_tools.h"
#include "../include/sc_sockets.h"
#include "../include/getopt.h"
#include "../include/ini.h"

#include "libserialport.h"
#include "../include/web_mgr.h"
#include "../include/ajax_mgr.h"
#include "../include/serial_mgr.h"
#include "../include/console_mgr.h"
#include "../include/debug.h"
#include "../include/sc_config.h"
#include "../include/main.h"

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
    fprintf(stderr,"\t -m module  || --module=module_name Serial comm module to be used. Default \"std\"\n");
    fprintf(stderr,"\t -d comport || --device=com_port    Communication port to attach to (required) \n");
    fprintf(stderr,"\t -b baud    || --baud=baudrate      Set baudrate for comm port. Defaults 9600\n");
    fprintf(stderr,"Web interface:\n");
    fprintf(stderr,"\t -w webport || --port=web_port      Where to listen for web interface. 0:disable . Default 8080\n");
    fprintf(stderr,"AgilityContest  interface:\n");
    fprintf(stderr,"\t -s ipaddr  || --server=ip_address  Location (IP) of AgilityContest server.\n");
    fprintf(stderr,"                                      Values: \"none\":disable - \"find\":search - Default: \"localhost\"\n");
    fprintf(stderr,"\t -r ring    || --ring=ring_number   Tell server which ring to attach chrono. Default \"1\"\n");
    fprintf(stderr,"\t -n name    || --name=<name>        Set device name in API bus. Defaults to \"SerialChrono_<commport>\"\n");
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
    while ((option = getopt(argc, argv,"d:w:s:L:D:b:vqhtf")) != -1) {
        switch (option) {
            case 'd' : config->comm_port = strdup(optarg); break;
            case 'w' : config->web_port = atoi(optarg); break;
            case 's' : config->ajax_server = strdup(optarg); break;
            case 'L' : config->logfile = strdup(optarg); break;
            case 'D' : config->loglevel = atoi(optarg)%9; break;
            case 'b' : config->baud_rate = atoi(optarg); break; // pending: check valid baudrate
            case 'v' : config->verbose = 1; break;
            case 'q' : config->verbose = OPMODE_NORMAL; break;
            case 't' : config->opmode = OPMODE_TEST; break; // test serial port
            case 'f' : config->opmode = OPMODE_ENUM; break; // find serial ports
            case 'h' :
            case '?' : usage(); exit(0);
            default: return -1;
        }
    }
    return 0;
}

int main (int argc, char *argv[]) {
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

    print_configuration(config);

    // if opmode==enumerate, do nothing but search and print available serial ports
    if (config->opmode==OPMODE_ENUM) {
        serial_print_ports(config);
        return 0;
    }

    // start requested threads
    sc_threads = calloc(4,sizeof(sc_thread_slot));
    if (!sc_threads) {
        debug(DBG_ERROR,"Error allocating thread data space");
        usage();
        return 1;
    }
    // thread 0: recepcion de datos por puerto serie
    if (config->comm_port != (char*)NULL ) {
        debug(DBG_TRACE,"Starting comm port receiver thread");
        sc_thread_create(0,"SERIAL",config,serial_manager_thread);
    }
    // thread 1: gestion de mini-servidor web
    if (config->web_port !=0 ) {
        debug(DBG_TRACE,"Starting web server thread");
        sc_thread_create(1,"WEB",config,web_manager_thread);
    }
    // thread 2: comunicaciones ajax con servidor AgilityContest
    if (config->ajax_server!= (char*)NULL) {
        debug(DBG_TRACE,"Starting ajax event listener thread");
        sc_thread_create(2,"AJAX",config,ajax_manager_thread);
    }
    // Thread 3: interactive console
    if (config->opmode==OPMODE_TEST) {
        debug(DBG_TRACE,"Starting interactive console thread");
        sc_thread_create(3,"CONSOLE",config,console_manager_thread);
    }

    // ok. start socket server
    int sock = passiveUDP(config->local_port);
    if (sock < 0) {
        debug(DBG_ERROR,"could not create socket to listen commands");
        return -11;
    }

    // socket address used to store client address
    struct sockaddr_in client_address;
    unsigned int client_address_len = 0;
    char buffer[500];
    // run until exit command received
    int loop=1;
    while (loop) {
        // read content into buffer from an incoming client
        int len = recvfrom(sock, buffer, sizeof(buffer), 0,(struct sockaddr *)&client_address,&client_address_len);
        // inet_ntoa prints user friendly representation of the ip address
        buffer[len] = '\0';
        debug(DBG_TRACE,"received: '%s' from %s\n", buffer,sc_threads[atoi(buffer)].tname);
        // send received data to every active threads
        int count=0;
        for (int n=0;n<3;n++) {
            if (sc_threads[n].index==-1) continue;
            count++;
        }
        // if no alive thread or data includes "exit" command, ask for end loop
        if (count==0) loop=0;
        if (stripos(buffer,"exit")>=0) loop=0;

        // send "OK" content back to the client
        char *response="OK";
        sendto(sock, response, strlen(response), 0, (struct sockaddr *)&client_address,sizeof(client_address));
    }

    // arriving here means mark end of threads and wait them to die
    debug(DBG_TRACE,"Waiting for threads to exit");

}