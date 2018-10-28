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
#define BUFFERMAX 300	// max length of string buffer
#define TX_LIMIT 1000

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
  printf("%s: ", prompt);
  memset(in, '\0', sizeof(in));
  fgets(in, sizeof(in) - 1, stdin);
  in[strcspn(in, "\n")] = '\0';
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


int send_message(int socket, char *buf)
{
  int buf_len = strlen(buf);
  char buf_len_str [5];
  char ack_str[5];
  char ok[] = "OK";
  sprintf(buf_len_str, "%d", buf_len);

  send(socket, buf_len_str, strlen(buf_len_str), 0);

  //get ack message from server
  memset(ack_str, '\0', sizeof(ack_str));
  int ackRead = recv(socket, ack_str, sizeof(ack_str)-1, 0);
  if (ackRead < 0) error ("ERROR reading from socket", 1);

  if (strcmp(ack_str, ok) == 0)
  {
    // send message to server
    // This code was adapted from "Beej's Guide to Network Programming"
    // (See sendall() function above for more information
    if (sendall(socket, buf, &buf_len) == -1)
      error("ERROR writing to socket", 1);

  }
  else
	error("ERROR no acknowledgment from server",1);

  return 0;
}

/************************************************
 * NAME
 *   read_socket
 * DESCRIPTION
 *   This program takes as an argument an int
 *   representing a file descriptor for a socket
 *   to which a connection has been established.
 *   It then reads data from the socket (sent by
 *   the counterpart client program) [TX_LIMIT]
 *   bytes at a time until it encounters the
 *   terminating character "EOT". The read string
 *   is then returned.
 * *********************************************/
char *read_socket(int socket)
{
  int charsRead, charsWritten;
  char *text = NULL;
  int cap;
  int len = 0;
  bool stop = false;
  char buffer[TX_LIMIT];
  int cur_idx = 0;

  // clear buffer
  memset(buffer, 0, sizeof(buffer));

  // initialize text
  text = malloc(1);
  text[0] = '\0';

  while (stop == false)
  {
    // read up to TX_LIMIT
    charsRead = recv(socket, buffer, sizeof(buffer), 0);
    if (charsRead < 0)
      error("CLIENT(enc): ERROR reading from socket", 1);

    else if (charsRead != 0)
    {
      // update the string length
      len += charsRead;

      // allocate or reallocate the plaintext string
      cap = (len + 1) * sizeof(char);
      text = realloc(text, cap);
      if (text == NULL)
        error("CLIENT(enc): Could not allocate memory", 1);

      // copy received text to text string
      int i;
      for (i = 0; i < TX_LIMIT; i++)
      {
        text[cur_idx] = buffer[i];

        if (text[cur_idx] == 4)
        {
          stop = true;
          text[cur_idx] = '\0';
        }

        cur_idx++;
      }

      text[len] = '\0';
    }

    // clear buffer for next read
    memset(buffer, 0, sizeof(buffer));
  }

  return text;
}

/************************************************
 * NAME
 *   write_socket
 * DESCRIPTION
 *   This function takes as an argument an int
 *   representing a socket with which a connection
 *   has been established and a string and writes
 *   the string to the socket [TX_LIMIT] bytes
 *   at a time until the entire string was sent.
 * *********************************************/
void *write_socket(int socket, const char *text)
{
  int charsWritten, cap, chunk = 0, totalWritten = 0;
  char term[2];
  term[0] = 4;
  term[1] = '\0';
  char *out_str = NULL;
  char buffer[TX_LIMIT];
  int cur_idx = 0;

  cap = strlen(text) + 2;

  out_str = malloc(cap * sizeof(char));
  if (out_str == NULL)
    error("Could not allocate memory", 1);
  memset(out_str, 0, cap * sizeof(char));

  // build the output string by appending term char to text
  strcpy(out_str, text);
  strcat(out_str, term);

  while (totalWritten < cap)
  {
    // clear buffer
    memset(buffer, 0, TX_LIMIT * sizeof(char));

    // fill the buffer with the next set of chars
    int i;
    for (i = 0; i < TX_LIMIT; i++)
    {
      if (cur_idx < strlen(out_str))
        buffer[i] = out_str[cur_idx];
      else
        buffer[i] = '\0';

      cur_idx++;
    }

    // send buffer
    charsWritten = send(socket, buffer, sizeof(buffer), 0);
    if (charsWritten < 0)
      error("CLIENT(enc): ERROR writing to socket", 1);

    totalWritten += charsWritten;
  }

  free(out_str);
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
  char buffer[BUFFERMAX+1];
  char* message;
  char close_cmd[] = "\\quit";
  bool quit = false;
  int len;

  buffer[0] = '\0';

  get_handle(handle);

  //printf("Handle is: %s\n", handle);
  //printf("Hostname is: %s\n", hostname);
  //printf("Port is: %d\n", port);

  /* The following code was adapted from CS344, Lecture 4.2, slide 21
   * "client.c"
   */
  int socketFD, charsWritten, charsRead;

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

  while (quit == false)
  {
	get_input(handle, buffer);

	if (strcmp(buffer, close_cmd) == 0)
	  quit = true;

	else
	{
	  memset(message, '\0', sizeof(message));
      strcat(message, handle);
      strcat(message, "> ");
      strcat(message, buffer);

      write_socket(socketFD, message);

      //get return message from server
      memset(message, '\0', sizeof(message));
      message = read_socket(socketFD);

      printf("%s\n", message);

	}
  }

  close(socketFD);

  return EXIT_SUCCESS;
}
