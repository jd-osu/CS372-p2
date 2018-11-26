# !/usr/bin/python
# ftclient.py
# Author: Jason DiMedio
# CS372
# November 25, 2018
# This program is the client component of a FTP application. The client
# takes as command line arguments a server host name, a server port number,
# a command (and an optional filename argument for the command), and a data
# port. The client establishes a connection with a server and sends the
# data port and command information to the server. The server performs the
# desired action (sending a file or directory listing), and the client
# either displays the directory listing or stores the received file. 

import sys
from socket import *
import signal
import os

USAGE = "USAGE: " + str(sys.argv[0]) + " [server host] [server port] [command + filename(optional)] [data port]"

LIST = "-l"
GET = "-g"
ACK = "OK"

#******************************************************
# NAME
#    establish_control_connection
# DESCRIPTION
#    This function receives a socket as an argument
#    along with a host name, port number and data port number,
#    establishes a connection with a server and sends the
#    data port number to the server in anticipation of
#    receiving a data connection request from the server.
#    The file descriptor for the socket is then returned.
# ***************************************************
def establish_control_connection(socket, host, serv_port, dat_port):    
    # Configure the client control socket
	# The following code has been adapted from CS372, Lecture 15, slide 8
	# "Example application: TCP client"
	socket = socket(AF_INET, SOCK_STREAM)
	socket.connect((host, serv_port))
	
	socket.send(str(dat_port))

	response = socket.recv(1024)

	print "Connection established with " + str(socket.getpeername())
	
	return socket

#******************************************************
# NAME
#    send_request
# DESCRIPTION
#    This function receives a socket as an argument
#    along with a command string. The command string
#    is sent to the server via the socket and awaits
#    a response, which is returned.
# ***************************************************
def send_request(socket, cmd):    
	socket.send(cmd)
	response = socket.recv(1024)
	
	return response

#******************************************************
# NAME
#    establish_data_connection
# DESCRIPTION
#    This function receives a data port number as an argument
#    and sets up a socket to listen at the port number for an
#    incoming data connection from the server. The socket is
#    returned.
# ***************************************************
def establish_data_connection(dat_port):    
	# Configure the data socket
	# The following code has been adapted from CS372, Lecture 15, slide 9
	# "Example application: TCP server"
	socket = socket(AF_INET, SOCK_STREAM)
	socket.bind(('',dat_port))
	socket.listen(1)
	
	return socket

#******************************************************
# NAME
#    receive_data
# DESCRIPTION
#    This function receives a control socket, a data socket
#    and a string representing a control response from the
#    server. Based on the value of the string, the function
#    receives either a directory listing, which is displayed,
#    or a file, which is stored.
# ***************************************************
def receive_data(control_socket, data_socket, response):    
	if (response == LIST) :
		#signal server to send
		control_socket.send(ACK);
		
		connectionSocket, addr = data_socket.accept()
		
		print "Receiving directory structure from " + server_host + ":" + str(data_port) + "."
	  
		directory = connectionSocket.recv(2000)
		
		print directory
		
	elif (response == GET) :
		#signal server to send filename
		control_socket.send(ACK);
		filename = control_socket.recv(1024)
		
		#signal server to send size
		control_socket.send(ACK);
		size = int(control_socket.recv(1024))
		
		# signal server to send file
		control_socket.send(ACK)
		
		connectionSocket, addr = data_socket.accept()
		
		print "Receiving " + filename + "from " + server_host + ":" + str(data_port) + "."
	  
		file_contents=""
		
		while (len(file_contents) < size) :
			data = connectionSocket.recv(2000)
			file_contents += data
	  		
		if os.path.exists(filename):
			print "File with that name already exists! No file was transferred."
		else :
			file = open(filename, "w")
			
			file.write(file_contents)
			
			file.close()
			
			print "File transfer complete"
		
	else :
		print response # print error message from server

# Validate command line arguments
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

if (server_port <= 1024 or server_port > 65535 or data_port <= 1024 or data_port > 65535) :
    print "INVALID INPUT: Port must be in range 1025-65535"
    print USAGE
    exit(1)

# set up control connection with server
control_socket = establish_control_connection(socket, server_host, server_port, data_port)

# send request to server via control socket
response = send_request(control_socket, command)

#set up socket for any incoming data connections from server
data_socket = establish_data_connection(data_port)

#receive the data from the server (directory listing or file)
receive_data(control_socket, data_socket, response)

control_socket.close()

