/*=============================================================================
 * main.c --- Section 8, network transmission with RT-Thread + lwIP 1.4.1
 *
 * LoongIDE project: C Executable, LS1B200 (LS232), SDE Lite 4.9.2 for MIPS,
 * RTOS = RT-Thread, RT-Thread component "lwip 1.4.1" checked.
 *===========================================================================*/

#include <rtthread.h>
#include "ls1b.h"

#ifdef BSP_USE_GMAC0
/* IP address handed to the lwIP port; GMAC0 of the board */
static unsigned char gmac0_ip[4] = {192, 168, 1, 123};
extern void ls1x_initialize_lwip(unsigned char *ip0, unsigned char *ip1);
#endif

#include "lwip_test.h"

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    rt_kprintf("\r\nWelcome to RT-Thread.\r\n\r\n");
    rt_kprintf("[net] board IP  : %s\r\n", local_IP);
    rt_kprintf("[net] host IP   : %s\r\n", remote_IP);

#ifdef BSP_USE_GMAC0
    /* NULL for GMAC1 means "keep the default address" */
    ls1x_initialize_lwip(gmac0_ip, RT_NULL);

#if (TEST_TCP_SERVER)
    tcp_server_init();
    rt_kprintf("[net] TCP server listening on port %d\r\n", TCP_LOCAL_PORT);
#endif

#if (TEST_TCP_CLIENT)
    tcp_client_init();
    rt_kprintf("[net] TCP client connecting to %s:%d\r\n", remote_IP, TCP_REMOTE_PORT);
#endif

#if (TEST_UDP_SERVER)
    udp_server_init();
    rt_kprintf("[net] UDP server listening on port %d\r\n", UDP_LOCAL_PORT);
#endif

#if (TEST_UDP_CLIENT)
    udp_client_init();
    rt_kprintf("[net] UDP client sending to %s:%d\r\n", remote_IP, UDP_REMOTE_PORT);
#endif
#else
    rt_kprintf("[net] BSP_USE_GMAC0 is not defined - no Ethernet port\r\n");
#endif

    return 0;
}
