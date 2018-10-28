/************************************************
 * [program name]
 * Author:
 * CS372
 * October 27, 2018
 * This program ...
 * *********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

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

/************************************************
 * NAME
 *
 * DESCRIPTION
 *
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
 *
 * DESCRIPTION
 *
 * *********************************************/
void get_handle(char* handle)
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
 *
 * DESCRIPTION
 *
 * *********************************************/
bool get_message_input(char* msg, char* handle)
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

/*

 */
bool get_ack(int s)
{
    char notice[] = "sending";
    char ack[] = "OK";
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

/*
 * NOTE: This function was adapted from:
 * "Beej's Guide to Network Programming: Using Internet Sockets"
 * http://beej.us/guide/bgnet/html/single/bgnet.html
 */
int sendall(int s, char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
}



int main(int argc, char **argv)
{
  // if wrong number of arguments entered
  if (argc < 3)
  {
    fprintf(stderr, "USAGE: %s hostname port\n", argv[0]);
    exit(1);
  }

  char *hostname = argv[1];
  int port = atoi(argv[2]);
  char handle[HANDLEMAX+1];
  char message[HANDLEMAX+MESSAGEMAX+4];

  get_handle(handle);

  /* The following code was adapted from CS344, Lecture 4.2, slide 21
   * "client.c"
   */
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

  //set up the socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0);
  if (socketFD < 0) error("ERROR opening socket", 1);

  //connect to server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
	  error("ERROR connecting", 1);

  ;

  while (charsRead != 0)
  {
	  while (get_message_input(message, handle) == true)
	  {
		int msg_len = strlen(message);

		// send message to server
		// This code was adapted from "Beej's Guide to Network Programming"
		// (See sendall() function above for more information
		printf("sending string \"%s\" with length of %d\n", message, msg_len);
		if (sendall(socketFD, message, &msg_len) == -1)
		  error("ERROR writing to socket", 1);

		//get return message from server
		memset(message, '\0', sizeof(message));
		charsRead = recv(socketFD, message, sizeof(message)-1, 0);
		if (charsRead < 0) error ("ERROR reading from socket",1);

		printf("%s\n", message);
	  }
  }

  close(socketFD);

  return EXIT_SUCCESS;
}
