# Section 8 --- Network transmission with lwIP

## Project creation (LoongIDE)

*File -> New -> New Project Wizard*, then

| Field | Value |
|---|---|
| Project type | C Executable |
| Project name | `example_9_1` |
| Mcu Model | `LS1B200 (LS232)` |
| Tool Chain | SDE Lite 4.9.2 for MIPS |
| Using RTOS | RT-Thread |
| RT-Thread components | **lwip 1.4.1 - light weight TCP/IP stack** |

Add the six files of this directory to the project (right-click the project ->
*New Source File* / *New Header File*, or *Import*).

## Address plan

| Node | IP | Mask |
|---|---|---|
| Loongson 1B (GMAC0) | 192.168.1.123 | 255.255.255.0 |
| PC | 192.168.1.111 | 255.255.255.0 |

| Test | Board role | Ports |
|---|---|---|
| TCP server | listens | local 9060 |
| TCP client | connects to PC:9061 | remote 9061 |
| UDP server | listens | local 9062 |
| UDP client | sends to PC:9063 | remote 9063 |

## How to run one test

1. In `lwip_test.h` set exactly **one** of `TEST_TCP_SERVER`,
   `TEST_TCP_CLIENT`, `TEST_UDP_SERVER`, `TEST_UDP_CLIENT` to `1`
   (the file as shipped has `TEST_TCP_SERVER = 1`).
2. *Project -> Compile & Build*, then *Debug -> Run*.
3. Watch the messages in PuTTY on the board's UART5 console (115200 8N1).
4. On the PC use the matching helper:
   * TCP server test  -> `python tcp_client.py` (or netcat: `nc 192.168.1.123 9060`)
   * TCP client test  -> `python tcp_server.py`
   * UDP tests        -> `python udp_echo.py` / `python udp_peer.py`

The small Python peers in this directory let you reproduce the test without
installing a third-party terminal program; `nc` works as well.

## Corrections against the listing in the guide

| # | Guide | Problem | Here |
|---|---|---|---|
| 1 | `accept(sock_fd, ..., (socklen_t)&addrlen)` | casting an address to a value, not to a pointer: undefined behaviour, `addrlen` garbage | `&addrlen` with `socklen_t addrlen = sizeof(client_addr);` |
| 2 | `if (err == ERR_CLSD)` | lwIP's `recv()` returns `0` when the peer closes; `ERR_CLSD` is not a `recv` return value | `err == 0` closes, `errno == EINTR/EWOULDBLOCK` retries, anything else is an error |
| 3 | `socklen_t fromlen;` used uninitialised | `recvfrom` may reject the address or write garbage | `fromlen = sizeof(from);` before every call |
| 4 | `recvfrom(..., MSG_WAITALL, ...)` | `MSG_WAITALL` is not honoured for datagrams and blocks the thread forever if the peer is silent | flags `0` + `delay_ms` polling |
| 5 | fixed `socklen_t` mismatch in `bind` on some lwIP builds | `sizeof(struct sockaddr)` vs `sizeof(sockaddr_in)` | `sizeof(local_addr)` everywhere |
| 6 | `send()` result ignored | silent data loss | return value checked, `delay_ms` + retry |
