#!/usr/bin/env python3
"""PC side of the TCP *server* test: connects to the board (192.168.1.123:9060),
sends 5 messages, prints the echo."""
import socket, sys, time

HOST, PORT = "192.168.1.123", 9060

def main():
    s = socket.create_connection((HOST, PORT), timeout=5)
    print("connected to %s:%d" % (HOST, PORT))
    print("banner:", s.recv(256).decode(errors="replace").strip())
    for i in range(5):
        msg = "hello from PC, message %d" % (i + 1)
        s.sendall(msg.encode())
        echo = s.recv(256).decode(errors="replace")
        print("sent    : %s" % msg)
        print("received: %s" % echo)
        time.sleep(0.2)
    s.close()
    print("done")
    return 0

if __name__ == "__main__":
    sys.exit(main())
