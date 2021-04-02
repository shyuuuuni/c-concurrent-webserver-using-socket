/**
 *  @file   Concurrent-Web-Server.c
 *  @brief  he web server that parses the HTTP request from the browser, 
 *          creates an HTTP response message consisting of the requested file
 *          preceded by header lines, then sends the response
 *          directly to the client
 *  @author Seunghyun Kim
 */

/**
 * sys/types.h:   definitions of a number of data types
 *                used in socket.h and netinet/in.h
 * sys/socket.h:  definitions of structures needed for sockets
 * netinet/in.h:  constants and structures needed for internet domain addresses
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SUCCESS_RESULT 0
#define FAILURE_RESULT -1

#define MIN_PORT 0
#define MAX_PORT 65535

#define MAX_LINE 255
#define BUFFER_SIZE 1024

#define File_t int
#define UNKNOWN_FILE -1
#define NO_FILE 0
#define HTML_FILE 1
#define GIF_FILE 2
#define JPEG_FILE 3
#define MP3_FILE 4
#define PDF_FILE 5

/**
 *  @brief  The http request header line message. 
 *          Struct contains "action" "location" "http_version" 
 *          (eg. GET /index.html HTTP/1.1)
 */
typedef struct http_request_line {
  char* action; /** request message*/
  char* location; /** request location*/
  char* http_version; /** HTTP version*/
} http_request_line;

/**
 *  @brief  The http response header line message. 
 *          Struct contains "http_version" "code" "status" 
 *          (eg. HTTP/1.1 200 OK)
 */
typedef struct http_response_line {
  char* http_version; /** HTTP version*/
  int code; /** response code*/
  char* status; /** response status*/
} http_response_line;

/**
 *  @brief  The http body message struct. 
 *          Struct contains "field":"data" 
 *          (eg. Host: localhost:10000)
 */
typedef struct http_message {
  char* field;  /** Field name*/
  char* data; /** Field value*/
} http_message;

/* for MAC OS execution */
#define _XOPEN_SOURCE  500
#include <unistd.h>

void error(char *msg);
int GetPortNumber(int argc, char *argv[]);
int SetupServerSocket(int portno);
int ListenRequest(int client_socket, char *buffer);
int ParseHTTPRequest(http_request_line* req_header_line, http_message request_body[],
                    char *buffer);
int BuildResponse(int client_socket, http_request_line* req_header_line,
                  http_message request_body[], char *buffer);
int ResponseHeader(int client_socket, char* http_version, int code);
int ResponseBody(int client_socket, char* buffer, char* filesrc, File_t filetype);
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
      request_body_line; /** Number of request body lines*/

  socklen_t client_address_length;  /** Length of client-socket address*/
  struct sockaddr_in cli_addr;  /** The client socket address*/

  http_request_line req_header_line;   /** The request header message*/
  http_message request_body[MAX_LINE]; /** The request body message*/

  File_t request_file;
  char *filename, *filetype;
  char  filesrc[BUFFER_SIZE],
        input_buffer[BUFFER_SIZE],
        output_buffer[BUFFER_SIZE];
  

  /* Server start*/
  portno = GetPortNumber(argc, argv); /* Check arguments and return port number*/
  
  server_socket
  = SetupServerSocket(portno);  /* make server socket with port number*/
  
  /* Listen for socket connections. Backlog queue (connections to wait) is 5*/
  listen(server_socket,5);

  client_address_length = sizeof(cli_addr);
  /*
   *  Accept connect request
   *  1)  Block until a new connection is established
   *  2)  The new socket descriptor will be used for subsequent communication
   *      with the newly connected client.
   *      accept function returns -1 if failed to accept
  */
  client_socket = accept(server_socket,
                        (struct sockaddr *) &cli_addr,
                        &client_address_length);
  if (client_socket < 0) {  /* Failed to accept client*/
    error("ERROR during accept client socket.");
  }
  
  /* Get request from the client*/
  ListenRequest(client_socket, input_buffer);

  /* Parse request message to variables*/
  request_body_line = ParseHTTPRequest(&req_header_line, request_body, input_buffer);
  if (request_body_line < 0) {
    error("ERROR request_body_line is invalid.");
  } else {
    printf("SUCCESS getting request body lines.\n");
  }

  /* Build response by request and send the response message*/
  if (BuildResponse(client_socket, &req_header_line, request_body, output_buffer)
        != SUCCESS_RESULT) {
          error("ERROR during building response."); /* Fail response*/
  } else {
    printf("SUCCESS finishing the connection...\n");  /* Success response*/
  }

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
void error(char *msg) {
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

  /*
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
  }

  printf("Waiting for client request at port %d\n", port);
  return port;
}

/**
 *  @brief  This makes binded server socket and returns that.
 *  @param portno  The port number
 *  @return Return server socket (integer)
 */ 
int SetupServerSocket(int portno) {
  int server_socket;

  /** 
   *  Structure containing an Internet address
   *  struct sockaddr_in has:
   *  1) sin_family: Protocol family
   *  2) sin_port:   16bits port number
   *  3) sin_addr:   32bits Host IP address
   *  4) sin_zero:   Dummy data (fill with 0)
  */
  struct sockaddr_in serv_addr;

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
  return server_socket;
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
 *  @brief  This function parses buffer to http-request-header and
 *          http-request-body.
 *  @param  req_header_line  The filled request header pointer.
 *  @param  request_body  The filled request body pointer.
 *  @param  buffer  Total request message from client.
 *  @return Returns number of request body lines.
 */
int ParseHTTPRequest(http_request_line* req_header_line,
                    http_message request_body[],
                    char *buffer) {
  int request_body_line = 0;  /* The number of request lines*/
  char  *token,
        *rest_buffer;

  /* The http request header message*/
  token = strtok_r(buffer,"\n", &rest_buffer);
  if (token[strlen(token) - 1] == '\r') {
    token[strlen(token) - 1] = '\0';
  }

  req_header_line->action = strtok(token," ");
  req_header_line->location = strtok(NULL," ");
  req_header_line->http_version = strtok(NULL," ");

  for(request_body_line = 0; token != NULL; request_body_line++) {
    token = strtok_r(rest_buffer, "\n", &rest_buffer);
    request_body[request_body_line].field = strtok(token,":");
    request_body[request_body_line].data = strtok(NULL,"\n");
  }

  return request_body_line - 1;
}

/**
 *  @brief  This function parses buffer to http-request-header and
 *          http-request-body.
 *  @param  req_header_line  The request header pointer.
 *  @param  request_body  The request body pointer. Use this data
 *                        if request message is needed.
 *  @param  buffer  Total request message from client.
 *  @return Returns number of request body lines.
 */
int BuildResponse(int client_socket, http_request_line* req_header_line, http_message request_body[],
                  char *buffer) {
  int bytes = 0,  /** Response message's bytes*/
      code; /** Response status code*/
  char  *filename, *filetype;
  char  filesrc[BUFFER_SIZE];
  char delimiters[] = {'.'};
  File_t request_file;  /** Request file type*/

  /* Save original file source*/
  strcpy(filesrc, req_header_line->location + 1);

  if (strcmp(req_header_line->location, "/") == 0) { /* Input is {IP}:{port}*/
    request_file = NO_FILE;
  } else if (strchr(req_header_line->location, '.') == NULL) { /* Has no type*/
    filename = filesrc;
    filetype = NULL;
    request_file = UNKNOWN_FILE;
  } else {
    /* Divide file source into {filename}.{filetype}*/
    filename = strtok_r(req_header_line->location, delimiters, &filetype);

    /* Ccheck request file type*/
    if (strcmp(filetype, "html") == 0) {
      request_file = HTML_FILE;
    } else if (strcmp(filetype, "gif") == 0) {
      request_file = GIF_FILE;
    } else if (strcmp(filetype, "jpeg") == 0) {
      request_file = JPEG_FILE;
    } else if (strcmp(filetype, "mp3") == 0) {
      request_file = MP3_FILE;
    } else if (strcmp(filetype, "pdf") == 0) {
      request_file = PDF_FILE;
    } else {
      request_file = UNKNOWN_FILE;
    }
  }

  if (strcmp(req_header_line->action, "GET") == 0) {
    /* GET method inputed*/

    /* Set status code by request file*/
    if (request_file == NO_FILE) {
      /* Route to 'index.html', 301 Moved Permanetly*/
      printf("RESPONSE route to \"/index.html\"\n");
      code = 301;
      request_file = HTML_FILE;
      strcpy(filesrc, "index.html");
    } else if (access(filesrc, F_OK) != -1) {
      /* Exist the request file, 200 OK*/
      printf("RESPONSE \"%s\" exists\n", filesrc);
      code = 200;
    } else {
      /* 404 Not Found*/
      printf("RESPONSE \"%s\" does not exists\n", filesrc);
      code = 404;
      request_file = HTML_FILE;
      strcpy(filesrc, "404-Not-Found.html");
    }

    /* Send response message*/
    if (ResponseHeader(client_socket, req_header_line->http_version, code)
        != SUCCESS_RESULT) {
          error("ERROR during sending response header.");
        }
    bytes = ResponseBody(client_socket, buffer, filesrc, request_file);
    printf("RESPONSE body:: %d bytes\n", bytes);
  } else if (strcmp(req_header_line->action, "POST") == 0) {
    /* POST method inputed*/
  } else {  /* 400 Bad Request*/
    error("ERROR request header is invalid action.");
  }

  return SUCCESS_RESULT;
}

/**
 *  @brief  This responses the http header function.
 *  @param  client_socket Request from the client socket.
 *  @return Return 0 if successful.
 */ 
int ResponseHeader(int client_socket, char* http_version, int code) {
  char response_header[BUFFER_SIZE];
  char *comment;
  int response_header_size,
      header_bytes;

  if (code == 200) {
    comment = "OK";
  } else if (code == 301) {
    comment = "Moved Permanently";
  } else if (code == 400) {
    comment = "Bad Request";
  } else if (code == 404) {
    comment = "Not Found";
  }

  sprintf(response_header, "%s %d %s\n\n", http_version, code, comment);
  printf("RESPONSE %s", response_header);
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
 *  @brief  This routes the function that sends response body by content type.
 *  @param  client_socket  Request from the client socket.
 *  @param  buffer  The buffer to write response body.
 *  @param  filesrc  The source of existing file.
 *  @param  filetype  The content type of the file.
 *  @return Return bytes of the response message.
 */
int ResponseBody(int client_socket, char* buffer, char* filesrc,
                    File_t filetype) {
  int response_bytes = 0;
  printf("Request {%s} by method {%d}\n", filesrc, filetype);

  /* Routing*/
  if (filetype == HTML_FILE) {
    response_bytes = HtmlResponse(client_socket, buffer, filesrc);
  }

  printf("SUCCESS sending response body to client.\n");
  return response_bytes;
}

/**
 *  @brief  This is HTML responser function.
 *          Read the file and response to the client.
 *  @param  client_socket Request from the client socket.
 *  @param  buffer  The buffer to write response body.
 *  @param  html_file The request file name.
 *  @return Return bytes of the response message.
 */
int HtmlResponse(int client_socket, char* buffer, char* html_file) {
  FILE *pFile = fopen(html_file, "r"); /* Response file pointer*/
  int byte_sum = 0, /** Total response bytes*/
      data_bytes = 0,
      buffer_length;

  printf("HtmlResponse input html_file: %s\n", html_file);

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

  printf("SUCCESS sending HTML response data to client.\n");
  return byte_sum;
}

