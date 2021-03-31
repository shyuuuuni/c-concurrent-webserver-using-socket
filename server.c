/**
 *  @file   Concurrent-Web-Server.c
 *  @brief  he web server that parses the HTTP request from the browser, 
 *          creates an HTTP response message consisting of the requested file
 *          preceded by header lines, then sends the response
 *          directly to the client
 *  @author Seunghyun Kim
 */
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
/**
 * sys/types.h:   definitions of a number of data types
 *                used in socket.h and netinet/in.h
 * sys/socket.h:  definitions of structures needed for sockets
 * netinet/in.h:  constants and structures needed for internet domain addresses
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SUCCESS_RESULT 0
#define FAILURE_RESULT -1
#define MIN_PORT 0
#define MAX_PORT 65535
#define BUFFER_SIZE 1024

/* for MAC OS execution */
#define _XOPEN_SOURCE  500
#include <unistd.h>

void error(char *msg);
int SetupServerSocket(void);
int GetPortNumber(int argc, char *argv[]);
int ListenRequest(int client_socket, char *buffer);
int ResponseHeader(int client_socket);
int HtmlResponse(int client_socket, char* buffer, char* html_file);

/**
 *  @brief This is the main function of Concurrent-Web-Server
 *  @param argc The number of arguments inputed to main function
 *  @param argv The arguments. argv[0]: execute command, argv[1]: port-number
 *  @return Execution success status
 */
int main(int argc, char *argv[])
{
  int server_socket, /** Descriptors return from socket()*/
      client_socket,  /** Descriptors return from accept()*/
      portno, /** Server port number*/
      nbytes; /** Bytes read or write*/

  socklen_t client_address_length;  /** Length of lient-socket address */
  
  char buffer[BUFFER_SIZE];

  char  *response_header,
        *response_content;

  /** 
   *  Structure containing an Internet address
   *  struct sockaddr_in:
   *  1) sin_family: Protocol family
   *  2) sin_port:   16bits port number
   *  3) sin_addr:   32bits Host IP address
   *  4) sin_zero:   Dummy data (fill with 0)
  */
  struct sockaddr_in serv_addr, cli_addr;
  
  portno = GetPortNumber(argc, argv);
  
  /**
   *  Create a server socket
   *  socket(int domain, int type, int protocol) function:
   *  1) domain = AF_INET: Protocol family is IPv4
   *  2) type   = SOCK_STREAM: Protocol type is TCP/IP
   *  3) socket function returns -1 if failed to open socket
   */ 
  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) { /* Failed to create socket*/
    error("ERROR during opening server socket.");
  }

  memset(&serv_addr, 0, sizeof(serv_addr)); /* Fill serv_addr with 0*/

  /* INADDR_ANY: Bind socket to all available interfaces*/
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_family = AF_INET; /* AF_INET: Protocol type is TCP/IP*/
  serv_addr.sin_port = htons(portno); /* Convert host to network byte order*/
  
  /**
   *  Bind the socket to the server address
   *  bind function returns -1 if failed to bind address
   */
  if (bind(server_socket,
          (struct sockaddr *) &serv_addr,
          sizeof(serv_addr)) < 0) { /* Failed to bind socket*/
    error("ERROR during binding the server socket.");
  }
  printf("SUCCESS binding the server socket.\n");
  
  /* Listen for socket connections. Backlog queue (connections to wait) is 5*/
  listen(server_socket,5);
  
  client_address_length = sizeof(cli_addr);
  /**
   *  Accept connect request
   *  1)  Block until a new connection is established
   *  2)  The new socket descriptor will be used for subsequent communication
   *      with the newly connected client.
   *  accept function returns -1 if failed to accept
  */
  client_socket = accept(server_socket,
                        (struct sockaddr *) &cli_addr,
                        &client_address_length);
  if (client_socket < 0) {  /* Failed to accept client*/
    error("ERROR during accept client socket.");
  }
  
  /* Get request from the client*/
  ListenRequest(client_socket, buffer);

  /* Send response to client*/
  ResponseHeader(client_socket);
  HtmlResponse(client_socket, buffer, "index.html");

  /* Stop socket connection*/
  close(server_socket);
  printf("SUCCESS closing the server socket.\n");
  close(client_socket);
  printf("SUCCESS closing the client socket.\n");
  
  printf("SUCCESS stop the web server.\n");
  return SUCCESS_RESULT; 
}

/**
 *  @brief  This is error handler function.
 *          Print the error message and exit process.
 *  @param msg The string of the error message
 *  @return  Return nothing
 */
void error(char *msg)
{
  perror(msg);
  exit(1);
}

/**
 *  @brief  This is a function that checks arguments of the main function.
 *  @param  argc  The number of arguments.
 *  @param  argv  The arguments.
 *  @return Return port number if argument is valid.
 */
int GetPortNumber(int argc, char* argv[]) {
  int port;

  /**
   *  Verify that the argument is a valid input
   *  1)  Check the arguments contain port number
   *  2)  The port number is valid in the range 0~65535,
   *      but the well-known port range 0~1023 can't be used
   */
  if (argc < 2) { /* The arguments does not contain port number*/
    fprintf(stderr, "ERROR during starting server. Check the port number.\n");
    exit(1);
  } else {
    port = atoi(argv[1]); /* convert string to integer*/

    if (MIN_PORT <= port && port < 1024) { /* Well-known port*/
      fprintf(stderr, 
              "WARNING, %d is in well-known port range.",
              port);
      exit(1);
    } else if (MAX_PORT < port || port < MIN_PORT) { /* Out of range*/
      fprintf(stderr, "ERROR, %d is unexpected port number.", port);
      exit(1);
    }

  printf("Waiting for client request at port %d\n", port);
  return port;
}

/**
 *  @brief  This is HTTP listener function.
 *  @param  client_socket Request from the client socket.
 *  @param  buffer  The buffer to read from the html file.
 *  @return Return 0 if successful.
 */
int ListenRequest(int client_socket, char *buffer) {
  int request_bytes;

  memset(buffer,0x00,BUFFER_SIZE); /* Clear buffer*/
  
  /* Read request from the client socket.*/
  request_bytes = read(client_socket, buffer, BUFFER_SIZE);

  if (request_bytes < 0) { /* Failed to read*/
    error("ERROR during reading request from client");
  }

  printf("SUCCESS reading request from client.\n");
  return SUCCESS_RESULT;
}

/**
 *  @brief  This responses the http header function.
 *  @param  client_socket Request from the client socket.
 *  @return Return 0 if successful.
 */ 
int ResponseHeader(int client_socket) {
  char *response_header;
  int response_header_size,
      header_bytes;

  response_header /* Here is the response header*/
  = "HTTP/1.0 200 OK\n"
    "Content-type: text/html\n"
    "\n";

  response_header_size = strlen(response_header);
  
  /* Send response header message to client*/
  header_bytes = write(client_socket, response_header, response_header_size);
  if (header_bytes < 0) { /* Failed to write*/
    error("ERROR during sending header to client");
  }

  printf("SUCCESS sending response header to client.\n");
  return SUCCESS_RESULT;
}

/**
 *  @brief  This is HTML responser function.
 *          Read the file and response to the client.
 *  @param  client_socket Request from the client socket.
 *  @param  buffer  The buffer to read from the html file.
 *  @param  html_file The request file name.
 *  @return Return bytes of the response message.
 */
int HtmlResponse(int client_socket, char* buffer, char* html_file) {
  FILE *pFile = fopen(html_file, "r"); /* Response file pointer*/
  int byte_sum = 0, /** Total response bytes*/
      data_bytes = 0,
      buffer_length;

  while(feof(pFile) == 0) {
    memset(buffer,0x00,BUFFER_SIZE);
    fgets(buffer,BUFFER_SIZE,pFile);
    
    buffer_length = strlen(buffer);
    data_bytes = write(client_socket, buffer, buffer_length);

    if (data_bytes < 0) { /* Failed to write*/
      error("ERROR during sending data to client.");
    } else {
      byte_sum += data_bytes;
    }
  }

  printf("SUCCESS sending response data to client.\n");
  return byte_sum;
}

