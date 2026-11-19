# Loongson 1B (MIPS32) — Computer System Comprehensive Design

`https://github.com/shafa-19/Loongson-project`

Course project for **Computer System Comprehensive Design** (Beijing University of Chemical
Technology). Everything in this repository was written for, and verified on, a real
**Loongson 1B / LS1B200 (LS232)** development board: MIPS32 Release 2, bare metal and
RT-Thread, with the LoongIDE 1.1 tool chain (SDE Lite 4.9.2 for MIPS).

The repository contains the four hardware tasks of the course, the Linux porting study,
and one self-designed mini project — a **Bomb Defusal Game** — together with the two
reports and the scripts used to build, load and test the code on the board.

---

## Contents

| Directory | What it is | Status |
|---|---|---|
| `01_assembly/` | Overflow-safe summation of odd numbers in MIPS32 assembly (`sltu`-style overflow guard, sum stored at `0x80030000`) | verified in the debugger on the board |
| `02_uart_led/` | Serial-port controlled running LED: 8 LEDs on GPIO, command interpreter (`ON n`, `OFF`, `SPEED ms`, `START`, `STOP`) | verified on the board |
| `03_rtthread_barber/` | Sleeping-barber problem on RT-Thread (2 counting semaphores + 1 mutex, 6 threads) | verified on the board |
| `04_lwip_network/` | TCP server, TCP client, UDP server, UDP client over lwIP 1.4.1 and GMAC0 | all four tests passed on the board |
| `05_linux/` | Porting Linux to the board: kernel and root file system images, user programs | documented (see note below) |
| `06_bomb_game/` | **Self-designed mini project**: interactive bomb defusal game (LEDs + serial console + tick) | verified on the board |
| `docs/` | The final report, the mini-project report, hardware result logs, demo instructions | — |
| `tools/` | Build, deploy and test scripts used during the project | — |

---

## The four hardware tasks

| Task | Key idea | Result |
|---|---|---|
| Assembly | add the odd numbers 1+3+5+… while the 32-bit sum cannot overflow; the last admissible term is 0x0001FFFD, the result 0xFFFE0001 is stored at 0x80030000 | `t0=0xFFFE0001`, `t1=0x0001FFFF` refused, memory verified byte by byte |
| UART + LED | 8 LEDs on GPIO 9, 11, 15, 17, 20, 22, 6, 7 (active low), driven from a line-based command interpreter on UART3/UART5 | every command tested, including the rejection of out-of-range arguments |
| RT-Thread | barber thread (priority 10) and five customers (5…9); `customers`/`barber` semaphores, `mutex` around `freeseats` | 3 customers sit (`freeseats` 2→1→0), 2 leave, 3 haircuts, barber sleeps |
| lwIP | socket API on top of lwIP 1.4.1; ports 9060–9063 (TCP server/client, UDP server/client) | 3 TCP echoes, 5 client messages, 3 UDP datagrams, ping 6/6 |

`05_linux/` is a study: the host environment, the image sizes produced
(`vmlinux` 12 723 580 B, `rootfs.ext2` 87 410 688 B), the PMON flash procedure, and the
reason the root file system does not fit into the flash partition together with the two
possible solutions. It is documented rather than executed, and the report says so
explicitly.

---

## Self-designed project: Bomb Defusal Game

`06_bomb_game/` — an interactive game that turns the eight LEDs into the eight wires of a
bomb. In every stage the bomb blinks its arming code on the LEDs; the player repeats the
wire numbers on the serial console before the 90-second countdown reaches zero.

* 5 stages (2 → 6 wires), 3 strikes, `SCAN` costs 15 s, `REPLAY` 5 s, a clean stage gives 5 s back
* no dynamic memory, no `printf`, no floating point: a bare-metal state machine
* both serial ports are polled, so a terminal on UART5 and the guide's UART3 work at the same time
* tested twice: a **host simulation** that plays the real `main.c` against stubbed hardware
  (4 scenarios, 30 checks) and a scripted run on the **real board** (`DEFUSED`, 13 s left)

Full description: [`06_bomb_game/README.md`](06_bomb_game/README.md).
Report: [`docs/Bomb_Defusal_Game_Report.pdf`](docs/Bomb_Defusal_Game_Report.pdf).

---

## Building

The projects are LoongIDE projects. The IDE makefiles are unusable on a modern Windows
installation (the bundled MSYS runtime collides with the system one), so the sources are
compiled directly with the tool chain that ships with the IDE:

```bat
python tools\build_proj.py D:\loongson1B_example\example_10_bomb example_10_bomb
```

```
compiled: 84 / 84
   text    data     bss     dec     hex  filename
  59504    3608   17968   81080   13cb8  Build\example_10_bomb.exe
```

Two details matter for a **bare-metal** project and are handled by the script:

* the BSP selects its OS branch with `BSP_USE_OS = (!defined(OS_NONE))`, so a bare-metal
  build needs `-DOS_NONE` (the IDE stores exactly that in `DefinedSymbols`);
* the start-up code needs `core/include` and `core/mips` on the include path.

RT-Thread projects (for example `03_rtthread_barber`) are built the same way; the script
then defines `OS_RTTHREAD` instead, and the network task additionally needs
`-DUSE_LWIP -DBSP_USE_GMAC0`.

## Running on the board

1. Open the `.lxp` file in LoongIDE and press **F9** (Debug → Run).
   If the project was built with `tools\build_proj.py`, keep `AutoRebuild=0` in the
   project file so that the IDE downloads the existing binary instead of rebuilding it.
2. Press **Debug → Continue** so the program starts (F9 alone only loads it and stops
   on the first line of `main`).
3. Connect a terminal to the console port: **9600 8N1**, no flow control.
   `tools\serial_terminal.ps1 -Port COM5 -Baud 9600` is a minimal terminal that works
   where PuTTY cannot configure the adapter.

`tools\run_on_board.ps1 -Project <name> -Dir <dir>` automates steps 1–2 through the
EJTAG proxy of the IDE (`127.0.0.2:50000`).

## Testing

`tools\board_game_test.ps1` plays the game on the board over the serial port (it reads
the console, asks for the code with `SCAN`, answers it and records the transcript), and
`06_bomb_game/host_test/` contains the host harness that runs the real `main.c` against
stubbed BSP functions.

---

## Platform notes (learned the hard way)

| Symptom | Cause | Fix |
|---|---|---|
| `wait for pracc of LS232(1B/1C) fail`, F9 hangs | the IDE tries to download over the parallel-port path | set `DownloadByNet=0`; if it still appears, power-cycle the board for 10 s |
| `rtconfig.h not found` in a bare-metal build | `OS_RTTHREAD` was defined | compile with `-DOS_NONE` |
| the board is silent on the network | lwIP and GMAC code compiled out | build with `-DUSE_LWIP -DBSP_USE_GMAC0` |
| `SetCommState fails`, no serial data | CH340 adapter of the test PC | re-plug the adapter; 9600 baud is reliable, 115200 is not |
| `console_init(9600)` inside `main()` crashes RT-Thread | `rt_kprintf` is called with interrupts disabled at boot | keep 115200 at boot and change the speed after `Clock_initialize()` |

---

## Reports

* `docs/Computer_System_Comprehensive_Design_Report.pdf` — the final report
  (Preface, 7 chapters, appendix; the real hardware results are reproduced as figures).
* `docs/Bomb_Defusal_Game_Report.pdf` — the report of the self-designed project.
* `docs/HARDWARE_RESULTS.md` — the raw results collected on the board.
* `docs/DEMO_ON_BOARD.md` — step-by-step demo instructions.

## License

The code written for this project (`01_…`–`06_…`, `tools/`) is released under the MIT
license, see `LICENSE`. The board support package inside `06_bomb_game/`
(`core/`, `ls1x-drv/`, `libc/`, `include/`, `ld.script`) is the vendor BSP distributed
with the course platform (© Suzhou Tiancheng Software Ltd., Apache-2.0 headers) and is
included only so that the project builds out of the box.

## Author

**EYVAZLI SHAFA** — GitHub [@shafa-19](https://github.com/shafa-19) — eivazoffshafa@gmail.com

Course: Computer System Comprehensive Design, Beijing University of Chemical Technology.
