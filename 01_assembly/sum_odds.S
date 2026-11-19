/*======================================================================
 * sum_odds.S --- standalone MIPS32 (little-endian) reference program
 *
 * Same algorithm as the fragment that is pasted into bsp_start.S, but
 * written as a complete, self-contained module so that it can be
 * assembled / linked / executed outside LoongIDE (SDE toolchain or any
 * MIPS32 simulator) for verification purposes.
 *
 * Build (SDE Lite 4.9.2 for MIPS):
 *   mips-sde-elf-gcc -mips32 -EL -msoft-float -nostdlib -nostartfiles \
 *                    -Ttext=0x80001000 sum_odds.S -o sum_odds.elf
 *====================================================================*/

    .text
    .globl  sum_odds
    .ent    sum_odds

/*----------------------------------------------------------------------
 * unsigned int sum_odds(void)
 *   returns the sum in v0 and also stores it to 0x80030000
 *--------------------------------------------------------------------*/
sum_odds:
    .set    reorder

    li      t0, 0               /* sum                                   */
    li      t1, 1               /* current odd term                      */
    li      t2, -1              /* 0xFFFFFFFF, max unsigned value        */
    li      t3, 0               /* last accepted term                    */

1:  subu    t4, t2, t0          /* remaining space in the register       */
    bltu    t4, t1, 2f          /* stop before an unsigned wrap          */
    addu    t0, t0, t1          /* sum += term                           */
    move    t3, t1              /* remember the last accepted term       */
    addu    t1, t1, 2           /* 1, 3, 5, 7, ...                       */
    b       1b

2:  lui     t5, 0x8003          /* 0x80030000                            */
    sw      t0, 0(t5)
    move    v0, t0              /* return value                          */
    jr      ra
    nop

    .set    noreorder
    .end    sum_odds
