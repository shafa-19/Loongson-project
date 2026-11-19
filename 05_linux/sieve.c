/*=============================================================================
 * sieve.c --- system-level performance measurement on the board
 *
 * Counts the primes below N with a Sieve of Eratosthenes and prints the
 * elapsed CPU time, so the same source can be compared with -O0 and -O2
 * (see ../report chapter 6.3).  Under the MIPS32 build there is no hardware
 * FPU, so the soft-float library is used only for the final seconds value.
 *
 *   mipsel-linux-gcc -O2 -Wall sieve.c -o sieve_O2
 *   mipsel-linux-gcc -O0 -Wall sieve.c -o sieve_O0
 *   ./sieve_O2 2000000
 *===========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_N 10000000

static unsigned char composite[MAX_N + 1];

int main(int argc, char **argv)
{
    long n = 1000000;
    long i, j;
    long count = 0;
    clock_t t0, t1;
    double elapsed;

    if (argc > 1) {
        n = atol(argv[1]);
        if (n < 2 || n > MAX_N) {
            fprintf(stderr, "usage: %s [2..%d]\n", argv[0], MAX_N);
            return 1;
        }
    }

    memset(composite, 0, sizeof(composite));
    t0 = clock();

    for (i = 2; i * i <= n; i++)
        if (!composite[i])
            for (j = i * i; j <= n; j += i)
                composite[j] = 1;

    for (i = 2; i <= n; i++)
        if (!composite[i])
            count++;

    t1 = clock();
    elapsed = (double)(t1 - t0) / CLOCKS_PER_SEC;

    printf("n = %ld\n", n);
    printf("primes = %ld\n", count);
    printf("time = %.3f s\n", elapsed);
    return 0;
}
