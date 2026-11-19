/*=============================================================================
 * udp_client.c --- board acts as UDP client, sends 5 datagrams to PC:9063
 *===========================================================================*/

#include "lwip_test.h"

#if (TEST_UDP_CLIENT)

#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"

static char data_buf[UDP_CLIENT_BUFSIZE] = "this is a UDP test package";

static void udp_client_thread(void *arg)
{
    struct sockaddr_in remote_addr;
    struct sockaddr from;
    socklen_t fromlen;
    int sock_fd;
    int err;
    int count = 0;

    (void)arg;

    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        rt_kprintf("[udp_client] create socket failed\r\n");
        return;
    }

    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family      = AF_INET;
    remote_addr.sin_len         = sizeof(remote_addr);
    remote_addr.sin_addr.s_addr = inet_addr(remote_IP);
    remote_addr.sin_port        = htons(UDP_REMOTE_PORT);

    while (count < 5) {
        unsigned int ticks = get_clock_ticks();

        memset(data_buf, 0, UDP_CLIENT_BUFSIZE);
        snprintf(data_buf, UDP_CLIENT_BUFSIZE - 1, "client ticks = %u.", ticks);

        err = sendto(sock_fd, data_buf, strlen(data_buf), 0,
                     (struct sockaddr *)&remote_addr, sizeof(remote_addr));
        if (err > 0)
            rt_kprintf("[udp_client] SEND: %s\r\n", data_buf);
        else {
            delay_ms(1000);
            continue;
        }

        delay_ms(100);

        fromlen = sizeof(from);
        memset(data_buf, 0, UDP_CLIENT_BUFSIZE);
        err = recvfrom(sock_fd, data_buf, UDP_CLIENT_BUFSIZE - 1, 0,
                       &from, &fromlen);
        if (err > 0)
            rt_kprintf("[udp_client]   RECEIVE: %s\r\n", data_buf);

        if (++count >= 5)
            break;
    }

    closesocket(sock_fd);
    rt_kprintf("[udp_client] finished 5 datagrams, socket closed\r\n");
}

void udp_client_init(void)
{
    sys_thread_new("udp_client",
                   udp_client_thread,
                   NULL,
                   DEFAULT_THREAD_STACKSIZE,
                   DEFAULT_THREAD_PRIO + 1);
}

#endif /* TEST_UDP_CLIENT */
