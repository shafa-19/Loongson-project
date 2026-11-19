#!/usr/bin/env python3
"""PC side of the UDP *client* test: sends 5 datagrams to the board, which
echoes them back."""
import socket, sys, time

HOST, PORT = "192.168.1.123", 9062

def main():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(3)
    for i in range(5):
        msg = ("udp test %d" % (i + 1)).encode()
        s.sendto(msg, (HOST, PORT))
        try:
            data, addr = s.recvfrom(1024)
            print("sent: %-12s echo: %s" % (msg.decode(), data.decode(errors="replace")))
        except socket.timeout:
            print("sent: %-12s echo: <timeout>" % msg.decode())
        time.sleep(0.3)
    s.close()
    return 0

if __name__ == "__main__":
    sys.exit(main())
