#!/usr/bin/python

import socket
import sys

TCP_IP = '127.0.0.1'
TCP_PORT = 64000
BUFFER_SIZE = 2048  # Normally 1024, but we want fast response

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind((TCP_IP, TCP_PORT))
s.listen(1)

conn, addr = s.accept()
print 'Connection address:', addr

a = 1

while 1:
    data = conn.recv(BUFFER_SIZE)
    #data = ''.join(data[0:3].split())
    print int(data),
    sys.stdout.flush()
    #if int(data) != a:
	#	print "ERRORE ERRORE ERRORE ERRORE"
    a = a + 1
conn.close()
