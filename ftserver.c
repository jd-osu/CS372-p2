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
#include <errno.h>

// length of string buffers
#define BUFFER 1000 
#define FILENAME 200
#define CMD 3
#define CLIENTADDRESS 50
#define CLIENTNAME 100

#define SERVERSOURCE "ftserver.c"
#define SERVEREXEC "ftserver"
#define CLIENTSOURCE "ftclient.py"

static const char ack[] = "OK";
static const char list[] = "-l";
static const char get[] = "-g";

// bool type defined as true/false logic is used extensively
typedef enum {false, true} bool;

// data pertaining to the connection(s) for each client-server session
struct Conn
{
  int server_port;
  int data_port;
  char client_address[CLIENTADDRESS];
  char client_name[CLIENTNAME];
  char cmd[CMD];
  char filename[FILENAME];
  char msg_buffer[BUFFER];
  int control_socket;
  int control_conn;
  int data_socket;
};

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
  char file_nf[] = "File not found!";
  FILE *file;
  char *text = NULL;
  int c;
  int cap, len;

  // open the file
  file = fopen(filename, "r");
  if (file == NULL)
  {
    if (errno == 2)
    {
      send_ctrl_msg(conn, file_nf);
      return text;
    }
    else {
      sprintf(error_text, "Could not open %s", filename);
      error(error_text, 1);
    }
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

/******************************************************
* NAME
*    establish_data_socket
* DESCRIPTION
****************************************************/
void establish_data_connection(struct Conn *conn)
{
  /* Configure the socket
   * The following code was adapted from CS344, Lecture 4.2, slide 21
   * "client.c"
   */
  struct sockaddr_in serverAddress;
  struct hostent* serverHostInfo;
  socklen_t sizeOfServerInfo;
  sizeOfServerInfo = sizeof(serverAddress);

  memset((char*)&serverAddress, '\0', sizeof(serverAddress));
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(conn->data_port);
  inet_pton(AF_INET, conn->client_address, &(serverAddress.sin_addr));
  //serverHostInfo = gethostbyaddr((struct sockaddr*)&serverAddress, sizeOfServerInfo, AF_INET);

  //if (serverHostInfo == NULL) error("ERROR, no such host",1);

  //memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length);

  conn->data_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (conn->data_socket < 0) error("ERROR opening socket", 1);

  printf("Establishing data connection with client at %s...\n", inet_ntoa(serverAddress.sin_addr));

  if (connect(conn->data_socket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
    error("ERROR connecting", 1);

  printf("Data connection established with %s\n", conn->client_address);
}

/******************************************************
* NAME
*    establish_control_socket
* DESCRIPTION
****************************************************/
void configure_control_socket(struct Conn *conn, int port)
{
  conn->server_port = port;
  
  /* Configure the control socket
   * The following code was adapted from CS344, Lecture 4.3, slide 16
   * "server.c"
   */
  struct sockaddr_in serverAddress;
  
  // set up address struct for server
  memset((char *)&serverAddress, '\0', sizeof(serverAddress));
  
  //create socket
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(conn->server_port);
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  
  //set up socket
  conn->control_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (conn->control_socket < 0) error("ERROR opening socket", 1);
  
  // Listen for connection
  if (bind(conn->control_socket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
    error("ERROR binding", 1);
  listen(conn->control_socket, 5);
  
  printf("Server open on %d\n", conn->server_port);
}

/******************************************************
* NAME
*    establish_control_socket
* DESCRIPTION
****************************************************/
void establish_control_connection(struct Conn *conn)
{
  /* Accept and establish a connection with client
  * The following code was adapted from CS344, Lecture 4.3, slide 16
  * "server.c"
  */
  //accept connection
  socklen_t sizeOfClientInfo;
  struct sockaddr_in clientAddress;
  sizeOfClientInfo = sizeof(clientAddress);
  conn->control_conn = accept(conn->control_socket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
  if (conn->control_conn < 0) error("ERROR on accept", 1);
    
  // The following line of code (inet_ntoa in particular) adapted from:
  // "How to convert string to IP address and vice versa"
  // https://stackoverflow.com/questions/5328070/how-to-convert-string-to-ip-address-and-vice-versa
  inet_ntop(AF_INET, &(clientAddress.sin_addr),conn->client_address, CLIENTADDRESS);
  
  printf("Connection from %s\n", conn->client_address);
}

/******************************************************
* NAME
*    
* DESCRIPTION
****************************************************/
void read_control(struct Conn *conn)
{
  int charsRead;
  
  // get data port first
  memset(conn->msg_buffer, '\0', BUFFER * sizeof(conn->msg_buffer[0]));
  charsRead = recv(conn->control_conn, conn->msg_buffer, BUFFER-1, 0);
  if (charsRead < 0) error("ERROR reading from socket", 1);
}

/******************************************************
* NAME
*    
* DESCRIPTION
****************************************************/
void send_ctrl_msg(struct Conn *conn, const char *msg)
{
  int charsSent = send(conn->control_conn, msg, strlen(msg), 0);
  if (charsSent < 0) error("ERROR writing to socket", 1);
}

/******************************************************
* NAME
*    
* DESCRIPTION
****************************************************/
void clear_connection(struct Conn *conn)
{
  conn->data_port = 0;
  memset(conn->client_address, '\0', CLIENTADDRESS * sizeof(conn->client_address[0]));
  memset(conn->cmd, '\0', CMD * sizeof(conn->cmd[0]));
  memset(conn->filename, '\0', FILENAME * sizeof(conn->filename[0]));
  memset(conn->msg_buffer, '\0', BUFFER * sizeof(conn->msg_buffer[0]));
  conn->control_conn = 0;
  conn->data_socket = 0;
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
void send_directory(struct Conn *conn)
{
  printf("List directory requested on port %d.\n", conn->data_port);

  static const char server_src_txt[] = SERVERSOURCE;
  static const char server_exc_txt[] = SERVEREXEC;
  static const char client_src_txt[] = CLIENTSOURCE;
  char *text = NULL;
  int c;
  int cap, len;
  char buffer[BUFFER];
    
  // allocate memory for string and clear it
  cap = 100 * sizeof(char);
  text = malloc(cap);
  if (text == NULL)
    error("Could not allocate memory", 1);
  memset(text, '\0', 100);

  
  DIR *d;
  
  int i = 0;
    
  struct dirent *dir;
  d = opendir(".");
  if (d) {
    while ((dir = readdir(d)) != NULL) {
    
      //ignore source code files and directories
      if ((dir->d_type == DT_REG) &&
        (strcmp(dir->d_name, server_src_txt) != 0) &&
        (strcmp(dir->d_name, server_exc_txt) != 0) &&
        (strcmp(dir->d_name, client_src_txt) != 0) )
      {
    
        // check if enough memory is allocated and reallocate if necessary
        if ((i + strlen(dir->d_name)) >= (cap - 1))
        {
          cap = 100 * cap;
          text = realloc(text, cap);
          if (text == NULL)
            error("Could not allocate memory", 1);
        }
        
        int j;
        
        for (j=0; j < strlen(dir->d_name); j++)
        {
          text[i] = dir->d_name[j];
          i++;
        }
        
        text[i] = '\n';
        i++;
      }
    }
  
    text[i] = '\0';
    
    closedir(d);
  }
 
  int msg_len = strlen(text);
  int msg_sent;

  // send control message to client
  send_ctrl_msg(conn, list);
    
  //await ready message from client
  read_control(conn);
  
  // send the directory listing text to the client
  establish_data_connection(conn);
  
  printf("Sending directory contents to %s:%d\n", conn->client_address, conn->data_port);
  
  msg_sent = sendall(conn->data_socket, text, &msg_len);

  // if there was an error during sending
  if (msg_sent < 0)
    error("ERROR writing to socket", 1);
  
  close(conn->data_socket);

  free(text);
}

/******************************************************
* NAME
*    send_file
* DESCRIPTION
****************************************************/
void send_file(struct Conn *conn)
{
  char *contents;

  // read text from file
  contents = read_file(conn->filename);
  
  if (contents != NULL)
    free(contents);
}

/************************************************
 * NAME
 *  process_command
 * DESCRIPTION
 *  
 * *********************************************/
void process_command(struct Conn *conn)
{
  // global constant variables for commands

  static const char file_dir[] = "[file directory!]";
  static const char get_res[] = "[getting file!]";
  static const char invalid_cmd[] = "Invalid command!";

  
  // The following code for getting the command substring was adapted from:
  // "Get a substring of a char* [duplicate]"
  // https://stackoverflow.com/questions/4214314/get-a-substring-of-a-char
  memcpy(conn->cmd, &conn->msg_buffer[0], 2);
  conn->cmd[2] = '\0';
  
  if (strcmp(conn->cmd, list) == 0)
    send_directory(conn);
  else if (strcmp(conn->cmd, get) == 0)
  {
    if (strlen(conn->msg_buffer) > 3 && conn->msg_buffer[2] == ' ')
    {
      memcpy(conn->filename, &conn->msg_buffer[3], strlen(conn->msg_buffer)-3);
      conn->filename[strlen(conn->msg_buffer)-3] = '\0';
      
      send_file(conn);
    }
    else
      send_ctrl_msg(conn, invalid_cmd);
  }
  else
    send_ctrl_msg(conn, invalid_cmd);

}

int main(int argc, char **argv)
{
  // Make sure there is at least 1 command line argument
  if (argc < 2)
  {
    fprintf(stderr, "USAGE: %s [server port]\n", argv[0]);
    exit(1);
  }

  struct Conn conn;
    
  configure_control_socket(&conn, atoi(argv[1]));
  
  while(1)
  {
    establish_control_connection(&conn);
    
    // get data port first
    read_control(&conn);
    printf("1. I received this from the client: %s\n", conn.msg_buffer);
    
    conn.data_port = atoi(conn.msg_buffer);
    printf("Here is the data_port as an int: %d\n", conn.data_port);
      
    // send ACK "OK" msg in response
    send_ctrl_msg(&conn, ack);    
    
    // get command
    read_control(&conn);
    printf("2. I received this from the client: %s\n", conn.msg_buffer);
    
    // process command and send control response
    process_command(&conn);
    
    clear_connection(&conn);
  }
  
  return EXIT_SUCCESS;
  
  
  /*


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
