/*=============================================================================
 * Computer System Comprehensive Design --- Section 7
 * RT-Thread application: the sleeping-barber problem
 *
 *   waiting room : FREESEATS chairs (default 3)
 *   customers    : 5 threads, priorities 5..9 (lower value = higher priority)
 *   barber       : 1 thread, priority 10
 *
 * Synchronisation objects
 *   customers (counting semaphore, init 0)  - number of waiting customers
 *   barber    (counting semaphore, init 0)  - "the barber is ready for you"
 *   mutex     (mutex,             init 1)   - protects freeseats / the chairs
 *
 * The five customer threads use different priorities, so the order in which
 * they enter the shop is deterministic and reproducible on the board.
 *
 * Two optional demo helpers (can be disabled with the defines below):
 *   DEMO_LEDS  1 - the 8-LED module shows the scenario:
 *                  LED 1..5 = customer 1..5 is waiting
 *                  LED 8    = the barber is cutting hair
 *   DEMO_SLOW  1 - generous delays so that the run is easy to watch
 *                  (the guide values finish the whole run in ~0.2 s)
 *===========================================================================*/

#include <rtthread.h>
#include "ls1b.h"
#include "mips.h"
#include "bsp.h"

/*---------------------------------------------------------------------------
 * Configuration
 *-------------------------------------------------------------------------*/
#define THREAD_PRIORITY     10
#define THREAD_STACK_SIZE   (1024 * 20)
#define THREAD_TIMESLICE    5
#define CUSTOMER_NUM        5
#define FREESEATS_INIT      3          /* chairs in the waiting room */

#ifndef DEMO_LEDS
#define DEMO_LEDS           1
#endif

#ifndef DEMO_SLOW
#define DEMO_SLOW           1
#endif

#if DEMO_SLOW
#define T_ARRIVE()          rt_thread_mdelay(2000)   /* all at the same time */
#define T_HAIRCUT()         rt_thread_mdelay(4000)
#else
#define T_ARRIVE()          rt_thread_mdelay(50)
#define T_HAIRCUT()         rt_thread_mdelay(40)
#endif

/*---------------------------------------------------------------------------
 * Optional LED visualisation (8-LED module on the GPIO header)
 *-------------------------------------------------------------------------*/
#if DEMO_LEDS
static const int led_pin[8] = { 9, 11, 15, 17, 20, 22, 6, 7 };

static void leds_init(void)
{
    int i;

    for (i = 0; i < 8; i++)
    {
        LS1B_GPIO_CFG(led_pin[i] / 32) |=  (1u << (led_pin[i] % 32));   /* GPIO mode  */
        LS1B_GPIO_EN(led_pin[i] / 32)  &= ~(1u << (led_pin[i] % 32));   /* output     */
        LS1B_GPIO_OUT(led_pin[i] / 32) |=  (1u << (led_pin[i] % 32));   /* off (high) */
    }
}

static void led_set(int idx, int on)
{
    if ((idx < 0) || (idx > 7))
        return;

    if (on)
        LS1B_GPIO_OUT(led_pin[idx] / 32) &= ~(1u << (led_pin[idx] % 32));
    else
        LS1B_GPIO_OUT(led_pin[idx] / 32) |=  (1u << (led_pin[idx] % 32));
}
#else
static void leds_init(void) { }
static void led_set(int idx, int on) { (void)idx; (void)on; }
#endif

/*---------------------------------------------------------------------------
 * State
 *-------------------------------------------------------------------------*/
static rt_sem_t   customers = RT_NULL;
static rt_sem_t   barber    = RT_NULL;
static rt_mutex_t mutex     = RT_NULL;

static rt_thread_t barber_tid = RT_NULL;
static rt_thread_t customer_tid[CUSTOMER_NUM];

static const int customer_id[CUSTOMER_NUM] = { 0, 1, 2, 3, 4 };
static volatile int freeseats = FREESEATS_INIT;

/*---------------------------------------------------------------------------
 * Barber thread
 *-------------------------------------------------------------------------*/
static void barber_thread(void *parameter)
{
    int served = 0;

    (void)parameter;

    for (;;)
    {
        /* 1. sleep until a customer appears */
        if (rt_sem_take(customers, RT_WAITING_FOREVER) != RT_EOK)
            continue;

        served++;
        rt_kprintf("[barber] find customer %d, begin to cut hair.\n", served);

        /* 2. one waiting-room chair became free, tell the customer to come */
        rt_mutex_take(mutex, RT_WAITING_FOREVER);
        freeseats++;
        rt_sem_release(barber);
        rt_mutex_release(mutex);

        /* 3. the haircut itself */
        led_set(7, 1);
        T_HAIRCUT();
        led_set(7, 0);

        rt_kprintf("[barber] finish cut hair.\n");

        /* 4. nobody waiting -> the next rt_sem_take() blocks (he sleeps) */
        if (freeseats >= FREESEATS_INIT)
            rt_kprintf("[barber] no customer, go to sleep.\n");
    }
}

/*---------------------------------------------------------------------------
 * Customer thread
 *-------------------------------------------------------------------------*/
static void customer_thread(void *parameter)
{
    int id = (int)(rt_ubase_t)parameter;

    T_ARRIVE();                                   /* walking to the shop     */

    rt_mutex_take(mutex, RT_WAITING_FOREVER);     /* enter the critical part */

    if (freeseats > 0)
    {
        freeseats--;
        led_set(id, 1);                           /* LED 1..5: he is waiting */
        rt_kprintf("[customer %d] sits down, freeseats=%d\n", id + 1, freeseats);

        rt_sem_release(customers);                /* wake the barber         */
        rt_mutex_release(mutex);

        rt_kprintf("[customer %d] is waiting for a haircut.\n", id + 1);

        rt_sem_take(barber, RT_WAITING_FOREVER);  /* wait for my turn        */
        led_set(id, 0);
        rt_kprintf("[customer %d] haircut done, leaves.\n", id + 1);
    }
    else
    {
        rt_mutex_release(mutex);
        rt_kprintf("[customer %d] no free seat, leaves.\n", id + 1);
    }
}

/*---------------------------------------------------------------------------
 * Entry point
 *-------------------------------------------------------------------------*/
int main(int argc, char **argv)
{
    int i;
    int priority;

    (void)argc;
    (void)argv;

    rt_kprintf("\r\nWelcome to RT-Thread.\r\n\r\n");

    ls1x_drv_init();
    rt_ls1x_drv_init();
    install_3th_libraries();

    leds_init();

    customers = rt_sem_create("cust_sem", 0, RT_IPC_FLAG_PRIO);
    if (customers == RT_NULL)
    {
        rt_kprintf("create semaphore customers failed.\n");
        return -1;
    }

    barber = rt_sem_create("barber_sem", 0, RT_IPC_FLAG_PRIO);
    if (barber == RT_NULL)
    {
        rt_kprintf("create semaphore barber failed.\n");
        return -1;
    }

    mutex = rt_mutex_create("seatmutex", RT_IPC_FLAG_PRIO);
    if (mutex == RT_NULL)
    {
        rt_kprintf("create mutex failed.\n");
        return -1;
    }

    barber_tid = rt_thread_create("barber", barber_thread, RT_NULL,
                                  THREAD_STACK_SIZE, THREAD_PRIORITY,
                                  THREAD_TIMESLICE);
    if (barber_tid == RT_NULL)
    {
        rt_kprintf("create barber thread failed.\n");
        return -1;
    }
    rt_thread_startup(barber_tid);
    rt_kprintf("[init] barber thread started (priority %d).\n", THREAD_PRIORITY);

    for (i = 0; i < CUSTOMER_NUM; i++)
    {
        priority = THREAD_PRIORITY - CUSTOMER_NUM + i;    /* 5, 6, 7, 8, 9 */

        customer_tid[i] = rt_thread_create("customer", customer_thread,
                                           (void *)(rt_ubase_t)customer_id[i],
                                           THREAD_STACK_SIZE, priority,
                                           THREAD_TIMESLICE);
        if (customer_tid[i] == RT_NULL)
        {
            rt_kprintf("create customer %d failed.\n", i + 1);
            continue;
        }

        rt_thread_startup(customer_tid[i]);
        rt_kprintf("[init] customer %d started (priority %d).\n", i + 1, priority);
    }

    return 0;
}

/*
 * @@ End
 */
