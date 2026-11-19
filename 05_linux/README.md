# Section 9 --- Porting Linux and writing C programs

## Chain that has to be built (host = Ubuntu VM, target = Loongson 1B)

| Step | Tool / command | Output |
|---|---|---|
| 1 | install the cross compiler `gcc-4.3-ls232` | `mipsel-linux-gcc` |
| 2 | `make ARCH=mips CROSS_COMPILE=mipsel-linux- ls1b_defconfig` then `make uImage` | kernel image |
| 3 | `make` in the buildroot / rootfs tree | `rootfs` (cramfs / jffs2) |
| 4 | copy kernel + rootfs into the Tftpd32 root directory | files served over TFTP |
| 5 | on the board: `set serverip`, `tftp` download of kernel and rootfs | board boots Linux |
| 6 | `login: root` | shell prompt |

## Application development cycle

```sh
# inside the VM
export PATH=/opt/gcc-4.3-ls232/bin:$PATH
mipsel-linux-gcc -O2 -Wall hello.c -o hello
```

```sh
# on the board
ifconfig eth0 192.168.1.123            # same subnet as the PC
ping 192.168.1.111                     # PC must answer
tftp -g -r hello 192.168.1.111 ./      # fetch from Tftpd32
chmod 777 hello
./hello
```

## Programs in this directory

| File | Purpose |
|---|---|
| `hello.c` | sanity check: `uname()`, pid, argv --- proves the cross-compiled ELF runs under the ported kernel |
| `sieve.c` | Sieve of Eratosthenes, prints the elapsed CPU time so that `-O0` and `-O2` builds can be compared |
| `gpio_led.c` | optional: drives the eight Section-6 LEDs from user space by mapping the LS1B GPIO block through `/dev/mem` |
| `build.sh` | builds all of them with the LS1B toolchain |

## Troubleshooting (from 9.3 of the guide, extended)

| Symptom | Cause | Fix |
|---|---|---|
| `ping` to the PC fails | different subnets, firewall, or the PC has a second active adapter | `ifconfig eth0` check, `route -n`, disable the Windows firewall for the private network |
| `tftp: timeout` | Tftpd32 not running, or the file is not in its root directory | start Tftpd32 as administrator, set the *Current Directory* to the folder holding the binary |
| `-sh: ./hello: not found` although the file exists | wrong interpreter (`/lib/ld-linux.so` missing) | link statically (`-static`) for the first test |
| `Permission denied` | the TFTP transfer does not preserve the executable bit | `chmod 777 hello` |
| board boots but the rootfs is read-only | cramfs is a read-only file system by design | mount a tmpfs over the directory you write to, or use jffs2 |
| `/dev/mem: Permission denied` in `gpio_led` | `CONFIG_DEVMEM` off or not root | run as `root`, or use the sysfs LED class |
