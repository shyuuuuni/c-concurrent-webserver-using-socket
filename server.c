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
#include <sys/uio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
/* Success and error value*/
#define SUCCESS_RESULT 0
#define FAILURE_RESULT -1

/* Port number range*/
#define MIN_PORT 0
#define MAX_PORT 65535

/* Limit number*/
#define MAX_LINE 255
#define BUFFER_SIZE 4096

/** Index of MINE types*/
#define File_t int
#define UNKNOWN_FILE -1
#define NO_FILE 0
#define HTML_FILE 1
#define GIF_FILE 2
#define JPEG_FILE 3
#define MP3_FILE 4
#define PDF_FILE 5

/** MINE types' content-type. {type)/{subtype}*/
char* Content_Types[] =  {"",
                          "text/html",
                          "image/gif",
                          "image/jpeg",
                          "audio/mpeg", /* mp3 or other MPEG media*/
                          "application/pdf",};

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

void error(char *msg);
int GetPortNumber(int argc, char *argv[]);
int SetupServerSocket(int portno);
int ListenRequest(int client_socket, char *buffer);
int ParseHTTPRequest(http_request_line* req_header_line, http_message request_body[],
                    char *buffer);
int BuildResponse(int client_socket, http_request_line* req_header_line,
                  http_message request_body[], char *buffer);
int ResponseHeader(int client_socket, char* http_version, int code, File_t filetype, char* filesrc);
ssize_t ResponseBody(int client_socket, char* buffer, char* filesrc, File_t filetype);
ssize_t SendResponse(int client_socket, char* buffer, char* file_name, int is_text);

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

  File_t file_type;
  char *file_name, *file;
  char  filesrc[BUFFER_SIZE],
        input_buffer[BUFFER_SIZE],
        output_buffer[BUFFER_SIZE];

  client_address_length = sizeof(cli_addr);

  /* Server start*/
  portno = GetPortNumber(argc, argv); /* Check arguments and return port number*/
  
  server_socket
  = SetupServerSocket(portno);  /* make server socket with port number*/
  
  /* Listen for socket connections. Backlog queue (connections to wait) is 5*/
  listen(server_socket,5);
  printf("\n[+] SUCCESS start server_socket.\n");

  while(1) {
    /* Get request from the client*/
    client_socket = accept(server_socket,
                        (struct sockaddr *) &cli_addr,
                        &client_address_length);
    if (client_socket <= 0) {  /* Failed to accept client*/
      error("[-] ERROR during accept client socket.");
    }
    

    memset(input_buffer, 0x00, BUFFER_SIZE);
    memset(output_buffer, 0x00, BUFFER_SIZE);

    /* Get request from the client*/
    if (ListenRequest(client_socket, input_buffer) == 0) {
      continue;
    } else {
      printf("[+] SUCCESS reading request from client.\n");
    }

    printf("*******************new******************\n");
    
    /* Parse request message to variables*/
    request_body_line = ParseHTTPRequest(&req_header_line, request_body, input_buffer);
    if (request_body_line < 0) {
      error("[-] ERROR request_body_line is invalid.");
    } else {
      printf("[+] SUCCESS getting request body lines.\n");
    }

    /* Build response by request and send the response message*/
    if (BuildResponse(client_socket, &req_header_line, request_body, output_buffer)
          != SUCCESS_RESULT) {
      error("[-] ERROR during building response."); /* Fail response*/
    } else {
      printf("[+] SUCCESS finishing the connection...\n");  /* Success response*/
    }

    close(client_socket);  /* Finish client socket*/
    printf("[+] SUCCESS closing the client socket.\n");
  }

  close(server_socket);  /* Finish server socket*/
  printf("[+] SUCCESS closing the server socket.\n");
  
  printf("[+] SUCCESS stop the web server.\n");
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
  //  */
  if (argc < 2) { /* The arguments does not contain port number*/
    fprintf(stderr, "[-] ERROR during starting server. Check the port number.\n");
    exit(1);
  } else {
    port = atoi(argv[1]); /* convert string to integer*/

    if (MIN_PORT <= port && port < 1024) { /* Well-known port*/
      fprintf(stderr, 
              "WARNING, %d is in well-known port range.",
              port);
      exit(1);
    } else if (MAX_PORT < port || port < MIN_PORT) { /* Out of range*/
      fprintf(stderr, "[-] ERROR, %d is unexpected port number.", port);
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
    error("[-] ERROR during opening server socket.");
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
    error("[-] ERROR during binding the server socket.");
  }

  printf("[+] SUCCESS binding the server socket.\n");
  printf("****************************************\n");
  return server_socket;
}

/**
 *  @brief  This is HTTP listener function.
 *  @param  client_socket Request from the client socket.
 *  @param  buffer  The buffer to read from the http message.
 *  @return Return 0 if successful.
 */
int ListenRequest(int client_socket, char *buffer) {
  int request_bytes;
  memset(buffer,0x00,BUFFER_SIZE); /* Clear buffer*/
  
  /* Read request from the client socket.*/
  request_bytes = read(client_socket, buffer, BUFFER_SIZE -1);
  if (request_bytes < 0) { /* Failed to read*/
    error("[-] ERROR during reading request from client");
  }

  return request_bytes;
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

  printf("%s", buffer); /* Dump the HTTP Request (Part A)*/

  /* Seperate header and body*/
  token = strtok_r(buffer,"\n", &rest_buffer);
  if (token[strlen(token) - 1] == '\r') {
    token[strlen(token) - 1] = '\0';
  }

  /* Parse and save the request header line*/
  req_header_line->action = strtok(token," ");
  req_header_line->location = strtok(NULL," ");
  req_header_line->http_version = strtok(NULL," ");

  /* Parse and save the request header line*/
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
 *  @param  Request from the client socket.
 *  @param  req_header_line  The request header pointer.
 *  @param  request_body  The request body pointer. Use this data
 *                        if request message is needed.
 *  @param  buffer  Total request message from client.
 *  @return Returns number of request body lines.
 */
int BuildResponse(int client_socket, http_request_line* req_header_line,
                  http_message request_body[], char *buffer) {
  ssize_t request_body_bytes = 0;  /** Response message's bytes*/
  int code; /** Response status code*/
  char  *file_name, *file_extension;  /* {file_name}.{file_extension}*/
  char  filesrc[BUFFER_SIZE]; /* Full name of file. {file_name.file_extension}*/
  File_t filetype;  /** Request file type*/

  memset(filesrc, 0x00, BUFFER_SIZE);
  strcpy(filesrc, req_header_line->location + 1); /* Save original file source*/

  if (strcmp(req_header_line->location, "/") == 0) { /* Input is {IP}:{port}*/
    filetype = NO_FILE;
  } else if (strchr(filesrc, '.') == NULL || 
            filesrc[strlen(filesrc)-1] == '.' ||
            filesrc[0] == '.') {
    /* Input has no-type. "/{example}" or "/{example.}"*/
    file_name = filesrc;
    file_extension = NULL;
    filetype = UNKNOWN_FILE;
  } else {
    /* Divide file source into {file_name}.{file_extension}*/
    file_name = strtok_r(req_header_line->location, ".", &file_extension);
    
    /* Check request file type*/
    if (strcmp(file_extension, "html") == 0) {
      filetype = HTML_FILE;
    } else if (strcmp(file_extension, "gif") == 0) {
      filetype = GIF_FILE;
    } else if (strcmp(file_extension, "jpeg") == 0) {
      filetype = JPEG_FILE;
    } else if (strcmp(file_extension, "mp3") == 0) {
      filetype = MP3_FILE;
    } else if (strcmp(file_extension, "pdf") == 0) {
      filetype = PDF_FILE;
    } else {
      filetype = UNKNOWN_FILE;
    }
  }

  if (strcmp(req_header_line->action, "GET") == 0) {
    /* GET method inputed*/
    /* Set status code by request file*/
    if (filetype == NO_FILE) {
      /* Route to 'index.html', 301 Moved Permanetly*/
      printf("[*] RESPONSE \"/\" here.\n");
      code = 200;
      strcpy(filesrc, "src/index.html");
      filetype = HTML_FILE;
      // error("[-] ERROR GET / failed");
    } else if (access(filesrc, F_OK) != -1) {
      /* Exist the request file, 200 OK*/
      printf("[*] RESPONSE \"%s\" exists\n", filesrc);
      code = 200;
    } else {
      /* 404 Not Found*/
      printf("[*] RESPONSE \"%s\" does not exists\n", filesrc);
      code = 404;
      filetype = HTML_FILE;
      strcpy(filesrc, "src/404.html");
    }

    /* Send response message*/
    if (ResponseHeader(client_socket, req_header_line->http_version, code, filetype, filesrc)
        != SUCCESS_RESULT) {
          error("[-] ERROR during sending response header.");
        }
    request_body_bytes = ResponseBody(client_socket, buffer, filesrc, filetype);
    printf("[*] RESPONSE body:: %zd bytes\n", request_body_bytes);
  } else if (strcmp(req_header_line->action, "POST") == 0) {
    /* POST method inputed*/
  } else {  /* 400 Bad Request*/
    error("[-] ERROR request header is invalid action.");
  }

  return SUCCESS_RESULT;
}

/**
 *  @brief  This responses the http header function.
 *  @param  client_socket Request from the client socket.
 *  @param  http_version  Request HTTP version.
 *  @param  code  statue code number.
 *  @param  filetype  Index of MINE types.
 *  @param  filesrc  The request file name.
 *  @return Return 0 if successful.
 */ 
int ResponseHeader(int client_socket, char* http_version, int code, File_t filetype, char* filesrc) {
  char  response_header[BUFFER_SIZE]; /** Buffer to save response header.*/
  char  *status,
        *type_comment;
  int header_bytes, /** Writen file's bytes*/
      message_size = 0, /** Number of HTTP header messages*/
      i;
  http_message messages[MAX_LINE];  /** Buffer's array to save response headers.*/

  /* Set status message by status code.*/
  if (code == 200) {
    status = "OK";
  } else if (code == 301) {
    status = "Moved Permanently";
  } else if (code == 400) {
    status = "Bad Request";
  } else if (code == 404) {
    status = "Not Found";
  }

  if (0 < filetype && filetype < 6) { /* Known file type*/
    messages[message_size].field = "Content-Type";
    messages[message_size++].data = Content_Types[filetype];
    
    messages[message_size].field = "Accept-Ranges";
    messages[message_size++].data = "bytes";
    if(filetype == PDF_FILE ||
      filetype == MP3_FILE) { /* Display PDF/MP3 file on browser*/
      char* content_message = malloc(sizeof(char) * 1024);
      sprintf(content_message, "inline; filename=\"%s\"", filesrc);
      messages[message_size].field = "Content-Disposition";
      messages[message_size++].data = content_message;
    }
  } else {  /* Unknown file type*/
    type_comment = "text/plain";  /* Display file as text*/
  }

  /* Send reponse header to client and print response header.*/
  /* Process header line*/
  sprintf(response_header, "%s %d %s\n", http_version, code, status);
  header_bytes = write(client_socket, response_header, strlen(response_header));
  if (header_bytes < 0) { /* Failed to write*/
    error("[-] ERROR during sending header_line to client");
  }
  
  /* Process header messages*/
  printf("[*] RESPONSE server response:\n%s", response_header);
  for(i = 0; i < message_size; i++) {
    sprintf(response_header, "%s: %s\n", messages[i].field, messages[i].data);
    header_bytes = write(client_socket, response_header, strlen(response_header));
    printf("%s: %s\n", messages[i].field, messages[i].data);
    if (header_bytes < 0) { /* Failed to write*/
      error("[-] ERROR during sending header_field to client");
    }
  }
  write(client_socket, "\n", 1); /* End of header line*/

  printf("[+] SUCCESS sending response header to client.\n");
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
ssize_t ResponseBody(int client_socket, char* buffer, char* filesrc,
                    File_t filetype) {
  ssize_t response_bytes = 0;
  printf("Request {%s} by method #{%d}\n", filesrc, filetype);

  /* Routing*/
  if (filetype == HTML_FILE || filetype == UNKNOWN_FILE) {
    response_bytes = SendResponse(client_socket, buffer, filesrc, 1);
  } else if (GIF_FILE <= filetype && filetype <= PDF_FILE) {
    response_bytes = SendResponse(client_socket, buffer, filesrc, 0);
  } else {
    error("[-] ERROR routing error.");
  }

  printf("[+] SUCCESS sending response body to client.\n");
  return response_bytes;
}

/**
 *  @brief  This is HTTP response function.
 *          Read the file and response to the client.
 *  @param  client_socket Request from the client socket.
 *  @param  buffer  The buffer to write response body.
 *  @param  file_name The request file name.
 *  @param  is_text  1 if MINE is text type. 0 is non-text type.
 *  @return Return bytes of the response message.
 */
ssize_t SendResponse(int client_socket, char* buffer, char* file_name, int is_text) {
  FILE *target_file;  /** Request file's pointer*/
  ssize_t byte_sum = 0, /** Total response bytes*/
          data_bytes = 0; /** Bytes returned by write()*/
  size_t  buffer_length,
          file_size,  /** Total file size*/
          sended_size = 0;

  memset(buffer,0x00,BUFFER_SIZE);

  if (is_text) {
    target_file = fopen(file_name, "r"); /* Read as text*/

    /* Read each lines and send piece of message to client*/
    while(feof(target_file) == 0) {
      fgets(buffer,BUFFER_SIZE - 2,target_file);
      
      // buffer_length = strlen(buffer);
      data_bytes = write(client_socket, buffer, strlen(buffer));
      if (data_bytes < 0) { /* Failed to write*/
        error("[-] ERROR during sending data to client.");
      } else {
        byte_sum += data_bytes;
      }
    }
  } else {
    size_t read_size;
    target_file = fopen(file_name, "rb"); /* Read as binary*/
    fseek(target_file, 0, SEEK_END);
    file_size = ftell(target_file);  /* Get file size*/
    fseek(target_file, 0, SEEK_SET);
    printf("%s: Total %zu bytes\n", file_name, file_size);

    /* File send progress: sended_size/file_size(%) */
    while (sended_size != file_size) {
      read_size = fread(buffer, 1, BUFFER_SIZE, target_file); /* read file*/
      if(read_size < 0) { /* Failed to read file.*/
        error("[-] ERROR during reading file as binary.");
      } else {
        sended_size += read_size;
      }
      
      data_bytes = write(client_socket, buffer, read_size); /* send file*/
      if (data_bytes < 0) { /* Failed to write*/
        error("[-] ERROR during sending data to client.");
      } else {
        byte_sum += data_bytes;
      }
    }
  }

  fclose(target_file);
  printf("[+] SendResponse input file_name: %s, %zd Bytes\n", file_name, byte_sum);

  printf("[+] SUCCESS sending HTML response data to client.\n");
  return byte_sum;
}

