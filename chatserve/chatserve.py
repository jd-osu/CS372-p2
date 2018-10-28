# !/usr/bin/python
# mypython
# Author: 
# CS344
# February 24, 2018
# This program ...

import sys
from socket import *

if (len(sys.argv) < 2) : 
    print "USAGE: ", str(sys.argv[0]), " port"
    exit(1)

port = int(sys.argv[1])

handle = "ChatServer"

# The following code has been adapted from CS372, Lecture 15, slide 9
# "Example application: TCP server"
serverSocket = socket(AF_INET, SOCK_STREAM)
serverSocket.bind(('',port))
serverSocket.listen(1)

while 1:
    print "listening for new connection..."
    connectionSocket, addr = serverSocket.accept()

    in_msg_length = 1
    out_msg_length = 1
    
    while in_msg_length != 0 and out_msg_length != 0 :
        in_msg = connectionSocket.recv(1024)
        in_msg_length = len(in_msg)
        
        out_msg = raw_input(handle+":")
        out_msg_length = connectionSocket.send(out_msg)

    print "connection was closed."
    

#    connectionSocket.close()


