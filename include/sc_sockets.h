//
// Created by jantonio on 2/06/19.
//

#ifndef SERIALCHRONOMETER_SC_SOCKETS_H
#define SERIALCHRONOMETER_SC_SOCKETS_H

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <string.h>
#include <stdlib.h>

#ifndef	INADDR_NONE
#define	INADDR_NONE	0xffffffff
#endif	/* INADDR_NONE */

#ifndef SERIALCHRONOMETER_SC_SOCKETS_C
#define EXTERN extern
#else
#define EXTERN
#endif

EXTERN int connectsock(const char *host, const char *service, const char *transport );
EXTERN int connectTCP(const char *host, const char *service );
EXTERN int connectUDP(const char *host, const char *service );
EXTERN int passivesock(const char *service, const char *transport, int qlen);
EXTERN int passiveUDP(const char *service);
EXTERN int passiveTCP(const char *service, int qlen);

#undef EXTERN

#endif //SERIALCHRONOMETER_SC_SOCKETS_H
