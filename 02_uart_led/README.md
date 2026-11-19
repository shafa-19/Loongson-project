# Section 6 --- Serial-port controlled running LED

## Hardware

| Signal | Pin | Direction | Note |
|---|---|---|---|
| LED 1..8 | GPIO 9, 11, 15, 17, 20, 22, 6, 7 | output | active low (0 = lit) |
| Console | UART3 | 115200 8N1 | USB-to-serial cable, 9-pin female on the board |

Wiring: board UART3 -> USB-to-serial -> PC, then open AiCube-ISP (or PuTTY) on the
virtual COM port (115200 8N1) and send the commands with a CR/LF terminator.

## Command set

| Command | Effect |
|---|---|
| `ON 3` | lights LED 3, all others off, stops the runner |
| `OFF` | every LED off |
| `SPEED 300` | running-LED period = 300 ms (100..1000 accepted) |
| `START` | starts the running LED |
| `STOP` | stops the running LED |

## What was fixed compared to the listing in the guide

1. `buffer_uart[total_read - 1] = '\0'` was replaced by
   `buffer_uart[total_read] = '\0'`: the old code overwrote the last real
   character instead of terminating after it (`ON 3` became `ON `).
2. CR **and** LF are both accepted as terminator, so the command never gets
   executed twice when the terminal sends `\r\n`.
3. An empty line no longer calls `parse_command()` with a stale buffer.
4. Every command answers with `OK:` / `ERR:` feedback, which makes the serial
   test in the report reproducible.
5. `led_select()` + the `led_pins[]` table replaced the 8-way switch and the
   eight hard-coded `gpio_write` calls.
6. A line longer than the buffer is rejected instead of silently corrupting the
   state.
