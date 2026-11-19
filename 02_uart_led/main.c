/*=============================================================================
 * Computer System Comprehensive Design --- Section 6
 * Serial-port controlled running LED (UART3 + GPIO)
 *
 * Board      : Loongson 1B (LS1B200 / LS232)
 * Toolchain  : SDE Lite 4.9.2 for MIPS, -mips32 -EL -msoft-float
 * Console    : UART3 (115200 8N1), commands are terminated by CR or LF
 *
 * Command set (ASCII, case sensitive):
 *   ON n      turn LED n on (1..8) and switch all other LEDs off
 *   OFF       turn every LED off
 *   SPEED ms  set the running-LED delay, 100 <= ms <= 1000
 *   START     start the running LED
 *   STOP      stop the running LED
 *
 * The LEDs are active low: gpio_write(pin, 0) lights the LED.
 *===========================================================================*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ls1b.h"
#include "mips.h"
#include "ls1b_gpio.h"
#include "ns16550.h"

/*---------------------------------------------------------------------------
 * Hardware configuration
 *-------------------------------------------------------------------------*/
#define LED_1   9
#define LED_2   11
#define LED_3   15
#define LED_4   17
#define LED_5   20
#define LED_6   22
#define LED_7   6
#define LED_8   7

#define GPIO_OUT        0
#define LED_COUNT       8

#define MAX_SPEED       100     /* fastest running-LED period, ms          */
#define MIN_SPEED       1000    /* slowest running-LED period, ms          */
#define BUFFER_SIZE     32      /* command buffer, bytes                   */
#define DEFAULT_SPEED   500

/*---------------------------------------------------------------------------
 * Global state
 *-------------------------------------------------------------------------*/
static const int led_pins[LED_COUNT] = {
    LED_1, LED_2, LED_3, LED_4, LED_5, LED_6, LED_7, LED_8
};

static int  speed       = DEFAULT_SPEED;  /* running-LED period, ms        */
static int  current_led = 0;              /* index into led_pins[]         */
static int  is_running  = 0;              /* running-LED state machine     */
static char buffer_uart[BUFFER_SIZE];

/*---------------------------------------------------------------------------
 * Local helpers
 *-------------------------------------------------------------------------*/
static void uart_send(const char *s)
{
    ls1x_uart_write(devUART3, (void *)s, (int)strlen(s), NULL);
}

static void gpio_init(void)
{
    int i;
    for (i = 0; i < LED_COUNT; i++)
        gpio_enable(led_pins[i], GPIO_OUT);
}

static void uart_init(void)
{
    ls1x_uart_init(devUART3, NULL);
    ls1x_uart_open(devUART3, NULL);
}

/* Light exactly one LED;  index < 0 turns every LED off. */
static void led_select(int index)
{
    int i;
    for (i = 0; i < LED_COUNT; i++)
        gpio_write(led_pins[i], 1);          /* active low -> 1 = off       */

    if (index >= 0 && index < LED_COUNT)
        gpio_write(led_pins[index], 0);      /* 0 = on                      */
}

static void next_led(void)
{
    current_led = (current_led + 1) % LED_COUNT;
}

/*---------------------------------------------------------------------------
 * Command interpreter
 *-------------------------------------------------------------------------*/
static void parse_command(char *command)
{
    if (strncmp(command, "ON ", 3) == 0) {
        int n = atoi(&command[3]);
        if (n >= 1 && n <= LED_COUNT) {
            current_led = n - 1;
            is_running  = 0;                 /* manual ON stops the runner  */
            led_select(current_led);
            uart_send("OK: LED turned on\r\n");
        } else {
            uart_send("ERR: LED index must be 1..8\r\n");
        }
    }
    else if (strcmp(command, "OFF") == 0) {
        is_running = 0;
        led_select(-1);
        uart_send("OK: all LEDs off\r\n");
    }
    else if (strncmp(command, "SPEED ", 6) == 0) {
        int new_speed = atoi(&command[6]);
        if (new_speed >= MAX_SPEED && new_speed <= MIN_SPEED) {
            speed = new_speed;
            uart_send("OK: speed updated\r\n");
        } else {
            uart_send("ERR: speed must be 100..1000\r\n");
        }
    }
    else if (strcmp(command, "START") == 0) {
        is_running = 1;
        uart_send("OK: running LED started\r\n");
    }
    else if (strcmp(command, "STOP") == 0) {
        is_running = 0;
        uart_send("OK: running LED stopped\r\n");
    }
    else if (command[0] != '\0') {
        uart_send("ERR: unknown command\r\n");
    }
}

/*---------------------------------------------------------------------------
 * Entry point
 *-------------------------------------------------------------------------*/
int main(void)
{
    int  bytes_read = 0;
    int  total_read = 0;
    char byte;

    ls1x_drv_init();                /* internal + external controllers     */
    install_3th_libraries();        /* third-party libraries               */

    gpio_init();
    uart_init();
    led_select(-1);

    uart_send("\r\nLS1B LED controller ready.\r\n");
    uart_send("Commands: ON <1-8> | OFF | SPEED <100-1000> | START | STOP\r\n");
    uart_send("> ");

    for (;;) {
        bytes_read = ls1x_uart_read(devUART3, &byte, 1, 1);

        if (bytes_read > 0) {
            if (byte == '\n' || byte == '\r') {
                buffer_uart[total_read] = '\0';     /* terminate the string */
                if (total_read > 0) {
                    uart_send("\r\n");
                    parse_command(buffer_uart);      /* reply comes from it */
                }
                total_read = 0;
                uart_send("> ");
            }
            else if (total_read < BUFFER_SIZE - 1) {
                buffer_uart[total_read++] = byte;    /* echo the character  */
                ls1x_uart_write(devUART3, &byte, 1, NULL);
            }
            else {
                total_read = 0;                      /* overlong: drop it   */
                uart_send("\r\nERR: line too long\r\n> ");
            }
        }

        if (is_running) {
            led_select(current_led);
            delay_ms(speed);
            next_led();
        }
    }

    return 0;
}
