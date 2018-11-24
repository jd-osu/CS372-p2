# !/usr/bin/python
# ftclient.py
# Author: Jason DiMedio
# CS372
# November 25, 2018
# [description] test change  

import sys
from socket import *
import signal

USAGE = "USAGE: " + str(sys.argv[0]) + " [server host] [server port] [command + filename(optional)] [data port]"

LIST = "-l"
GET = "-g"
ACK = "OK"

#TODO: Client has at least functions which perform: Initiate Contact, MakeRequest, ReceiveData

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

if (not(server_port.isdigit()) or not(data_port.isdigit())) :
    print "INVALID INPUT: Ports must be integers."
    print USAGE
    exit(1)
    
server_port = int(server_port)
data_port = int(data_port)

if (server_port < 1024 or server_port > 65535 or data_port < 1024 or data_port > 65535) :
    print "INVALID INPUT: Port must be in range 1024-65535"
    print USAGE
    exit(1)


print "server_host= " + server_host
print "server_port= " + str(server_port)
print "command= " + command
print "data_port= " + str(data_port)

# Configure the client control socket
# The following code has been adapted from CS372, Lecture 15, slide 8
# "Example application: TCP client"
control_socket = socket(AF_INET, SOCK_STREAM)
control_socket.connect((server_host, server_port))

# Configure the data socket
# The following code has been adapted from CS372, Lecture 15, slide 9
# "Example application: TCP server"
data_socket = socket(AF_INET, SOCK_STREAM)
data_socket.bind(('',data_port))
data_socket.listen(1)
	
print "Connection established with " + str(control_socket.getpeername())

control_socket.send(str(data_port))

response = control_socket.recv(1024)

print "1. Response from server: " + response

control_socket.send(command)

response = control_socket.recv(1024)

print "2. Response from server: " + response

if (response == LIST) :
	#signal server to send
	control_socket.send(ACK);
	
	connectionSocket, addr = data_socket.accept()
	
	print "Receiving directory structure from " + server_host + ":" + data_port + "."
  
	data = connectionSocket.recv(2000)
    
	print "Directory listing:\n" + data
	
elif (response == GET) :
	#signal server to send
	control_socket.send(ACK)
	
	connectionSocket, addr = data_socket.accept()
	
	data = connectionSocket.recv(2000)
	
	print "File contents:\n" + data
	
print response
	
control_socket.close()


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

