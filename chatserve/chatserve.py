# !/usr/bin/python
# mypython
# Author: 
# CS344
# February 24, 2018
# This program ...

import sys
from socket import *

notice = "sending"
ack = "OK"
quit_cmd = "\\quit"

def get_ack(socket):
    print "beginning of get_ack"
    print "socket=", socket
    print "notice=", notice
    print "ack=", ack
    
    ready = False
    notice_sent = socket.send(notice)
    
    print "notice_sent=", notice_sent
    
    if notice_sent > 0 :
        ack_in = socket.recv(len(ack)+1)
        print "ack_in=", ack_in
        if ack_in == ack :
            ready = True
            
    return ready

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
    conn_good = True
    stop = False
    
    while in_msg_length != 0 and conn_good and not stop:
        total_read = 0
        in_msg = ""
        
        in_msg = connectionSocket.recv(2000)
        in_msg_length = len(in_msg)
        
        if in_msg_length != 0 :
            if in_msg == notice :
                connectionSocket.send(ack)
            else :
                print in_msg
                
                input = raw_input(handle+": ")
                
                if input == quit_cmd :
                    stop = True
                else:                
                    out_msg = handle + "> " + input
                
                    if get_ack(connectionSocket) :
                        connectionSocket.sendall(out_msg)
                    else:
                        conn_good = False

    print "connection was closed."
    

#    connectionSocket.close()


