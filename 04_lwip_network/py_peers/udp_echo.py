#!/usr/bin/env python3
"""PC side of the UDP *server* test: echoes datagrams back to the board."""
import socket, sys

HOST, PORT = "192.168.1.111", 9062

def main():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind((HOST, PORT))
    print("UDP echo listening on %s:%d ..." % (HOST, PORT))
    for _ in range(5):
        data, addr = s.recvfrom(1024)
        print("from %s: %s" % (addr, data.decode(errors="replace").strip()))
        s.sendto(data, addr)
    print("done")
    return 0

if __name__ == "__main__":
    sys.exit(main())
