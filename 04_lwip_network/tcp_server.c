/*=============================================================================
 * tcp_server.c --- board acts as TCP server (echo)
 *
 * The server binds 192.168.1.123:9060, accepts one client at a time and
 * echoes every received block back to the sender until the client closes
 * the connection.
 *===========================================================================*/

#include "lwip_test.h"

#if (TEST_TCP_SERVER)

#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"

static char data_buf[TCP_SERVER_BUFSIZE] = "hello, I'm tcp server!\n";

static void tcp_server_thread(void *arg)
{
    struct sockaddr_in local_addr;
    struct sockaddr_in client_addr;
    socklen_t addrlen;
    int sock_fd;
    int client_fd;
    int err;

    (void)arg;

    /*---- 1. socket -------------------------------------------------------*/
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        rt_kprintf("[tcp_server] create socket failed\r\n");
        return;
    }
    rt_kprintf("[tcp_server] socket created\r\n");

    /*---- 2. bind ---------------------------------------------------------*/
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family      = AF_INET;
    local_addr.sin_len         = sizeof(local_addr);      /* lwIP extension */
    local_addr.sin_addr.s_addr = inet_addr(local_IP);
    local_addr.sin_port        = htons(TCP_LOCAL_PORT);

    err = bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr));
    if (err != ERR_OK) {
        rt_kprintf("[tcp_server] bind failed: %d\r\n", err);
        closesocket(sock_fd);
        return;
    }
    rt_kprintf("[tcp_server] bound to %s:%d\r\n", local_IP, TCP_LOCAL_PORT);

    /*---- 3. listen -------------------------------------------------------*/
    err = listen(sock_fd, 3);
    if (err != ERR_OK) {
        rt_kprintf("[tcp_server] listen failed: %d\r\n", err);
        closesocket(sock_fd);
        return;
    }
    rt_kprintf("[tcp_server] listening\r\n");

    /*---- 4. accept / echo loop -------------------------------------------*/
    for (;;) {
        rt_kprintf("[tcp_server] waiting for a client ...\r\n");

        addrlen   = sizeof(client_addr);
        client_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &addrlen);
        if (client_fd < 0) {
            rt_kprintf("[tcp_server] accept failed: %d\r\n", client_fd);
            delay_ms(200);
            continue;
        }

        rt_kprintf("[tcp_server] client %s:%d connected\r\n",
                   inet_ntoa(client_addr.sin_addr),
                   ntohs(client_addr.sin_port));
        send(client_fd, data_buf, strlen(data_buf), 0);

        for (;;) {
            memset(data_buf, 0, TCP_SERVER_BUFSIZE);

            err = recv(client_fd, data_buf, TCP_SERVER_BUFSIZE - 1, 0);
            if (err > 0) {
                rt_kprintf("[tcp_server] RECEIVE: %s\r\n", data_buf);
                send(client_fd, data_buf, err, 0);
                rt_kprintf("[tcp_server]     SEND: %s\r\n", data_buf);
            }
            else if (err == 0) {          /* peer closed the connection     */
                rt_kprintf("[tcp_server] client disconnected\r\n");
                closesocket(client_fd);
                break;
            }
            else {
                if (errno == EINTR || errno == EWOULDBLOCK) {
                    delay_ms(50);         /* transient, keep waiting        */
                    continue;
                }
                rt_kprintf("[tcp_server] recv error %d\r\n", errno);
                closesocket(client_fd);
                break;
            }
        }
    }

    /* never reached */
    closesocket(sock_fd);
}

void tcp_server_init(void)
{
    sys_thread_new("tcp_server",
                   tcp_server_thread,
                   NULL,
                   DEFAULT_THREAD_STACKSIZE,
                   DEFAULT_THREAD_PRIO + 1);
}

#endif /* TEST_TCP_SERVER */
