/*=============================================================================
 * hello.c --- first user-space program on the ported Linux
 *
 * Build in the Ubuntu virtual machine:
 *   export PATH=/opt/gcc-4.3-ls232/bin:$PATH
 *   mipsel-linux-gcc -O2 -Wall hello.c -o hello
 *===========================================================================*/

#include <stdio.h>
#include <unistd.h>
#include <sys/utsname.h>

int main(int argc, char **argv)
{
    struct utsname u;
    int i;

    printf("Hello from the Loongson 1B running Linux!\n");
    printf("pid = %d\n", (int)getpid());

    if (uname(&u) == 0)
        printf("kernel: %s %s %s\n", u.sysname, u.release, u.machine);

    printf("arguments (%d):", argc);
    for (i = 0; i < argc; i++)
        printf(" %s", argv[i]);
    printf("\n");

    return 0;
}
