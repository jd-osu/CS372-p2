Chat Application
Author: Jason DiMedio
CS372
October 28, 2018

Files:
	- chatserve.py
	- chatclient.c
	- makefile
	- README.txt (this file)

To compile (client only):
	Client:
	In the directory where the program files are stored, type:
		make

To run:
	Server:
	In the directory where the program files are stored, type:
		python chatserve.py [PORT NUMBER] (example: "python chatserve.py 99999")

	Client:
	In the directory where the program files are stored, type:
		client [HOSTNAME] [PORT NUMBER]   (example: "client flip1.engr.oregonstate.edu 99999")

To operate:

	Client:
		- Enter handle (1-10 characters, no spaces)
		- When prompted, enter a message to send to the server (1-500 characters).
		- The cursor will appear on a blank line while the program is awaiting a message from the server.
		- Messages from the server will be displayed on a new line with the server's handle.
		- Type "\quit" to close the connection.
		- If the connection is closed, either by the "\quit" command or by the server, the program will terminate.
		
	Server:
		- The default handle is "ChatServer".
		- The cursor will appear on a blank line while the program is awaiting a new chat connection or new message.
		- Messages from the client will appear on a new line with the client's handle prepended to the message.
		- When prompted, enter a message to send to the client (1-500 characters).
		- Type "\quit" to close the connection and listen for a new connection.
		- Type "Ctrl+C" to close any open connections and terminate the program.
		
		