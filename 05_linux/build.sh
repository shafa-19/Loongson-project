#!/bin/sh
#==============================================================================
# build.sh -- cross-compile the Linux examples inside the Ubuntu VM
#
#   export PATH=/opt/gcc-4.3-ls232/bin:$PATH     (as required by the guide)
#   ./build.sh
#
# The resulting ELF binaries are mipsel, little endian, soft-float, which is
# what the LS1B kernel expects.
#==============================================================================
set -e

CC=${CC:-mipsel-linux-gcc}
CFLAGS="-mips32 -EL -msoft-float -Wall"

echo "compiler : $($CC --version | head -1)"

for src in hello sieve gpio_led; do
    if [ -f "$src.c" ]; then
        echo "  CC  $src.c -> $src"
        $CC $CFLAGS -O2 "$src.c" -o "$src"
    fi
done

# an -O0 build of the benchmark, to compare in the report
if [ -f sieve.c ]; then
    echo "  CC  sieve.c -> sieve_O0 (-O0)"
    $CC $CFLAGS -O0 sieve.c -o sieve_O0
fi

ls -l hello sieve sieve_O0 gpio_led 2>/dev/null || true
echo "done - copy the binaries to the Tftpd32 root directory on the PC"
