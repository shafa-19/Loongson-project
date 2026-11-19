/*=============================================================================
 * udp_server.c --- board acts as UDP echo server on port 9062
 *===========================================================================*/

#include "lwip_test.h"

#if (TEST_UDP_SERVER)

#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"

static char data_buf[UDP_SERVER_BUFSIZE];

static void udp_server_thread(void *arg)
{
    struct sockaddr_in local_addr;
    struct sockaddr from;
    socklen_t fromlen;
    int sock_fd;
    int err;

    (void)arg;

    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        rt_kprintf("[udp_server] create socket failed\r\n");
        return;
    }

    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family      = AF_INET;
    local_addr.sin_len         = sizeof(local_addr);
    local_addr.sin_addr.s_addr = inet_addr(local_IP);
    local_addr.sin_port        = htons(UDP_LOCAL_PORT);

    err = bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr));
    if (err != ERR_OK) {
        rt_kprintf("[udp_server] bind failed: %d\r\n", err);
        closesocket(sock_fd);
        return;
    }
    rt_kprintf("[udp_server] bound to %s:%d, waiting for datagrams\r\n",
               local_IP, UDP_LOCAL_PORT);

    for (;;) {
        fromlen = sizeof(from);
        memset(data_buf, 0, UDP_SERVER_BUFSIZE);

        err = recvfrom(sock_fd, data_buf, UDP_SERVER_BUFSIZE - 1, 0,
                       &from, &fromlen);
        if (err > 0) {
            rt_kprintf("[udp_server] RECEIVE: %s\r\n", data_buf);

            sendto(sock_fd, data_buf, err, 0, &from, fromlen);
            rt_kprintf("[udp_server]     SEND: %s\r\n", data_buf);
        }
        else {
            delay_ms(100);              /* nothing received, poll again    */
        }
    }

    /* never reached */
    closesocket(sock_fd);
}

void udp_server_init(void)
{
    sys_thread_new("udp_server",
                   udp_server_thread,
                   NULL,
                   DEFAULT_THREAD_STACKSIZE,
                   DEFAULT_THREAD_PRIO + 1);
}

#endif /* TEST_UDP_SERVER */
