/*
 * Copyright (C) 2020-2021 Suzhou Tiancheng Software Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Computer System Comprehensive Design -- self-designed mini project
 *
 *      BOMB DEFUSAL GAME
 *
 * Target : Loongson 1B (LS1B200 / LS232), bare metal, no RTOS
 *
 * Hardware used (all of it already present in the course project):
 *
 *   - 8 LEDs on the GPIO header, active LOW:
 *         LED 1..8 -> GPIO 9, 11, 15, 17, 20, 22, 6, 7
 *     They play the role of the eight wires of the bomb: the bomb "shows" its
 *     arming code by blinking them one by one, and the player repeats the code.
 *
 *   - Two polled serial ports:
 *         UART3 (the port required by the laboratory guide) and
 *         UART5 (the console port).  Both are accepted, the game answers on
 *         the port the player typed on, so a single terminal is enough.
 *     The terminal is the keypad of the bomb: the player types the wire
 *     numbers 1..8.
 *
 *   - The 1 ms system tick (get_clock_ticks()) drives the countdown.
 *
 * Game rules
 * ----------
 *   The bomb has to be defused in 90 seconds by cutting five fuses ("stages").
 *   For every stage the bomb blinks a code of N wire numbers, and the player
 *   has to type exactly the same numbers in the same order:
 *
 *      stage 1 -> 2 numbers, stage 2 -> 3 numbers, ... stage 5 -> 6 numbers
 *
 *   - a correct entry lights the corresponding LED briefly and moves on;
 *   - a wrong entry costs a strike and 10 seconds, three strikes blow the bomb;
 *   - SCAN (D) shows the code on the terminal but costs 15 seconds;
 *   - REPLAY (R) shows the code again on the LEDs and costs 5 seconds;
 *   - a stage cleared without any mistake in it gives 5 seconds back;
 *   - when the time reaches zero the bomb explodes (all LEDs on).
 *
 * Commands on the terminal
 * ------------------------
 *      1..8    cut wire n (enter one number per line, like the code order)
 *      D       scan: print the current code (costs 15 s)
 *      R       replay: blink the code again (costs 5 s)
 *      S       status
 *      N       new game
 *      H       help
 *      ENTER   start the game from the idle screen
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ls1b.h"
#include "mips.h"

//-------------------------------------------------------------------------------------------------
// BSP
//-------------------------------------------------------------------------------------------------

#include "bsp.h"
#include "ls1b_gpio.h"
#include "ns16550.h"
#include "termios.h"

//-------------------------------------------------------------------------------------------------
// Configuration
//-------------------------------------------------------------------------------------------------

#define LED_COUNT        8          /* eight wires                         */
#define LEVELS           5          /* stages that have to be cleared      */
#define SEQ_MAX          6          /* longest code: SEQ_BASE + LEVELS - 1 */
#define SEQ_BASE         2          /* length of the code of stage 1       */

#define STRIKES_MAX      3          /* wrong entries that blow the bomb    */
#define BOMB_TIME_MS     90000u     /* 90 s of countdown                   */
#define STRIKE_PENALTY   10000u     /* -10 s for a wrong wire              */
#define SCAN_COST        15000u     /* -15 s to see the code as text       */
#define REPLAY_COST      5000u      /* -5 s to see the code on the LEDs    */
#define CLEAR_BONUS      5000u      /* +5 s for a clean stage              */

#define LINE_MAX         24

/* Terminal speed.  115200 is the value of the laboratory guide; some
 * CH340/CH341 USB-serial adapters cannot work above 19200 baud, so the
 * default here is 9600.  Build with -DCMD_BAUD_CODE=B115200 for 115200. */
#ifndef CMD_BAUD_CODE
#define CMD_BAUD_CODE    B9600
#endif

/* The LEDs are wired active low: 0 lights the LED, 1 turns it off. */
#define LED_ON           0
#define LED_OFF          1

/* Game states */
#define ST_IDLE          0          /* waiting for ENTER / N               */
#define ST_PLAY          1          /* code shown, waiting for the player  */
#define ST_BUSY          2          /* an animation is running             */
#define ST_OVER          3          /* bomb exploded or defused            */

static const int led_pin[LED_COUNT] = { 9, 11, 15, 17, 20, 22, 6, 7 };

//-------------------------------------------------------------------------------------------------
// Input channels (UART3 and optionally UART5)
//-------------------------------------------------------------------------------------------------

struct channel
{
    void         *port;
    char          line[LINE_MAX];
    int           len;
    unsigned long mix;              /* keystroke entropy for the RNG       */
};

static struct channel ch[2];
static int            ch_cnt = 0;

//-------------------------------------------------------------------------------------------------
// Game state
//-------------------------------------------------------------------------------------------------

static int            state      = ST_IDLE;
static unsigned char  seq[SEQ_MAX];      /* the arming code of this stage   */
static int            seq_len    = 0;
static int            entered    = 0;    /* correct numbers entered so far  */
static int            stage      = 0;    /* 0..LEVELS-1                     */
static int            strikes    = 0;
static int            stage_error;       /* mistake made in this stage?     */
static unsigned int   deadline;          /* tick of the explosion           */
static unsigned int   shown_sec  = 0xFFFFFFFF;
static unsigned long  rnd_state  = 1;
static void          *play_port  = NULL;    /* port the game talks to  */

//-------------------------------------------------------------------------------------------------
// Small helpers: serial output
//-------------------------------------------------------------------------------------------------

static void out(void *port, const char *s)
{
    if (port != NULL)
        ls1x_uart_write(port, (void *)s, (int)strlen(s), 0);
}

/* print a decimal number without printf (keeps the code small and safe) */
static void out_num(void *port, unsigned int v)
{
    char buf[12];
    int  i = 0;

    if (v == 0)
    {
        out(port, "0");
        return;
    }
    while ((v > 0) && (i < (int)sizeof(buf) - 1))
    {
        buf[i++] = (char)('0' + (v % 10));
        v /= 10;
    }
    while (i > 0)
        ls1x_uart_write(port, &buf[--i], 1, 0);
}

/* the BSP ignores a second init(), the ioctl writes the divisor directly */
static void set_uart_speed(void *port, unsigned int baud_code)
{
    struct termios t;

    if (port == NULL)
        return;

    memset(&t, 0, sizeof(t));
    t.c_cflag = CS8 | baud_code;
    ls1x_uart_ioctl(port, IOCTL_NS16550_SET_MODE, &t);
}

/* throw away everything that is already waiting in the receive FIFO */
static void flush_port(void *port)
{
    char c;
    int  guard = 0;

    while ((ls1x_uart_read(port, &c, 1, 0) > 0) && (guard++ < 256))
        ;
}

//-------------------------------------------------------------------------------------------------
// Small helpers: LEDs
//-------------------------------------------------------------------------------------------------

static void led_set(int n, int value)       /* n = 1..8 */
{
    if ((n >= 1) && (n <= LED_COUNT))
        gpio_write(led_pin[n - 1], value);
}

static void led_all(int value)
{
    int i;

    for (i = 1; i <= LED_COUNT; i++)
        led_set(i, value);
}

static void led_pulse(int n)                /* short confirmation flash */
{
    led_set(n, LED_ON);
    delay_ms(90);
    led_set(n, LED_OFF);
}

static void led_strike_flash(void)          /* three red flashes, no "red" */
{
    int i;

    for (i = 0; i < 3; i++)
    {
        led_all(LED_ON);
        delay_ms(120);
        led_all(LED_OFF);
        delay_ms(120);
    }
}

static void led_win_pattern(void)           /* running light, three rounds */
{
    int round, n;

    for (round = 0; round < 3; round++)
    {
        for (n = 1; n <= LED_COUNT; n++)
        {
            led_all(LED_OFF);
            led_set(n, LED_ON);
            delay_ms(70);
        }
        for (n = LED_COUNT; n >= 1; n--)
        {
            led_all(LED_OFF);
            led_set(n, LED_ON);
            delay_ms(70);
        }
    }
    led_all(LED_OFF);
}

static void led_boom_pattern(void)
{
    led_all(LED_ON);
    delay_ms(2500);
    led_all(LED_OFF);
}

//-------------------------------------------------------------------------------------------------
// Pseudo random numbers (a linear congruential generator is enough here)
//-------------------------------------------------------------------------------------------------

static unsigned int rnd_next(void)
{
    rnd_state = rnd_state * 1103515245UL + 12345UL;
    return (unsigned int)((rnd_state >> 16) & 0x7FFFFFFFUL);
}

//-------------------------------------------------------------------------------------------------
// Time helpers
//-------------------------------------------------------------------------------------------------

static unsigned int time_left(unsigned int now)
{
    int left = (int)(deadline - now);

    return (left > 0) ? (unsigned int)left : 0u;
}

static unsigned int seconds_left(unsigned int now)
{
    return (time_left(now) + 999u) / 1000u;
}

//-------------------------------------------------------------------------------------------------
// Screens of the game
//-------------------------------------------------------------------------------------------------

static void banner(void *port)
{
    out(port, "\r\n");
    out(port, "==============================================\r\n");
    out(port, "   L O N G S O N  1 B   --   B O M B  D E F U S A L\r\n");
    out(port, "==============================================\r\n");
    out(port, "The bomb blinks its code on the 8 LEDs, one wire\r\n");
    out(port, "at a time.  Repeat the numbers in the same order\r\n");
    out(port, "and the next fuse is cut.  You have 90 seconds,\r\n");
    out(port, "three wrong wires and it goes off.\r\n");
}

static void help_screen(void *port)
{
    out(port, "\r\n--- commands -------------------------------\r\n");
    out(port, "  1..8   cut wire n (one number per line)\r\n");
    out(port, "  D      scan the code, costs 15 s\r\n");
    out(port, "  R      replay the code on the LEDs, costs 5 s\r\n");
    out(port, "  S      status\r\n");
    out(port, "  N      new game\r\n");
    out(port, "  H      this help\r\n");
    out(port, "--------------------------------------------\r\n");
}

/* one line that is rewritten in place once per second */
static void status_line(void *port, unsigned int now)
{
    out(port, "\r[");
    out_num(port, seconds_left(now));
    out(port, "s] stage ");
    out_num(port, (unsigned int)(stage + 1));
    out(port, "/");
    out_num(port, LEVELS);
    out(port, "  wire ");
    out_num(port, (unsigned int)(entered + 1));
    out(port, "/");
    out_num(port, (unsigned int)seq_len);
    out(port, "  strikes ");
    out_num(port, (unsigned int)strikes);
    out(port, "/");
    out_num(port, STRIKES_MAX);
    out(port, " > ");
}

static void prompt(void *port, unsigned int now)
{
    status_line(port, now);
    shown_sec = seconds_left(now);
}

//-------------------------------------------------------------------------------------------------
// Game logic
//-------------------------------------------------------------------------------------------------

/* blink the arming code of the current stage on the LEDs */
static void show_code(void *port)
{
    int i;

    state = ST_BUSY;
    out(port, "  the bomb is arming ...\r\n");
    delay_ms(500);

    for (i = 0; i < seq_len; i++)
    {
        led_set(seq[i], LED_ON);
        delay_ms(420);
        led_set(seq[i], LED_OFF);
        delay_ms(180);
    }

    led_all(LED_OFF);
    flush_port(port);                   /* drop keys pressed meanwhile */
}

static void stage_start(void *port, unsigned int now)
{
    int i;

    seq_len     = SEQ_BASE + stage;
    entered     = 0;
    stage_error = 0;

    for (i = 0; i < seq_len; i++)
        seq[i] = (unsigned char)(1 + (rnd_next() % LED_COUNT));

    out(port, "\r\n=== stage ");
    out_num(port, (unsigned int)(stage + 1));
    out(port, "/");
    out_num(port, LEVELS);
    out(port, ": the bomb will show ");
    out_num(port, (unsigned int)seq_len);
    out(port, " wires ===\r\n");

    show_code(port);
    state = ST_PLAY;
    prompt(port, now);
}

static void game_new(void *port, unsigned int now)
{
    int i;

    for (i = 0; i < ch_cnt; i++)
        ch[i].mix = ch[i].mix * 31u + now;      /* human timing = entropy */

    rnd_state = (ch[0].mix ^ (now * 2654435761UL)) | 1UL;

    stage       = 0;
    strikes     = 0;
    stage_error = 0;
    entered     = 0;
    deadline    = now + BOMB_TIME_MS;
    shown_sec   = 0xFFFFFFFF;
    state       = ST_IDLE;
    play_port   = port;

    led_all(LED_OFF);
    banner(port);
    out(port, "\r\nTime: 90 s, fuses: 5, strikes allowed: 3\r\n");
    stage_start(port, now);
}

static void game_over(void *port, const char *why)
{
    state = ST_OVER;

    out(port, "\r\n\r\n  ***  B O O M  ***  \r\n");
    out(port, "  the bomb exploded: ");
    out(port, why);
    out(port, "\r\n  fuses cut: ");
    out_num(port, (unsigned int)stage);
    out(port, "/");
    out_num(port, LEVELS);
    out(port, ", strikes: ");
    out_num(port, (unsigned int)strikes);
    out(port, "/");
    out_num(port, STRIKES_MAX);
    out(port, "\r\n");

    led_boom_pattern();

    out(port, "  type N (or press ENTER) for a new bomb\r\n> ");
    state = ST_IDLE;
}

static void game_win(void *port, unsigned int now)
{
    state = ST_OVER;

    out(port, "\r\n\r\n  ***  D E F U S E D  ***\r\n");
    out(port, "  all five fuses are cut.\r\n");
    out(port, "  time left: ");
    out_num(port, seconds_left(now));
    out(port, " s, strikes used: ");
    out_num(port, (unsigned int)strikes);
    out(port, "\r\n");

    led_win_pattern();

    out(port, "  type N (or press ENTER) for a new bomb\r\n> ");
    state = ST_IDLE;
}

static void strike(void *port, int wrong_wire, unsigned int now)
{
    strikes++;
    stage_error = 1;
    deadline   -= (deadline > STRIKE_PENALTY) ? STRIKE_PENALTY : deadline;

    out(port, "\r\n  WRONG wire ");
    out_num(port, (unsigned int)wrong_wire);
    out(port, " -- the code was ");
    {
        int i;
        for (i = 0; i < seq_len; i++)
        {
            out_num(port, (unsigned int)seq[i]);
            out(port, (i + 1 < seq_len) ? "-" : "");
        }
    }
    out(port, ", you were at position ");
    out_num(port, (unsigned int)(entered + 1));
    out(port, "\r\n  strike ");
    out_num(port, (unsigned int)strikes);
    out(port, "/");
    out_num(port, STRIKES_MAX);
    out(port, ", 10 seconds lost\r\n");

    led_strike_flash();

    if (strikes >= STRIKES_MAX)
    {
        game_over(port, "three wrong wires");
        return;
    }

    /* the same code is shown again, the player starts this stage over */
    entered = 0;
    show_code(port);
    state = ST_PLAY;
    prompt(port, now);
}

static void stage_cleared(void *port, unsigned int now)
{
    stage++;
    out(port, "\r\n  fuse ");
    out_num(port, (unsigned int)stage);
    out(port, " cut\r\n");

    if (stage >= LEVELS)
    {
        game_win(port, now);
        return;
    }

    if (!stage_error && (deadline > 0))
    {
        deadline += CLEAR_BONUS;
        out(port, "  clean stage: +5 seconds\r\n");
    }

    stage_start(port, now);
}

//-------------------------------------------------------------------------------------------------
// Command handling
//-------------------------------------------------------------------------------------------------

static void handle_line(struct channel *c, unsigned int now)
{
    char *s = c->line;

    if (s[0] == '\0')
    {
        if ((state == ST_IDLE) || (state == ST_OVER))
            game_new(c->port, now);
        else if (state == ST_PLAY)
            prompt(c->port, now);
        return;
    }

    /* a single digit 1..8 cuts a wire */
    if ((strlen(s) == 1) && (s[0] >= '1') && (s[0] <= '8'))
    {
        int wire = s[0] - '0';

        if (state != ST_PLAY)
        {
            out(c->port, "\r\n  the bomb is not armed, type N for a new one\r\n> ");
            return;
        }
        if (wire == seq[entered])
        {
            entered++;
            led_pulse(wire);
            if (entered >= seq_len)
                stage_cleared(c->port, now);
            else
            {
                out(c->port, "\r\n  ok, wire ");
                out_num(c->port, (unsigned int)wire);
                out(c->port, " correct\r\n");
                prompt(c->port, now);
            }
        }
        else
            strike(c->port, wire, now);
        return;
    }

    if ((strcmp(s, "H") == 0) || (strcmp(s, "HELP") == 0))
    {
        help_screen(c->port);
        out(c->port, "> ");
    }
    else if ((strcmp(s, "N") == 0) || (strcmp(s, "NEW") == 0))
        game_new(c->port, now);
    else if ((strcmp(s, "S") == 0) || (strcmp(s, "STATUS") == 0))
    {
        if (state == ST_PLAY)
            prompt(c->port, now);
        else
            out(c->port, "\r\n  no bomb is armed, type N\r\n> ");
    }
    else if ((strcmp(s, "D") == 0) || (strcmp(s, "SCAN") == 0))
    {
        int i;

        if (state != ST_PLAY)
        {
            out(c->port, "\r\n  nothing to scan, type N\r\n> ");
            return;
        }
        deadline -= (deadline > SCAN_COST) ? SCAN_COST : deadline;
        out(c->port, "\r\n  scan: the code is ");
        for (i = 0; i < seq_len; i++)
        {
            out_num(c->port, (unsigned int)seq[i]);
            out(c->port, (i + 1 < seq_len) ? "-" : "");
        }
        out(c->port, "   (15 seconds lost)\r\n");
        prompt(c->port, now);
    }
    else if ((strcmp(s, "R") == 0) || (strcmp(s, "REPLAY") == 0))
    {
        if (state != ST_PLAY)
        {
            out(c->port, "\r\n  nothing to replay, type N\r\n> ");
            return;
        }
        deadline -= (deadline > REPLAY_COST) ? REPLAY_COST : deadline;
        out(c->port, "\r\n  replay ...\r\n");
        show_code(c->port);
        state = ST_PLAY;
        prompt(c->port, now);
    }
    else
        out(c->port, "\r\n  unknown command, H for help\r\n> ");
}

/* upper case without pulling in ctype.h */
static char to_upper(char c)
{
    if ((c >= 'a') && (c <= 'z'))
        return (char)(c - 'a' + 'A');
    return c;
}

/* true while the player is in the middle of a line on any port */
static int any_typing(void)
{
    int i;

    for (i = 0; i < ch_cnt; i++)
        if (ch[i].len > 0)
            return 1;
    return 0;
}

/* read the ports, echo and collect complete lines */
static void poll_input(unsigned int now)
{
    int  i;
    char ch_in;

    for (i = 0; i < ch_cnt; i++)
    {
        struct channel *p = &ch[i];

        while (ls1x_uart_read(p->port, &ch_in, 1, 0) > 0)
        {
            if ((ch_in == '\r') || (ch_in == '\n'))
            {
                out(p->port, "\r\n");

                if (p->len > 0)
                    p->line[p->len] = '\0';
                else
                    p->line[0] = '\0';

                p->len        = 0;
                shown_sec     = 0xFFFFFFFF;
                handle_line(p, now);          /* may start / stop the game */
            }
            else if (p->len < LINE_MAX - 1)
            {
                p->line[p->len++] = to_upper(ch_in);
                ls1x_uart_write(p->port, &ch_in, 1, 0);   /* echo */
                p->mix = p->mix * 31u + (unsigned char)ch_in;
            }
            else
            {
                p->len = 0;
                out(p->port, "\r\n  line too long\r\n> ");
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------
// main
//-------------------------------------------------------------------------------------------------

int main(void)
{
    int i;

    ls1x_drv_init();                /* initialize the device drivers    */
    install_3th_libraries();        /* install the third party libraries */

    /* the eight LEDs are outputs, all of them off at power-on */
    for (i = 0; i < LED_COUNT; i++)
        gpio_enable(led_pin[i], DIR_OUT);
    led_all(LED_OFF);

    /* UART3 is the port asked for by the guide, UART5 is the console */
    ls1x_uart_init(devUART3, NULL);
    ls1x_uart_open(devUART3, NULL);
    set_uart_speed(devUART3, CMD_BAUD_CODE);

    ch[ch_cnt].port = devUART3;
    ch[ch_cnt].len  = 0;
    ch[ch_cnt].mix  = 12345UL;
    ch_cnt++;

#ifdef BSP_USE_UART5
    ch[ch_cnt].port = devUART5;
    ch[ch_cnt].len  = 0;
    ch[ch_cnt].mix  = 54321UL;
    set_uart_speed(devUART5, CMD_BAUD_CODE);
    ch_cnt++;
#endif

    for (i = 0; i < ch_cnt; i++)
    {
        banner(ch[i].port);
        help_screen(ch[i].port);
        out(ch[i].port, "\r\nType N (or just press ENTER) to arm the bomb\r\n> ");
    }

    for (;;)
    {
        unsigned int now = get_clock_ticks();

        poll_input(now);

        /* countdown: update the status line once per second */
        if ((state == ST_PLAY) && (play_port != NULL))
        {
            unsigned int sec = seconds_left(now);

            if (time_left(now) == 0)
                game_over(play_port, "the time ran out");
            else if ((sec != shown_sec) && !any_typing())
                prompt(play_port, now);
        }

        delay_ms(2);
    }

    /*
     * Never goto here!
     */
    return 0;
}

/*
 * @@ End
 */
