#!/usr/bin/env python
import socket
import sys
 

args = len(sys.argv)
if args <  3:
    print("Invalid Syntax:   %s <host> <port> <data> <# times>"% sys.argv[0])
    sys.exit(1)


host                = str(sys.argv[1])
port                = int(sys.argv[2])
msgFromClient       = str(sys.argv[3])
numPackets          = int(sys.argv[4])
bufferSize          = 1024
serverAddressPort   = (host, port)

# Create a UDP socket at client side
UDPClientSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)

# Send to server using created UDP socket
bytesToSend = str.encode(msgFromClient)
counter = 0
while counter < numPackets:
    UDPClientSocket.sendto(bytesToSend, serverAddressPort)
    # Specify timeout for failed connections to prevvent long hangs
    UDPClientSocket.settimeout(3)
    # Recieve Reponse from Server
    msgFromServer = UDPClientSocket.recvfrom(bufferSize)
    msg = msgFromServer[0]
    print("Server Response: %s" % (msg))
    counter += 1
