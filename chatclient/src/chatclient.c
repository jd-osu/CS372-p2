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
void get_handle(char* handle)
{
  char *input;
  int valid_input = 0;

  char space_str[] = " ";

  size_t input_size = 50;

  // allocate memory for input string
  input = (char *)malloc(input_size * sizeof(char));
  if (input == NULL)
	  error("Unable to allocate (get_handle)", 1);

  // until a valid input is received...
  while (valid_input == 0)
  {
    // prompt the user
    printf("Enter handle: ");
    getline(&input, &input_size, stdin);

    if (input[strlen(input)-1] == '\n')
      input[strlen(input)-1] = 0;

    if ((strstr(input, space_str) == NULL) && (strlen(input) > 0) && (strlen(input) <= 10))
      valid_input = 1;
    else
      printf("Handle must be 1-10 characters with no spaces.\n");
  }

  strcpy(handle, input);

  // free dynamically allocated memory
  free(input);
}

int main(int argc, char **argv)
{
  char error_text[100];

  // if wrong number of arguments entered
  if (argc < 3)
  {
    fprintf(stderr, "USAGE: %s hostname port\n", argv[0]);
    exit(1);
  }

  char *hostname = argv[1];
  int port = atoi(argv[2]);
  char handle[11];

  get_handle(handle);

  printf("Handle is: %s\n", handle);
  printf("Hostname is: %s\n", hostname);
  printf("Port is: %d\n", port);

  /* The following code was adapted from CS344, Lecture 4.2, slide 21
   * "client.c"
   */
  int socketFD, charsWritten, charsRead;

  struct sockaddr_in serverAddress;
  struct hostent* serverHostInfo;
  char buffer[256];

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

  //get input message from user
  printf("Enter text to send to the server: ");
  memset(buffer, '\0', sizeof(buffer));
  fgets(buffer, sizeof(buffer) - 1, stdin);
  buffer[strcspn(buffer, "\n")] = '\0';

  // send message to server
  charsWritten = send(socketFD, buffer, strlen(buffer), 0);
  if (charsWritten < 0) error("ERROR writing to socket", 1);
  if (charsWritten < strlen(buffer)) printf("WARNING: Not all data written to socket.\n");

  //get return message from server
  memset(buffer, '\0', sizeof(buffer));
  charsRead = recv(socketFD, buffer, sizeof(buffer)-1, 0);
  if (charsRead < 0) error ("ERROR reading from socket", 1);
  printf("Received from server: \"%s\"\n", buffer);

  close(socketFD);

  return EXIT_SUCCESS;
}
