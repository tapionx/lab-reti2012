#!/usr/bin/python
import socket
import sys
import time

HOST, PORT = "localhost", 59000

# Create a socket (SOCK_STREAM means a TCP socket)
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Connect to server and send data
sock.connect((HOST, PORT))

for i in range(1,1000):
	sock.send(str(i) + "\n")
	print i

time.sleep(20)

