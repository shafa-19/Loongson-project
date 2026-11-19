/*
 * harness.c -- host-side test of the bomb defusal game
 *
 * The real main.c is copied here as main_under_test.c and compiled with
 * -Dmain=game_main, so the whole game code runs unchanged on the PC.
 * Everything the BSP would provide is stubbed below:
 *
 *   - a virtual 1 ms clock that advances by 2 ms per main-loop iteration
 *   - a UART FIFO in front of devUART5 (the console) and an output capture
 *   - the 8 LED pins as an array
 *
 * A small "bot" plays the game through the same interface a human would use
 * (it only types characters) and checks the transcript afterwards.
 *
 * usage: harness win | strike | timeout | commands
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*---------------------------------------------------------------------------*/
/* stubs                                                                     */
/*---------------------------------------------------------------------------*/

static unsigned int vclock = 0;
static int          led_pin_state[64];

void gpio_enable(int ioNum, int dir)   { (void)dir; (void)ioNum; }
void gpio_write(int ioNum, int val)    { if ((ioNum >= 0) && (ioNum < 64)) led_pin_state[ioNum] = val; }
int  gpio_read(int ioNum)              { return ((ioNum >= 0) && (ioNum < 64)) ? led_pin_state[ioNum] : 1; }

void *devUART3 = (void *)1;
void *devUART5 = (void *)2;

#define FIFO_SZ 64
static char fifo[FIFO_SZ];
static int  fh = 0, ft = 0;

static char outbuf[200000];
static int  outlen = 0;

int ls1x_uart_init(void *p, void *a) { (void)p; (void)a; return 0; }
int ls1x_uart_open(void *p, void *a) { (void)p; (void)a; return 0; }

int ls1x_uart_write(void *p, void *b, int n, int t)
{
    (void)p; (void)t;
    if (outlen + n < (int)sizeof(outbuf) - 1)
    {
        memcpy(outbuf + outlen, b, (size_t)n);
        outlen += n;
        outbuf[outlen] = '\0';
    }
    return n;
}

int ls1x_uart_read(void *p, void *b, int n, int t)
{
    (void)t; (void)n;
    if (p != devUART5)                  /* the bot types on the console only */
        return 0;
    if (fh == ft)
        return 0;
    *(char *)b = fifo[fh % FIFO_SZ];
    fh++;
    return 1;
}

int ls1x_uart_ioctl(void *p, int c, void *a) { (void)p; (void)c; (void)a; return 0; }

void ls1x_drv_init(void)          {}
void install_3th_libraries(void)  {}
void delay_ms(unsigned int ms)    { vclock += ms; }

static void q(const char *s)
{
    while (*s)
    {
        fifo[ft % FIFO_SZ] = *s++;
        ft++;
    }
}

/*---------------------------------------------------------------------------*/
/* the game under test                                                       */
/*---------------------------------------------------------------------------*/

static void bot(void);
static void finish(void);

#define main game_main
#include "main_under_test.c"
#undef main

/* the clock is defined after the include so that it can see the game state */
unsigned int get_clock_ticks(void)
{
    vclock += 2;                        /* one iteration of the main loop */

    if (vclock > 260000u)               /* watchdog: 4 minutes of game time */
    {
        printf("\n*** watchdog: still running, state=%d stage=%d strikes=%d\n",
               state, stage, strikes);
        printf("%s\n", outbuf);
        exit(2);
    }

    bot();
    return vclock;
}

/*---------------------------------------------------------------------------*/
/* the bot and the checks                                                    */
/*---------------------------------------------------------------------------*/

static const char *scenario = "win";
static int         started   = 0;
static int         hold      = 0;
static int         n_wrong   = 0;
static int         cmd_step  = 0;
static int         done      = 0;

static int has(const char *s) { return strstr(outbuf, s) != NULL; }

static void bot(void)
{
    if (done)
        return;
    if (hold-- > 0)
        return;
    hold = 30;                          /* ~60 ms of virtual time between keys */

    if (!strcmp(scenario, "win"))
    {
        if (!started)                   { q("\r"); started = 1; return; }
        if (state == ST_PLAY)           { char b[3] = {0}; b[0] = (char)('0' + seq[entered]); b[1] = '\n'; q(b); return; }
        if (has("D E F U S E D"))       { done = 1; finish(); }
    }
    else if (!strcmp(scenario, "strike"))
    {
        if (!started)                   { q("\r"); started = 1; return; }
        if (state == ST_PLAY)
        {
            char wrong[3] = {0};
            int  w = (seq[entered] % 8) + 1;        /* always a different wire */
            wrong[0] = (char)('0' + w);
            wrong[1] = '\n';
            q(wrong);
            n_wrong++;
            return;
        }
        if (has("three wrong wires"))   { done = 1; finish(); }
    }
    else if (!strcmp(scenario, "timeout"))
    {
        if (!started)                   { q("\r"); started = 1; return; }
        if (has("the time ran out"))    { done = 1; finish(); }
    }
    else if (!strcmp(scenario, "commands"))
    {
        if (!started)                   { q("\r"); started = 1; return; }

        if ((cmd_step >= 5) && has("fuse 1 cut")) { done = 1; finish(); }

        if (state == ST_PLAY)
        {
            switch (cmd_step)
            {
            case 0: q("H\n");  cmd_step++; return;      /* help                */
            case 1: q("S\n");  cmd_step++; return;      /* status              */
            case 2: q("D\n");  cmd_step++; return;      /* scan, costs 15 s    */
            case 3: q("R\n");  cmd_step++; return;      /* replay, costs 5 s   */
            case 4: q("Z\n");  cmd_step++; return;      /* unknown command     */
            case 5:                                     /* finish this stage   */
                {
                    char b[3] = {0};
                    b[0] = (char)('0' + seq[entered]);
                    b[1] = '\n';
                    q(b);
                    return;
                }
            default:
                return;
            }
        }
    }
}

static void finish(void)
{
    int fails = 0;

    printf("\n=========== scenario: %s ===========\n", scenario);
    printf("%s\n", outbuf);
    printf("====================================\n");

#define CHECK(cond, name) do { \
        if (cond) printf("  PASS  %s\n", name); \
        else    { printf("  FAIL  %s\n", name); fails++; } } while (0)

    CHECK(has("B O M B  D E F U S A L"), "banner printed");
    CHECK(has("stage 1/5"),              "stage 1 announced");
    CHECK(has("the bomb is arming"),      "arming animation ran");
    CHECK(vclock < 200000u,               "virtual clock sane");

    if (!strcmp(scenario, "win"))
    {
        CHECK(has("fuse 1 cut") && has("fuse 5 cut"), "all five fuses cut");
        CHECK(has("D E F U S E D"),                   "victory screen");
        CHECK(has("strikes used: 0"),                 "no strike used");
    }
    else if (!strcmp(scenario, "strike"))
    {
        CHECK(has("WRONG wire"),                      "wrong wire reported");
        CHECK(has("strike 3/3"),                      "third strike counted");
        CHECK(has("B O O M"),                         "explosion screen");
        CHECK(has("three wrong wires"),               "correct reason");
    }
    else if (!strcmp(scenario, "timeout"))
    {
        CHECK(has("B O O M"),                         "explosion screen");
        CHECK(has("the time ran out"),                "time-out reason");
        CHECK(vclock >= 90000u,                       "countdown reached zero");
    }
    else if (!strcmp(scenario, "commands"))
    {
        CHECK(has("commands ---"),                    "help screen (H)");
        CHECK(has("scan: the code is"),               "scan works (D)");
        CHECK(has("replay ..."),                      "replay works (R)");
        CHECK(has("unknown command"),                 "bad command rejected (Z)");
        CHECK(has("fuse 1 cut"),                      "stage cleared after the commands");
    }

    printf("\n%s: %d failure(s)\n", fails ? "RESULT FAIL" : "RESULT OK", fails);
    exit(fails ? 1 : 0);
}

/*---------------------------------------------------------------------------*/
/* own main: the bot drives the game itself                                  */
/*---------------------------------------------------------------------------*/

int main(int argc, char **argv)
{
    if (argc > 1)
        scenario = argv[1];

    game_main();                        /* never returns; finish() exits    */
    return 0;
}
