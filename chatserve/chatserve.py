# !/usr/bin/python
# mypython
# Author: 
# CS344
# February 24, 2018
# This program ...

import sys
from socket import *
import signal

notice = "sending"
ack = "OK"
quit_cmd = "\\quit"

# The signal handling code included here and below was adapted from:
# "Stack Overflow: How do I capture SIGINT in Python?"
# https://stackoverflow.com/questions/1112343/how-do-i-capture-sigint-in-python
def signal_handler (sig, frame):
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

def get_ack(socket):    
    ready = False
    notice_sent = socket.send(notice)
    
    if notice_sent > 0 :
        ack_in = socket.recv(len(ack)+1)
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
    print "Listening for new connection..."
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

    print "Connection was closed."
    

    connectionSocket.close()


