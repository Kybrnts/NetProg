/* HelloServer.c by Vivek Ramachandran modified by Me
 * Basic and simple "Hello World" Server
 * * Awaits for client connection
 * * Sends "Hello World" string back to client
 * This one can be compiled (without errors/warnings) with the more strict:
 * "gcc -xc -std=c89 -Wall -pedantic -o HelloServer2 HelloServer2.c"
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>  /* Required for socket usage                            */
#include <sys/socket.h>
#include <netinet/in.h> /* Required for structures                              */
#include <errno.h>      /* Required for perror()                                */
#include <string.h>     /* Required for bzero()                                 */
#include <arpa/inet.h>  /* Required for inet_ntoa() although deprecated         */
#include <unistd.h>     /* Required for close()                                 */

typedef struct sockaddr _sockaddr;
typedef struct sockaddr_in _sockaddr_in;

int main() {
  int sock = 0,                /* Server socket descriptor                      */
    cli = 0;                   /* Client's socket descriptor                    */
  _sockaddr_in *server = NULL, /* Server to listen                              */
    *client = NULL;            /* Client to be written when connection accepted */
  socklen_t len = 0;           /* Length of sockaddr_in structure               */
  char mesg[] = "Hello to the world of socket programming!\n"; /* Mesg. to send */
  int sent = 0;                /* Bytes sent to client counter                  */ 
  
  /* Take a TCP socket                                                          */
  if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) { /* Error check           */
    perror("Socket: ");
    return 1;
  }

  /* Allocate memory in heap for server's address structure                     */
  if((server = (_sockaddr_in *)malloc(sizeof(_sockaddr_in))) == NULL) {
    perror("Malloc: ");
    return 2;
  }
  
  /* Fill in server values                                                      */
  server->sin_family = AF_INET;                       /* As discussed in theory */
  server->sin_port = htons(10000);                    /* Use host byte order    */
  server->sin_addr.s_addr = INADDR_ANY;               /* By to any address      */
  bzero(server->sin_zero, 8);                         /* Complete padding       */

  len = sizeof(_sockaddr_in);                         /* Get size of structure  */
  /* Bind socket to port                                                        */
  if(bind(sock, (_sockaddr *)server, len) == -1) {    /* Error check            */
  /* Note that server is of sockaddr_in type, hence the casting; as sockaddr
   * is the superstructure containing it.
   */
    perror("bind");
    free(server); /* Release memory used for server                             */
    return 3;
  }
  /* Wait for clients (listen)                                                  */
  if(listen(sock, 5) == -1) { /* Error check (might not be needed               */
  /* 5 client back log queue requested                                         */
    perror("listen");
    free(server); /* Release memory used for server                             */
    return 4;
  }
  /* Once we start listening, we have to wait for the various clients to
   * connect. And once a client connects we start the processing.
   * In order to do that, lets use a perpetual loop.
   */
  while(1) {
  /* This is not the best way to do it, because server won't be able to exit
   * upon signal, but let's keep it like this to make it simple.
   */
    if((client = (_sockaddr_in *)malloc(sizeof(_sockaddr_in))) == NULL) {
      perror("Malloc: ");
      free(server); /* Release memory used for server                           */
      return 5;
    }    
    if((cli = accept(sock, (_sockaddr *)client, &len)) == -1) { /* Error check  */
    /* Accept() receives the server socket, to create (write) the client's.
     * All info is to be written to the casted to sockaddr client structure,
     * hence the pointer.
     * Also we need to get (write) the client socket structure length in len.
     */
      perror("accept");
      free(client); /* Release memory used for client                           */
      free(server); /* Release memory used for server                           */
      return 6;
    }
    /* Now client socket has been returned and we can talk to it                */
    sent = send(cli, mesg, strlen(mesg), 0); /* Send message to client          */
    printf("Sent %d bytes to client: %s\n", sent, inet_ntoa(client->sin_addr));
    /* Close connection                                                         */
    close(cli);
    free(client); /* Release memory used for client                             */
    printf("Client address memory released\n");
  }
  free(server);
  return 0;
}
