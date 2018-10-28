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
    
    while in_msg_length != 0:
        total_read = 0
        in_msg = ""
        
        stop = False
     
 #       while stop == False :
        temp = connectionSocket.recv(1024)
        print "read ", len(temp), " bytes"
        print "temp= ", temp
        for i in range(0, len(temp)-1):
            if temp[i] == 4 :
                stop = True
                temp[i] = '\0'
                break
        in_msg = in_msg + temp
        total_read += len(temp)
        print "in_msg= ", in_msg
        print "total_read= ", total_read
  
        in_msg_length = len(in_msg)
        
        if in_msg_length != 0 :
            print in_msg
            out_msg = handle + "> " + raw_input(handle+": ") + chr(4)
            connectionSocket.send(out_msg)

    print "connection was closed."
    

#    connectionSocket.close()


