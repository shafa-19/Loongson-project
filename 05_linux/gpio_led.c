/*=============================================================================
 * gpio_led.c --- user-space LED control on Linux through /dev/mem
 *
 * Optional experiment: the same eight LEDs that Section 6 drove from a
 * bare-metal program are toggled from a Linux process by mapping the GPIO
 * register block of the LS1B SoC.
 *
 *   mipsel-linux-gcc -O2 -Wall gpio_led.c -o gpio_led
 *   ./gpio_led 20            # 20 steps of the running LED
 *
 * NOTE 1: the kernel must expose /dev/mem (CONFIG_DEVMEM) and the process
 *         must run as root.  If /dev/mem is missing, use the sysfs LED class
 *         or a small kernel module instead -- see the note in README.md.
 * NOTE 2: LS1B GPIO base 0xBFD01000, one 32-bit register per group of 32
 *         pins (GPIO_CFG / GPIO_OUT / GPIO_IN / GPIO_OE at 0x00/0x10/0x20/0x30
 *         offsets, 4 bytes per group).  Verify the offsets against the
 *         LS1B user manual of your board revision before relying on them.
 *===========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define GPIO_BASE   0xBFD01000UL
#define GPIO_MAP_SIZE 0x100

/* the eight LED pins of Section 6 */
static const int led_pins[8] = {9, 11, 15, 17, 20, 22, 6, 7};

static volatile unsigned int *gpio;

static void gpio_write_bit(int pin, int value)
{
    /* one register per 32-bit group, bit (pin % 32) inside it */
    volatile unsigned int *reg = gpio + (0x10 / 4) + (pin / 32);
    unsigned int bit = 1u << (pin % 32);

    if (value)
        *reg |= bit;
    else
        *reg &= ~bit;
}

static void led_select(int index)
{
    int i;
    for (i = 0; i < 8; i++)
        gpio_write_bit(led_pins[i], 1);          /* active low: 1 = off */

    if (index >= 0 && index < 8)
        gpio_write_bit(led_pins[index], 0);      /* 0 = on */
}

int main(int argc, char **argv)
{
    int steps = 20;
    int fd, i;

    if (argc > 1)
        steps = atoi(argv[1]);

    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open /dev/mem");
        return 1;
    }

    gpio = (volatile unsigned int *)mmap(NULL, GPIO_MAP_SIZE,
                                         PROT_READ | PROT_WRITE,
                                         MAP_SHARED, fd, GPIO_BASE);
    if (gpio == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    printf("LED block mapped at %p, %d steps, 300 ms each\n", (void *)gpio, steps);

    for (i = 0; i < steps; i++) {
        led_select(i % 8);
        usleep(300 * 1000);
    }

    led_select(-1);
    munmap((void *)gpio, GPIO_MAP_SIZE);
    close(fd);
    printf("done\n");
    return 0;
}
