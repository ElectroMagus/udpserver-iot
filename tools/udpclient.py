#!/usr/bin/env python
import socket
import sys
 

args                = len(sys.argv)
if args <  3:
    print("Invalid Syntax:   %s <host> <port> <data>"% sys.argv[0])
    sys.exit(1)


host                = str(sys.argv[1])
port                = int(sys.argv[2])
msgFromClient       = str(sys.argv[3])
numPackets          = 1
bufferSize          = 1024
serverAddressPort   = (host, 20001)

# Create a UDP socket at client side
UDPClientSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)

counter = 1
while counter <= 1:

    # Send to server using created UDP socket
    #msgFromClient = str(counter)
    bytesToSend = str.encode(msgFromClient)
    UDPClientSocket.sendto(bytesToSend, serverAddressPort)
    UDPClientSocket.settimeout(3)
    msgFromServer = UDPClientSocket.recvfrom(bufferSize)
    msg = msgFromServer[0]
    print("%s: %s" % (counter, msg))
    counter += 1
