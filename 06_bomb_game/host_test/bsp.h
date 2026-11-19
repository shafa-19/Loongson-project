/* host-test stub: only what the game uses */
#ifndef BSP_H_STUB
#define BSP_H_STUB
#define BSP_USE_UART5           /* both serial ports are registered */
extern void ls1x_drv_init(void);
extern void install_3th_libraries(void);
extern unsigned int get_clock_ticks(void);
extern void delay_ms(unsigned int ms);
#endif
