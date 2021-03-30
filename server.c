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

/* for MAC OS execution */
#define _XOPEN_SOURCE  500
#include <unistd.h>

#define MIN_PORT 0
#define MAX_PORT 65535
#define BUFSIZE 1024

void error(char *msg);

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
      nbytes, /** Bytes read or write*/
      response_length; /** Length of Response message*/

  socklen_t client_address_length;  /** Length of lient-socket address */
  
  char buffer[BUFSIZE]; /** Request buffer*/
  char* response; /* The response message*/
  
  /** 
   *  Structure containing an Internet address
   *  struct sockaddr_in:
   *  1) sin_family: Protocol family
   *  2) sin_port:   16bits port number
   *  3) sin_addr:   32bits Host IP address
   *  4) sin_zero:   Dummy data (fill with 0)
  */
  struct sockaddr_in serv_addr, cli_addr;

  /**
   *  Verify that the argument is a valid input
   *  1)  Check the arguments contain port number
   *  2)  The port number is valid  in the range 0~65535,
   *      but the well-known port range 0~1023 can't be used
   */
  if (argc < 2) { /* The arguments does not contain port number*/
    fprintf(stderr,"ERROR, no port provided\n");
    exit(1);
  } else {
    portno = atoi(argv[1]); /* convert string to integer*/

    if (MIN_PORT <= portno && portno < 1024) { /* Well-known port*/
      fprintf(stderr, 
              "WARNING, %d is in well-known port range",
              portno);
      exit(1);
    } else if (MAX_PORT < portno || portno < MIN_PORT) { /* Out of range*/
      fprintf(stderr, "ERROR, %d is not a valid port", portno);
      exit(1);
    } else {  /* Valid arguments*/
      printf("valid port : %d\n", portno);
    }
  }
  
  /**
   *  Create a server socket
   *  socket(int domain, int type, int protocol) function:
   *  1) domain = AF_INET: Protocol family is IPv4
   *  2) type   = SOCK_STREAM: Protocol type is TCP/IP
   *  3) socket function returns -1 if failed to open socket
   */ 
  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) { /* Failed to create socket*/
    error("ERROR opening socket");
  }

  /* Initialize the serv_addr*/
  bzero((char *) &serv_addr, sizeof(serv_addr)); /* Fill serv_addr with 0*/

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
    error("ERROR on binding");
  }
  
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
    error("ERROR on accept");
  }
  
  bzero(buffer, BUFSIZE); /* Clear buffer*/

  /**
   * Read is a block function. So, waiting for client's request
   * It will read at most BUFSIZE-1 bytes
   */
  nbytes = read(client_socket, buffer, BUFSIZE);
  if (nbytes < 0) { /* Failed to read*/
    error("ERROR reading from socket");
  }
    printf("Here is the message:\n%s\n",buffer);
  

  response = "Response message!";
  response_length = strlen(response);

  /* Send response message to client */
  nbytes = write(client_socket, response, response_length);
  if (nbytes < 0) { /* Failed to write */
    error("ERROR writing to socket");
  }
  
  /* Stop socket connection*/
  close(server_socket);
  close(client_socket);
  
  return 0; 
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