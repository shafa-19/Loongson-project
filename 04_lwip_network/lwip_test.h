/*=============================================================================
 * lwip_test.h --- lwIP demo configuration (Section 8)
 *
 * Exactly one role per build: set TEST_TCP_SERVER / TEST_TCP_CLIENT /
 * TEST_UDP_SERVER / TEST_UDP_CLIENT to 1 for the test you are running and
 * rebuild.  Ports are chosen so that a board and a PC can be server and
 * client interchangeably.
 *===========================================================================*/

#ifndef _LWIP_TEST_H
#define _LWIP_TEST_H

#include <rtthread.h>
#include <string.h>
#include <stdio.h>

/*------------------------- network state flags -----------------------------*/
#define LWIP_CONNECTED      0x0001
#define LWIP_SEND_DATA      0x0002
#define LWIP_NEW_DATA       0x0004

/*------------------------- addresses and ports ----------------------------*/
#define BOARD_IP            "192.168.1.123"    /* Loongson 1B, GMAC0     */
#define PC_IP               "192.168.1.111"    /* PC running the peer    */

#define TCP_LOCAL_PORT      9060               /* board-side TCP port    */
#define TCP_REMOTE_PORT     9061               /* PC-side TCP port       */
#define UDP_LOCAL_PORT      9062               /* board-side UDP port    */
#define UDP_REMOTE_PORT     9063               /* PC-side UDP port       */

extern char *local_IP;                        /* "192.168.1.123"        */
extern char *remote_IP;                       /* "192.168.1.111"        */

/*------------------------- TCP connection state ---------------------------*/
#define MYTCP_STATE_NONE        0
#define MYTCP_STATE_RECVDATA    1
#define MYTCP_STATE_CLOSED      2

typedef struct mytcp_state {
    unsigned int state;
} mytcp_state_t;

/*------------------------- role selection --------------------------------*/
#define TEST_TCP_SERVER     1
#define TEST_TCP_CLIENT     0
#define TEST_UDP_SERVER     0
#define TEST_UDP_CLIENT     0

#if (TEST_TCP_SERVER)
#define TCP_SERVER_BUFSIZE  256
extern void tcp_server_init(void);
#endif

#if (TEST_TCP_CLIENT)
#define TCP_CLIENT_BUFSIZE  256
extern void tcp_client_init(void);
#endif

#if (TEST_UDP_SERVER)
#define UDP_SERVER_BUFSIZE  256
extern void udp_server_init(void);
#endif

#if (TEST_UDP_CLIENT)
#define UDP_CLIENT_BUFSIZE  256
extern void udp_client_init(void);
#endif

#endif /* _LWIP_TEST_H */
