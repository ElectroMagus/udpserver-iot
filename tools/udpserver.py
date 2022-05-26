#!/usr/bin/env python
import socket
import sys

args = len(sys.argv)
if args <  1:
    print("Invalid Syntax:   %s <port>"% sys.argv[0])
    print("Note that non-root users should select a port above 1024")
    sys.exit(1)


localIP     = ""
localPort   = int(sys.argv[1])
bufferSize  = 1024
msgFromServer       = "ACK"
bytesToSend         = str.encode(msgFromServer)

# Create a datagram socket
UDPServerSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)

# Bind to address and ip
UDPServerSocket.bind((localIP, localPort))
print("UDP server listening on port %s" % sys.argv[1])

# Listen for incoming datagrams
while(True):
    bytesAddressPair = UDPServerSocket.recvfrom(bufferSize)
    message = bytesAddressPair[0]
    address = bytesAddressPair[1]
    clientMsg = message
    clientIP  = address[0]
    print("%s: %s" % (clientIP, clientMsg))
    # Sending a reply to client
    UDPServerSocket.sendto(bytesToSend, address)
