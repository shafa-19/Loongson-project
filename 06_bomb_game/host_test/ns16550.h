/* host-test stub: the two ports are just handles */
#ifndef NS16550_H_STUB
#define NS16550_H_STUB
#define IOCTL_NS16550_SET_MODE 1
extern void *devUART3;
extern void *devUART5;
extern int ls1x_uart_init(void *port, void *arg);
extern int ls1x_uart_open(void *port, void *arg);
extern int ls1x_uart_read(void *port, void *buf, int len, int timeout);
extern int ls1x_uart_write(void *port, void *buf, int len, int timeout);
extern int ls1x_uart_ioctl(void *port, int cmd, void *arg);
#endif
