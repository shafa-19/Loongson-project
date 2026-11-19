#!/usr/bin/env python3
"""PC side of the TCP *client* test: listens on 192.168.1.111:9061 and echoes
whatever the board sends."""
import socket, sys

HOST, PORT = "192.168.1.111", 9061

def main():
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((HOST, PORT))
    srv.listen(1)
    print("listening on %s:%d ..." % (HOST, PORT))
    conn, addr = srv.accept()
    print("board connected from %s:%d" % addr)
    n = 0
    while n < 5:
        data = conn.recv(256)
        if not data:
            break
        n += 1
        print("received: %s" % data.decode(errors="replace").strip())
        conn.sendall(data)
    conn.close()
    srv.close()
    print("done, %d messages echoed" % n)
    return 0

if __name__ == "__main__":
    sys.exit(main())
