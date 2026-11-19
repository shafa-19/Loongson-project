# Section 7 --- RT-Thread: the sleeping-barber problem

Project settings in LoongIDE: *C Executable*, Mcu `LS1B200 (LS232)`,
Toolchain `SDE Lite 4.9.2 for MIPS`, **Using RTOS = RT-Thread**.

## Synchronisation design

| Object | Type | Initial value | Meaning |
|---|---|---|---|
| `customers` | counting semaphore | 0 | one token per waiting customer; the barber blocks on it |
| `barber` | counting semaphore | 0 | "the barber is ready, come to the chair" |
| `mutex` | mutex | 1 | protects `freeseats` (the waiting-room chairs) |
| `freeseats` | shared `volatile int` | 3 | chairs still free |

## Threads and priorities

| Thread | Priority | Stack | Role |
|---|---|---|---|
| barber | 10 | 20 KB | serves one customer per iteration, else sleeps |
| customer 1 | 5 | 20 KB | highest customer priority, arrives first |
| customer 2 | 6 | 20 KB | |
| customer 3 | 7 | 20 KB | |
| customer 4 | 8 | 20 KB | |
| customer 5 | 9 | 20 KB | |

In RT-Thread a *smaller* number means a *higher* priority; the barber is the
lowest-priority thread, which is why it only runs when all customers are
blocked and why the five arrivals happen strictly in id order.

## Expected console output

```
[init] barber thread started (priority 10).
[init] customer 1 started (priority 5).
...
[customer 1] sits down, freeseats=2
[customer 2] sits down, freeseats=1
[customer 3] sits down, freeseats=0
[customer 4] no free seat, leaves.
[customer 5] no free seat, leaves.
[barber] find customer 1, begin to cut hair.
[barber] finish cut hair.
... (3 haircuts in total)
```

3 of the 5 customers are served, 2 leave --- exactly what `FREESEATS_INIT = 3`
predicts.  Changing `FREESEATS_INIT` to 5 makes all five wait and be served.

## Bugs of the listing in the guide that were fixed here

| # | Problem in the guide listing | Consequence | Fix |
|---|---|---|---|
| 1 | `static void customer_thread(int num)` | RT-Thread calls the entry with `void *`; the id was read from a pointer, giving a wrong customer number on a 32-bit build | entry is `void customer_thread(void *)`, id unpacked with `(int)(rt_ubase_t)parameter` |
| 2 | `static rt_err_t res;` **inside** the thread functions | one `static` variable shared by all six threads | local (automatic) variables |
| 3 | `static rt_uint8_t number;` in the barber | the counter is shared with every caller of the function | automatic variable |
| 4 | `rt_sem_delete(customers)` when a take fails | deletes a semaphore that other threads still wait on | the barber just retries |
| 5 | `freeseats` declared without `volatile` | `-O2` may keep it in a register | `volatile int` |
| 6 | no return-value check of `rt_sem_create` / `rt_thread_create` | a failure silently produces a deadlock that looks like a design error | checked, `-1` returned with a message |
| 7 | priority computed as `THREAD_PRIORITY - 5 + i` | hard-coded 5 | expressed with `CUSTOMER_NUM` |
