/*=============================================================================
 * tcp_client.c --- board acts as TCP client
 *
 * Connects to PC_IP:9061, sends five timestamped messages, prints what the
 * peer echoes back and closes the connection.
 *===========================================================================*/

#include "lwip_test.h"

#if (TEST_TCP_CLIENT)

#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"

static char data_buf[TCP_CLIENT_BUFSIZE] = "hello, you are connected!\n";

static void tcp_client_thread(void *arg)
{
    struct sockaddr_in remote_addr;
    int sock_fd;
    int err;
    int count = 0;
    int rdbytes;

    (void)arg;

    /*---- 1. socket -------------------------------------------------------*/
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        rt_kprintf("[tcp_client] create socket failed\r\n");
        return;
    }

    /*---- 2. connect ------------------------------------------------------*/
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family      = AF_INET;
    remote_addr.sin_len         = sizeof(remote_addr);
    remote_addr.sin_addr.s_addr = inet_addr(remote_IP);
    remote_addr.sin_port        = htons(TCP_REMOTE_PORT);

    err = connect(sock_fd, (struct sockaddr *)&remote_addr, sizeof(remote_addr));
    if (err != ERR_OK) {
        rt_kprintf("[tcp_client] connect to %s:%d failed (%d)\r\n",
                   remote_IP, TCP_REMOTE_PORT, err);
        closesocket(sock_fd);
        return;
    }
    rt_kprintf("[tcp_client] connected to %s:%d\r\n", remote_IP, TCP_REMOTE_PORT);

    /*---- 3. five request/response round trips ----------------------------*/
    while (count < 5) {
        unsigned int ticks = get_clock_ticks();

        memset(data_buf, 0, TCP_CLIENT_BUFSIZE);
        snprintf(data_buf, TCP_CLIENT_BUFSIZE - 1, "client ticks = %u.\n", ticks);

        if (send(sock_fd, data_buf, strlen(data_buf), 0) <= 0) {
            rt_kprintf("[tcp_client] send failed\r\n");
            delay_ms(1000);
            continue;
        }
        rt_kprintf("[tcp_client] SEND: %s", data_buf);

        memset(data_buf, 0, TCP_CLIENT_BUFSIZE);
        rdbytes = recv(sock_fd, data_buf, TCP_CLIENT_BUFSIZE - 1, 0);
        if (rdbytes > 0)
            rt_kprintf("[tcp_client]   RECEIVE: %s\r\n", data_buf);
        else
            rt_kprintf("[tcp_client]   RECEIVE: nothing (%d)\r\n", rdbytes);

        count++;
        delay_ms(100);
    }

    closesocket(sock_fd);
    rt_kprintf("[tcp_client] finished 5 round trips, connection closed\r\n");
}

void tcp_client_init(void)
{
    sys_thread_new("tcp_client",
                   tcp_client_thread,
                   NULL,
                   DEFAULT_THREAD_STACKSIZE,
                   DEFAULT_THREAD_PRIO + 1);
}

#endif /* TEST_TCP_CLIENT */
