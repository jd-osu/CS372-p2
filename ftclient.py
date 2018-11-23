# !/usr/bin/python
# ftclient.py
# Author: Jason DiMedio
# CS372
# November 25, 2018
# [description]

import sys
from socket import *
import signal

USAGE = "USAGE: " + str(sys.argv[0]) + " [server host] [server port] [command + filename(optional)] [data port]"

# Make sure there are at least one command line argument
if (len(sys.argv) < 5) : 
    print USAGE
    exit(1)

server_host = str(sys.argv[1])
server_port = sys.argv[2]
command = str(sys.argv[3])

if (len(sys.argv) >= 6) :
    command = command + " " + str(sys.argv[4])
    data_port = sys.argv[5]
else :
    data_port = sys.argv[4]
    
if (not(server_port.isdigit) or
    not(data_port.isdigit) or
    server_port < 1024 or
    server_port > 1024 or
    data_port < 1024 or
    data_port > 65535) :
    print "Invalid port. Port must be integer in range: 1024-65535")
    print USAGE
    exit(1)

print "server_host= " + server_host
print "server_port= " + server_port
print "command= " + command
print "data_port= " + data_port


"""
notice = "sending"
ack = "OK"
quit_cmd = "\\quit"
handle = "ChatServer"

# The signal handling code included here and below was adapted from:
# "Stack Overflow: How do I capture SIGINT in Python?"
# https://stackoverflow.com/questions/1112343/how-do-i-capture-sigint-in-python
def signal_handler (sig, frame):
    print ""
    sys.exit(0)
signal.signal(signal.SIGINT, signal_handler)

#******************************************************
# NAME
#    get_ack
# DESCRIPTION
#    This function receives a socket as an argument
#    and exchanges a series of preparatory messages with
#    the client in order to confirm the connection is
#    still good. The function returns true if an ack
#    was received and false otherwise.
# ***************************************************
def get_ack(socket):    
    ready = False
    notice_sent = socket.send(notice)
    
    if notice_sent > 0 :
        ack_in = socket.recv(len(ack)+1)
        if ack_in == ack :
            ready = True
            
    return ready





# Configure the socket
# The following code has been adapted from CS372, Lecture 15, slide 9
# "Example application: TCP server"
serverSocket = socket(AF_INET, SOCK_STREAM)
serverSocket.bind(('',port))
serverSocket.listen(1)

# Listen continuously at the specified port (unless a SIGINT is caught)
while 1:
    print "Listening for new chat connection..."
    connectionSocket, addr = serverSocket.accept()

    #initialize message length, connection status and quit status
    in_msg_length = 1
    conn_good = True
    stop = False
    
    print "Chat connection established."
    
    #continue to receive messages while the connection is good and quit is not indicated
    while in_msg_length != 0 and conn_good and not stop:
        total_read = 0
        in_msg = ""
        
        in_msg = connectionSocket.recv(2000)
        in_msg_length = len(in_msg)
        
        # if a message of any length was received
        if in_msg_length != 0 :
            #if the message matches the "notice" command (e.g. "sending")
            if in_msg == notice :
                # send acknowledgment back in response
                connectionSocket.send(ack)
                
            # otherwise display the message and send response 
            else :
                
                print in_msg
                
                # receive the outgoing message as user input
                input = raw_input(handle+": ")
                
                if input == quit_cmd :
                    stop = True
                else:                
                    out_msg = handle + "> " + input
         
                    # if the connection is still good send the outgoing message
                    if get_ack(connectionSocket) :
                        connectionSocket.sendall(out_msg)
                    else:
                        conn_good = False

    print "Chat connection closed."
    

    connectionSocket.close()

"""

