/************************************************
 * ftserver.c
 * Author: Jason DiMedio
 * CS372
 * November 25, 2018
 * [description]
 * *********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <dirent.h>

#define HANDLEMAX 10	// max length of handles
#define MESSAGEMAX 500  // max length of message text
#define BUFFERMAX 1000	// max length of string buffer

// bool type defined as true/false logic is used extensively
typedef enum {false, true} bool;

/************************************************
 * NAME
 *   error
 * DESCRIPTION
 *   This program takes as an argument a string
 *   representing an error message and an int
 *   representing an exit conditon and outputs
 *   the message to stderr and exits.
 * *********************************************/
void error(const char *msg, int e)
{
  fprintf(stderr,"%s\n", msg);
  exit(e);
}

/******************************************************
* NAME
*    sendall
* DESCRIPTION
*    This function receives a socket, a string message
*    and a pointer to an int containing the string length
*    as arguments and continues sending portions of the
*    message until the entire message was sent.
*
* NOTE: This function was adapted from:
* "Beej's Guide to Network Programming: Using Internet Sockets"
* http://beej.us/guide/bgnet/html/single/bgnet.html
****************************************************/
int sendall(int s, char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n, ret;

	while(total < *len) {
		n = send(s, buf+total, bytesleft, 0);
		if (n == -1)
		{
			ret = -1;
			break;
		}
		total += n;
		bytesleft -= n;
	}

	*len = total; // return number actually sent here

	ret = total;

    return ret;
}
//TODO: Server has at least functions which perform: Start-up, HandleRequest

/************************************************
 * NAME
 *   read_file 
 * DESCRIPTION
 *   This program takes as an argument a string
 *   representing a filename, opens a file for
 *   reading and then reads the contents of the
 *   file into a string, which it then returns.
 *
 *NOTE: This function was adapted from a previously
 *submitted homework assignment for CS344 by the
 *author of this program. 
 * *********************************************/
char *read_file(const char *filename)
{
  char error_text[100];
  FILE *file;
  char *text = NULL;
  int c;
  int cap, len;

  // open the file
  file = fopen(filename, "r");
  if (file == NULL)
  {
    sprintf(error_text, "Could not open %s", filename);
    error(error_text, 1);
  }

  // allocate memory for string and clear it
  cap = 100 * sizeof(char);
  text = malloc(cap);
  if (text == NULL)
    error("Could not allocate memory", 1);

  int i = 0;

  // read text from file char by char
  while ((c = getc(file)) != EOF)
  {
    // check if enough memory is allocated and reallocate if necessary
    if (i >= (cap - 1))
    {
      cap = 100 * cap;
      text = realloc(text, cap);
      if (text == NULL)
        error("Could not allocate memory", 1);
    }

    text[i] = c;
    i++;
  }

  // remove final newline 
  //if (text[i-1] == '\n')
  //  text[i-1] = '\0';

  // check if there are any other newlines (which would be invalid)
  //if (strstr(text, "\n") != NULL)
  //{
    //sprintf(error_text, "Invalid char in %s", filename);
    //error(error_text, 1); 
  //}

  return text;
}

/************************************************
 * NAME
 *   send_directory
 * DESCRIPTION
 *
 *NOTE: The main part of this function was adapted from:
 *"How to list files in a directory in a C program?"
 * https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
 * *********************************************/
void send_directory(int port, char *address)
{
  char error_text[100];
  char *text = NULL;
  int c;
  int cap, len;
    
  // allocate memory for string and clear it
  cap = 100 * sizeof(char);
  text = malloc(cap);
  if (text == NULL)
    error("Could not allocate memory", 1);
  
  DIR *d;
  
  int i = 0;
    
  struct dirent *dir;
  d = opendir(".");
  if (d) {
    while ((dir = readdir(d)) != NULL) {
		i = i + strlen(dir->d_name);
		
		// check if enough memory is allocated and reallocate if necessary
		if (i >= (cap - 1))
		{
			cap = 100 * cap;
			text = realloc(text, cap);
			if (text == NULL)
				error("Could not allocate memory", 1);
		}
		
		sprintf(text,"%s\n", dir->d_name);
	}
	
	closedir(d);
  }

  printf("Directory text:\n%s", text);

  free(text);
}

/******************************************************
* NAME
*    send_file
* DESCRIPTION
****************************************************/
void send_file(int s, char *filename)
{
  char *contents;

  // read text from file
  contents = read_file(filename);

  
  
  free(contents);
}

/************************************************
 * NAME
 *	process_command
 * DESCRIPTION
 *	
 * *********************************************/
void process_command(char* command, char* response, int port, char* address)
{
  // global constant variables for commands
  static const char list[] = "-l";
  static const char get[] = "-g";
  static const char file_dir[] = "[file directory!]";
  static const char get_res[] = "[getting file!]";
  static const char invalid_cmd[] = "Invalid command!";
  
  char buffer[1000];
  
  // The following code for getting the command substring was adapted from:
  // "Get a substring of a char* [duplicate]"
  // https://stackoverflow.com/questions/4214314/get-a-substring-of-a-char
  char cmd_substr[3];
  memcpy(cmd_substr, &command[0], 2);
  cmd_substr[2] = '\0';
  
  if (strcmp(cmd_substr, list) == 0)
  {
	send_directory(port, address);	
	
    strcpy(response, file_dir);
  }
  else if (strcmp(cmd_substr, get) == 0)
  {
	char filename_substr[100];
	
	if (strlen(command) > 3 && command[2] == ' ')
	{
      memcpy(filename_substr, &command[3], strlen(command)-3);
      filename_substr[strlen(command)-3] = '\0';
	  
	  strcpy(response, get_res);
	}
	else
      strcpy(response, invalid_cmd);
  }
  else
    strcpy(response, invalid_cmd);

}

int main(int argc, char **argv)
{
  // Make sure there is at least 1 command line argument
  if (argc < 2)
  {
    fprintf(stderr, "USAGE: %s [server port]\n", argv[0]);
    exit(1);
  }

  int server_port = atoi(argv[1]);
  int data_port = 0;
    
  /* Configure the socket
   * The following code was adapted from CS344, Lecture 4.3, slide 16
   * "server.c"
   */
  int listenSocketFD, establishedConnectionFD, charsRead;
  socklen_t sizeOfClientInfo;
  char buffer[256];
  struct sockaddr_in serverAddress, clientAddress;
  
  // set up address struct for server
  memset((char *)&serverAddress, '\0', sizeof(serverAddress));
  
  //create socket
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(server_port);
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  
  //set up socket
  listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocketFD < 0) error("ERROR opening socket", 1);
  
  // Listen for connection
  if (bind(listenSocketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
	  error("ERROR binding", 1);
  listen(listenSocketFD, 5);
  
  printf("Server open on %d\n", server_port);

  while(1)
  {
	  //accept connection
	  sizeOfClientInfo = sizeof(clientAddress);
	  establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
	  if (establishedConnectionFD < 0) error("ERROR on accept", 1);
	  
	  // The following line of code (inet_ntoa in particular) adapted from:
	  // "How to get ip address from sock structure in c?"
	  // https://stackoverflow.com/questions/3060950/how-to-get-ip-address-from-sock-structure-in-c
	  printf("Connection from %s\n", inet_ntoa(clientAddress.sin_addr));
	  
	  
	  // get data port first
	  memset(buffer, '\0', 256);
	  charsRead = recv(establishedConnectionFD, buffer, 255, 0);
	  if (charsRead < 0) error("ERROR reading from socket", 1);
	  printf("1. I received this from the client: %s\n", buffer);
	  data_port = atoi(buffer);
	  printf("Here is the data_port as an int: %d\n", data_port);
	  
	  // send "OK" in response
	  charsRead = send(establishedConnectionFD, "OK", 2, 0);
	  if (charsRead < 0) error("ERROR writing to socket", 1);
	  
	  
	  // get command
	  memset(buffer, '\0', 256);
	  charsRead = recv(establishedConnectionFD, buffer, 255, 0);
	  if (charsRead < 0) error("ERROR reading from socket", 1);
	  printf("2. I received this from the client: %s\n", buffer);
	  char response_text[300];
	  
	  // process command and send response text
	  process_command(buffer, response_text, data_port);
	  charsRead = send(establishedConnectionFD, response_text, strlen(response_text), 0);
	  if (charsRead < 0) error("ERROR writing to socket", 1);
	  
	  close(establishedConnectionFD);
	  data_port = 0;
  }
  
  close(listenSocketFD);
  
  printf("Control connection closed.\n");
  
  return EXIT_SUCCESS;
  
  
  /*
  char handle[HANDLEMAX+1];
  char message[HANDLEMAX+MESSAGEMAX+3];
  bool conn_good = true;

  get_handle(handle);

  /* Configure the socket
   * The following code was adapted from CS344, Lecture 4.2, slide 21
   * "client.c"
   *//*
  int socketFD;
  int charsRead = 1;

  struct sockaddr_in serverAddress;
  struct hostent* serverHostInfo;

  memset((char*)&serverAddress, '\0', sizeof(serverAddress));
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(port);
  serverHostInfo = gethostbyname(argv[1]);

  if (serverHostInfo == NULL) error("ERROR, no such host",1);

  memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length);

  socketFD = socket(AF_INET, SOCK_STREAM, 0);
  if (socketFD < 0) error("ERROR opening socket", 1);

  printf("Connecting to server for new chat connection...\n");

  //connect to server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
	  error("ERROR connecting", 1);

  printf("Chat connection established.\n");

  // as long as the connection is still good and quit isn't indicated
  while (conn_good == true && get_message_input(message, handle) == true)
  {
	  int msg_len = strlen(message);
	  int msg_sent;

	  // send message to server
	  msg_sent = sendall(socketFD, message, &msg_len);

	  // if there was an error during sending
	  if (msg_sent < 0)
		  error("ERROR writing to socket", 1);
	  // sent successfully
	  else if (msg_sent > 0)
	  {
		  //get return message from server
		  memset(message, '\0', sizeof(message));
		  charsRead = recv(socketFD, message, sizeof(message)-1, 0);

		  // if incoming message is a "sending" indicator, reply with ack
		  if(strcmp(message, notice) == 0)
		  {
			  send(socketFD, ack, strlen(ack), 0);
			  memset(message, '\0', sizeof(message));
			  charsRead = recv(socketFD, message, sizeof(message)-1, 0);
		  }

		  // if there was an error reading
		  if (charsRead < 0) error ("ERROR reading from socket",1);

		  // if the connection is closed
		  else if (charsRead == 0)
			  conn_good = false;

		  // if the message was received, display it
		  else
			  printf("%s\n", message);
	  }
	  else
		  conn_good = false;
  }

  close(socketFD);

  printf("Chat connection closed.\n");

  */
}
