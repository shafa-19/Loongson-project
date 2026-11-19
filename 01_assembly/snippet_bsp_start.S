/*======================================================================
 * Computer System Comprehensive Design --- Section 5
 * Overflow-safe summation of 1 + 3 + 5 + ... = n^2
 *
 * Paste the block below into the red-box area of bsp_start.S
 * (project example_7_2), exactly where the guide asks for it.
 *====================================================================*/

    .set    reorder
    li      t0, 0               /* t0 = running sum                        */
    li      t1, 1               /* t1 = current odd term (1, 3, 5, ...)    */
    li      t2, -1              /* t2 = 0xFFFFFFFF = max unsigned 32-bit   */
    li      t3, 0               /* t3 = last odd term that was accepted    */

cal_loop:
    subu    t4, t2, t0          /* t4 = free space = MAX - sum             */
    bltu    t4, t1, save_result /* if (space < term) -> stop, it would wrap*/
    addu    t0, t0, t1          /* sum += term                             */
    move    t3, t1              /* archive the last accepted term          */
    addu    t1, t1, 2           /* next odd term                           */
    b       cal_loop

save_result:
    lui     t5, 0x8003          /* t5 = 0x80030000 (KSEG0 data address)    */
    sw      t0, 0(t5)           /* mem[0x80030000] = sum                   */
    .set    noreorder
