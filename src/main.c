//
// Created by jantonio on 5/05/19.
//
#include <string.h>
#include <stdlib.h>

#include "getopt.h"
#include "ini.h"

#include "../include/web_mgr.h"
#include "../include/ajax_mgr.h"
#include "../include/serial_mgr.h"
#include "../include/debug.h"
#include "../include/sc_config.h"
#include "../include/parser.h"
#include "../include/main.h"

int sc_thread_create(sc_thread_slot *slot) {
    return 0;
}

/**
 * Print usage
 * @return 0
 */
static int usage() {
    fprintf(stderr,"%s command line options:\n",program_name);

    fprintf(stderr,"\t -d comport || --device=com_port   Communication port to attach to (required) \n");
    fprintf(stderr,"\t -w webport || --port=web_port     Where to listen for web interface. 0:disable . Default 8080\n");
    fprintf(stderr,"\t -s ipaddr  || --server=ip_address Location of AgilityContest server. Default \"localhost\"\n");
    fprintf(stderr,"\t -r ring    || --ring=ring_number  Tell server which ring to attach chrono. Default \"1\"\n");
    fprintf(stderr,"\t -D level   || --debuglog=level    Set debug/logging level 0:none thru 8:all. Defaults to 3:error\n");
    fprintf(stderr,"\t -L file    || --logfile=filename  Set log file. Defaults to \"stderr\"\n");
    fprintf(stderr,"\t -b baud    || --baud=baudrate     Set baudrate for comm port. Defaults 9600\n");
    fprintf(stderr,"\t -t         || --test              Test mode. Don't try to connect server, just check comm port\n");
    fprintf(stderr,"\t -f         || --find-ports        Show available , non-busy comm ports\n");
    fprintf(stderr,"\t -v                                (Verbose) Send debug to stderr\n");
    fprintf(stderr,"\t -q                                (Quiet) Do not send debug log to console\n");
    fprintf(stderr,"\t -h  || -?  || --help              Display this help and exit\n");
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
        int nports=0;
        char **ports= serial_ports_enumerate(config,&nports);
        if (nports==0) {
            fprintf(stdout,"No available COMM ports found:\n");
        } else {
            fprintf(stdout,"List of available COMM ports:\n");
            for (int i=0;i<nports;i++) {
                fprintf(stdout,"%s\n",ports[i]);
                free(ports[i]);
            }
            free(ports);
        }
        return 0;
    }

    // if comm port is not null, open it, and store handler in configuration
    if (config->comm_port != (char*)NULL ) {

    }

    // start requested threads
    sc_threads = calloc(3,sizeof(sc_thread_slot));
    if (!sc_threads) {
        debug(DBG_ERROR,"Error allocating thread data");
        usage();
        return 1;
    }
    // thread 0: recepcion de datos por puerto serie
    if (config->comm_port != (char*)NULL ) {
        debug(DBG_TRACE,"Starting comm port receiver thread");
        sc_threads[0].tname="COMM";
        sc_threads[0].config=config;
        sc_threads[0].sc_thread_entry=serial_manager_thread;
        sc_thread_create(&sc_threads[0]);
    }
    // thread 1: gestion de mini-servidor web
    if (config->web_port !=0 ) {
        debug(DBG_TRACE,"Starting web server thread");
        sc_threads[1].tname="WEB";
        sc_threads[1].config=config;
        sc_threads[1].sc_thread_entry=web_manager_thread;
        sc_thread_create(&sc_threads[1]);
    }
    // thread 2: comunicaciones ajax con servidor AgilityContest
    if (config->ajax_server!= (char*)NULL) {
        debug(DBG_TRACE,"Starting ajax event listener thread");
        sc_threads[2].tname = "AJAX";
        sc_threads[2].config = config;
        sc_threads[2].sc_thread_entry = ajax_manager_thread;
        sc_thread_create(&sc_threads[2]);
    }
    // if interactive mode is enabled, enter infinite loop until exit
    if (config->opmode==OPMODE_TEST) {
        debug(DBG_TRACE,"Enter in interactive (test) mode");
        char *buff=calloc(1024,sizeof(char));
        if (!buff) {
            debug(DBG_ERROR,"Cannot enter interactive mode:calloc()");
        } else {
            int res=0;
            while(res>=0) {
                char *p=fgets(buff,1023,stdin);
                if (p) res=parse_cmd(config,"console",p);
                else res=-1; // received eof from stdin
            }
        }
    }
    // arriving here means mark end of threads and wait them to die
    debug(DBG_TRACE,"Waiting for threads to exit");

}