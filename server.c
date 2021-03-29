/**
 *  @file    Concurrent-Web-Server.c
 *  @brief   The web server that parses the HTTP request from the browser, 
 *          creates an HTTP response message consisting of the requested file
 *          preceded by header lines, then sends the response
 *          directly to the client
 *  @author  Seunghyun Kim
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

#define MAX_PORT 65535

void error(char *msg);

/**
 *  @brief This is the main function of Concurrent-Web-Server
 *  @param argc The number of arguments inputed to main function
 *  @param argv The arguments. argv[0]: execute command, argv[1]: port-number
 *  @return Execution success status
 */
int main(int argc, char *argv[])
{
  /** Descriptors return from socket and accept system calls*/
  int sockfd, newsockfd,
      portno; /** port number */
  socklen_t clilen;
  
  char buffer[256];
  
  /*sockaddr_in: Structure Containing an Internet Address*/
  struct sockaddr_in serv_addr, cli_addr;
  
  int n;
  /**
   *  Verify that the argument is a valid input.
   *  1.  Check the arguments contain port number.
   *  2.  The port number is valid  in the range 0~65535,
   *      but the well-known port range 0~1023 can't be used.
   */
  if (argc < 2) {
    fprintf(stderr,"ERROR, no port provided\n");
    exit(1);
  } else {
    portno = atoi(argv[1]);
    if (0 <= portno && portno < 1024) { /* well known port except*/
      fprintf(stderr, 
              "WARNING, %d is in well-known port range", portno);
      exit(1);
    } else if (MAX_PORT < portno || portno < 0) { /* portno should in 0~65535*/
      fprintf(stderr, "ERROR, %d is not a valid port", portno);
      exit(1);
    } else {
      printf("valid port : %d\n", portno);
    }
  }
  
  /*Create a new socket
    AF_INET: Address Domain is Internet 
    SOCK_STREAM: Socket Type is STREAM Socket */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");
  
  bzero((char *) &serv_addr, sizeof(serv_addr));
  // portno = atoi(argv[1]); //atoi converts from String to Integer
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY; //for the server the IP address is always the address that the server is running on
  serv_addr.sin_port = htons(portno); //convert from host to network byte order
  
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) //Bind the socket to the server address
    error("ERROR on binding");
  
  listen(sockfd,5); // Listen for socket connections. Backlog queue (connections to wait) is 5
  
  clilen = sizeof(cli_addr);
  /*accept function: 
    1) Block until a new connection is established
    2) the new socket descriptor will be used for subsequent communication with the newly connected client.
  */
  newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
  if (newsockfd < 0) 
    error("ERROR on accept");
  
  bzero(buffer,256);
  n = read(newsockfd,buffer,255); //Read is a block function. It will read at most 255 bytes
  if (n < 0) error("ERROR reading from socket");
    printf("Here is the message: %s\n",buffer);
  
  n = write(newsockfd,"I got your message",18); //NOTE: write function returns the number of bytes actually sent out �> this might be less than the number you told it to send
  if (n < 0) error("ERROR writing to socket");
  
  close(sockfd);
  close(newsockfd);
  
  return 0; 
}

/**
 *  @brief This is error handler function.
 *        Print the error message and exit process.
 *  @param msg The String of the error message
 *  @return  Return nothing
 */
void error(char *msg)
{
  perror(msg);
  exit(1);
}