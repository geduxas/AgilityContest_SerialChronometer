//
// Created by jantonio on 20/06/19.
//

#include "modules.h"
#include "sc_config.h"
#include "debug.h"

static configuration *config;
/* Declare our Add function using the above definitions. */
int ADDCALL module_init(configuration *cfg){
    config=cfg;

    enum sp_return ret=sp_get_port_by_name(config->comm_port,&config->serial_port);
    if (ret!= SP_OK) {
        debug(DBG_ERROR,"Cannot locate serial port '%s'",config->comm_port);
        return -1;
    }
    return 0;
}
int ADDCALL module_end() {
    if (config->serial_port) {
        sp_free_port(config->serial_port);
    }
    config->serial_port=NULL;
    return 0;
}

int ADDCALL module_open(){
    if (config->serial_port) {
        enum sp_return ret = sp_open(config->serial_port,SP_MODE_READ_WRITE);
        if (ret != SP_OK) {
            debug(DBG_ERROR,"Cannot open serial port %s",config->comm_port);
            return -1;
        }
    }
    sp_set_baudrate(config->serial_port, config->baud_rate);
    return 0;
}

int ADDCALL module_close(){
    if (config->serial_port) {
        sp_close(config->serial_port);
    }
    return 0;
}

int ADDCALL module_read(char *buffer,size_t length){
    enum sp_return ret = sp_blocking_read(config->serial_port,buffer,length,0);
    return ret;
}

int ADDCALL module_write(char *buffer,size_t length){
    enum sp_return ret=sp_nonblocking_write(config->serial_port,buffer,length);
    return ret;
}
