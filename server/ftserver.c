/************************************************
 * ftserver.c
 * Author: Jason DiMedio
 * CS372
 * November 25, 2018
 * This program is the server component of a FTP
 * application. The server takes as a command line
 * argument a port number to listen on. The server
 * accepts a connection from the client which serves
 * as a control connection for receiving commands.
 * A list directory command sends a directory listing
 * to the client, while a get command (which takes a
 * file name as an argument, causes the server to
 * establish a data connection through which it transfers
 * the file to the client.
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

// program file names for filtering from directory listing
static const char server_src_txt[] = "ftserver.c";
static const char server_exc_txt[] = "ftserver";
static const char client_src_txt[] = "ftclient.py";
static const char makefile_txt[] = "makefile";

// control commands
static const char ack[] = "OK";
static const char list[] = "-l";
static const char get[] = "-g";

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
 *   This function takes as an argument a string
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
    if (errno == 2)
      return text;
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
  memset(text, '\0', 100 * sizeof(text[0]));

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

  return text;
}

/******************************************************
* NAME
*    establish_data_socket
* DESCRIPTION
*    This function takes as an argument a Conn object
*    populated with command and data port information
*    and establishes a TCP connection and populates
*    the Conn object with a file descriptor for the
*    data connection socket.
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

  conn->data_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (conn->data_socket < 0) error("ERROR opening socket", 1);

  if (connect(conn->data_socket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
    error("ERROR connecting", 1);
}

/******************************************************
* NAME
*    configure_control_socket
* DESCRIPTION
*    This function takes as an input a Conn object and
*    a server port number and creates a socket for listening
*    at the port specified by the server port number. The
*    file descriptor for the socket is stored in the Conn
*    object.
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
*    establish_control_connection
* DESCRIPTION
*    This function takes as an input a Conn object
*    populated with a file descriptor for a socket
*    and stores the file descriptor for the connection
*    in the Conn object along with the client IP address.
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
*    read_control
* DESCRIPTION
*    This function takes as an argument a Conn object
*    populated with a file descriptor for a control
*    connection and reads a string from the control
*    connection. The string is stored in the Conn object.
****************************************************/
void read_control(struct Conn *conn)
{
  int charsRead;
  
  memset(conn->msg_buffer, '\0', BUFFER * sizeof(conn->msg_buffer[0]));
  charsRead = recv(conn->control_conn, conn->msg_buffer, BUFFER-1, 0);
  if (charsRead < 0) error("ERROR reading from socket", 1);
}

/******************************************************
* NAME
*    send_ctrl_msg
* DESCRIPTION
*    This function takes as an argument a Conn object 
*    populated with a file descriptor for a control
*    connection and sends the string through the control
*    connection.
****************************************************/
void send_ctrl_msg(struct Conn *conn, const char *msg)
{
  int charsSent = send(conn->control_conn, msg, strlen(msg), 0);
  if (charsSent < 0) error("ERROR writing to socket", 1);
}

/******************************************************
* NAME
*    clear_connection
* DESCRIPTION
*    This function takes as an argument a Conn object 
*    populated with data related to existing control
*    and data connections, which it then clears to
*    prepare the object to be used for another connection.
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
 *    send_directory
 * DESCRIPTION
 *    This function takes as an argument a Conn object
 *    populated with a file descriptor for a data connection.
 *    The contents of the present directory are then retrieved
 *    and directory names and any program files are removed
 *    from the list. A string representing the list is generated
 *    and sent to the client via the data connection.
 *
 *NOTE: The main part of this function was adapted from:
 *"How to list files in a directory in a C program?"
 * https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
 * *********************************************/
void send_directory(struct Conn *conn)
{
  printf("List directory requested on port %d.\n", conn->data_port);

  char *text = NULL;
  int c;
  int cap, len;
  char buffer[BUFFER];
    
  // allocate memory for string and clear it
  cap = 100 * sizeof(char);
  text = malloc(cap);
  if (text == NULL)
    error("Could not allocate memory", 1);
  memset(text, '\0', 100 * sizeof(text[0]));
  
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
        (strcmp(dir->d_name, server_exc_txt) != 0) &&
        (strcmp(dir->d_name, makefile_txt) != 0) )
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
  
    if (i != 0)
      // replace final newline with null character
      text[i-1] = '\0';
    
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
*    This function takes as an argument a Conn object
*    populated with a file descriptor for a data connection
*    along with a filename for a requested file.
*    The contents of the file are then retrieved
*    sent to the client via the data connection.
****************************************************/
void send_file(struct Conn *conn)
{
  char *contents;
  char size_str[100];
  int size;
  char file_nf[] = "File not found!";
  
  printf("File \"%s\" requested on port %d.\n", conn->filename, conn->data_port);

  // read text from file
  contents = read_file(conn->filename);
  
  if (contents == NULL)
  {
    send_ctrl_msg(conn, file_nf);
    return;
  }
  
  int msg_len = strlen(contents);
  int msg_sent;

  // send control message to client
  send_ctrl_msg(conn, get);
      
  //await ACK from client
  read_control(conn);
  
  // send filename to client
  send_ctrl_msg(conn, conn->filename);
  
  //await ACK from client
  read_control(conn);
  
  // send file size to client
  size = strlen(contents);
  sprintf(size_str, "%d", size);
  send_ctrl_msg(conn, size_str);
  
  //await ready message from client
  read_control(conn);
    
  // send the file contents to the client
  establish_data_connection(conn);
    
  printf("Sending \"%s\" to %s:%d\n", conn->filename, conn->client_address, conn->data_port);
    
  msg_sent = sendall(conn->data_socket, contents, &msg_len);

  // if there was an error during sending
  if (msg_sent < 0)
    error("ERROR writing to socket", 1);
    
  close(conn->data_socket);
  
  memset(contents, '\0', strlen(contents) * sizeof(contents[0]));    
  free(contents);
  
}

/************************************************
 * NAME
 *    process_command
 * DESCRIPTION
 *    This function takes as an argument a Conn object
 *    populated with a string received from the client.
 *    The string is parsed to extract command information
 *    and any arguments pertaining to the command, which
 *    are stored in the Conn object. Appropriate actions
 *    are then taken based on the specific commands.
 * *********************************************/
void process_command(struct Conn *conn)
{
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

  int port = atoi(argv[1]);

  // validate server port (range 1025-65535)
  if (port <= 1024 || port > 65535)
    error("INVALID INPUT: Port must be in range 1025-65535", 1);

  struct Conn conn;
    
  configure_control_socket(&conn, port);
  
  while(1)
  {
    establish_control_connection(&conn);
    
    // get data port and send ACK
    read_control(&conn);
    conn.data_port = atoi(conn.msg_buffer);
    send_ctrl_msg(&conn, ack);    
    
    // get command and process
    read_control(&conn);
    process_command(&conn);
    
    clear_connection(&conn);
  }
  
  return EXIT_SUCCESS;
  
}
