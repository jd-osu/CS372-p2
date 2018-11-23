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

#define HANDLEMAX 10	// max length of handles
#define MESSAGEMAX 500  // max length of message text
#define BUFFERMAX 1000	// max length of string buffer

// global constant variables for preparatory messages
static const char notice[] = "sending";
static const char ack[] = "OK";

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

/************************************************
 * NAME
 *	get_input
 * DESCRIPTION
 *	This is a generic function for getting string
 *	input from the user. It takes a prompt string
 *	and a pointer to a string to store the value.
 *	The function also replaces any terminating
 *	newline with a null character.
 * *********************************************/
void get_input(char* prompt, char* in)
{
  memset(in, '\0', sizeof(in));

  size_t input_size = BUFFERMAX;
  size_t chars;

  printf("%s: ", prompt);
  chars = getline(&in, &input_size, stdin);

  if (in[strlen(in)-1] == '\n')
    in[strlen(in)-1] = 0;
}

/************************************************
 * NAME
 *	get_handle
 * DESCRIPTION
 *	This function takes a string pointer as an argument
 *	and stores a value input by the user based on
 *	a hardcoded prompt specific to obtaining the
 *	handle. The value is first validated to ensure
 *	it is 1-10 characters with no spaces.
 * *********************************************/
void get_handle(char* handle)  // candidate for removal
{
  int max = HANDLEMAX;
  char *input;
  char prompt_str[] = "Enter handle";
  bool valid_input = false;

  char space_str[] = " ";

  size_t input_size = 50;

  // allocate memory for input string
  input = (char *)malloc(input_size * sizeof(char));
  if (input == NULL)
	  error("Unable to allocate (get_handle)", 1);

  // until a valid input is received...
  while (valid_input == false)
  {
    get_input(prompt_str, input);

    // if the input is one word and between 1-10 characters it is valid
    if ((strstr(input, space_str) == NULL) && (strlen(input) > 0) && (strlen(input) <= max))
      valid_input = true;
    else
      printf("Handle must be 1-%d characters with no spaces.\n", max);
  }

  strcpy(handle, input);

  // free dynamically allocated memory
  free(input);
}

/************************************************
 * NAME
 *	get_message_input
 * DESCRIPTION
 *	This function takes a string pointer as an argument
 *	and stores a value input by the user based on
 *	a prompt (specifically the user handle) passed as an
 *	argument. The value is first validated to ensure
 *	it is 1-500 characters with no spaces. Additionally,
 *	the function returns false if the "quit" command word
 *	is received as input.
 * *********************************************/
bool get_message_input(char* msg, char* handle)  // candidate for removal
{
  int max = MESSAGEMAX;
  bool valid_input = false;
  char buffer[BUFFERMAX+1];
  char close_cmd[] = "\\quit";

  memset(buffer, '\0', sizeof(buffer));
  memset(msg, '\0', sizeof(msg));

  // until a valid input is received...
  while (valid_input == false)
  {
    get_input(handle, buffer);

    if (strlen(buffer) > 0 && strlen(buffer) <= max)
      valid_input = true;
    else
      printf("Message must be 1-%d characters.\n", max);
  }

  if (strcmp(buffer, close_cmd) == 0)
	  return false;

  strcat(msg, handle);
  strcat(msg, "> ");
  strcat(msg, buffer);

  return true;
}

/******************************************************
* NAME
*    get_ack
* DESCRIPTION
*    This function receives a socket as an argument
*    and exchanges a series of preparatory messages with
*    the server in order to confirm the connection is
*    still good. The function returns true if an ack
*    was received and false otherwise.
# ***************************************************/
bool get_ack(int s)
{
    char ack_in[3];
    bool ready = false;
    int notice_sent, ack_recvd;

    notice_sent = send(s, notice, strlen(notice), 0);
    if (notice_sent > 0)
    {
    	ack_recvd = recv(s, ack_in, sizeof(ack_in)-1, 0);
    	if (strcmp(ack, ack_in) == 0)
    		ready = true;
    }

    return ready;
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

    if (get_ack(s))
    {
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
    }
    else
    	ret = 0;

    return ret;
}
//TODO: Server has at least functions which perform: Start-up, HandleRequest
/************************************************
 * NAME
 *	process_command
 * DESCRIPTION
 *	
 * *********************************************/
void process_command(char* command, char* response)
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
    strcpy(response, file_dir);
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

  int port = atoi(argv[1]);
    
  /* Configure the socket
   * The following code was adapted from CS344, Lecture 4.3, slide 16
   * "server.c"
   */
  int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
  socklen_t sizeOfClientInfo;
  char buffer[256];
  struct sockaddr_in serverAddress, clientAddress;
  
  // set up address struct for server
  memset((char *)&serverAddress, '\0', sizeof(serverAddress));
  
  //create socket
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(port);
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  
  //set up socket
  listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocketFD < 0) error("ERROR opening socket", 1);
  
  // Listen for connection
  if (bind(listenSocketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
	  error("ERROR binding", 1);
  listen(listenSocketFD, 5);
  
  printf("Server open on %d\n", port);

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
	  
	  memset(buffer, '\0', 256);
	  charsRead = recv(establishedConnectionFD, buffer, 255, 0);
	  if (charsRead < 0) error("ERROR reading from socket", 1);
	  printf("I received this from the client: %s\n", buffer);
	  
	  char response_text[300];
	  process_command(buffer, response_text);
	  
	  charsRead = send(establishedConnectionFD, response_text, strlen(response_text), 0);
	  if (charsRead < 0) error("ERROR writing to socket", 1);
	  
	  close(establishedConnectionFD);
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
