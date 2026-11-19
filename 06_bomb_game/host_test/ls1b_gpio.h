/* host-test stub: the LED pins are tracked in an array */
#ifndef LS1B_GPIO_H_STUB
#define LS1B_GPIO_H_STUB
#define DIR_IN   1
#define DIR_OUT  0
#define GPIO_COUNT 62
extern void gpio_enable(int ioNum, int dir);
extern int  gpio_read(int ioNum);
extern void gpio_write(int ioNum, int val);
#endif
