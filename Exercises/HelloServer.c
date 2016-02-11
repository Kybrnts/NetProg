/* HelloServer.c by Vivek Ramachandran
 * Basic and simple "Hello World" Server
 * * Awaits for client connection
 * * Sends "Hello World" string back to client
 * To compile this simply use "gcc HelloServer.c -o HelloServer". It shall
 * give no errors/warnings at all.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>  /* Required for socket usage                            */
#include <sys/socket.h>
#include <netinet/in.h> /* Required for structures                              */
#include <errno.h>      /* Required for perror()                                */
#include <string.h>     /* Required for bzero()                                 */

main() {
  int sock,                  /* Server socket descriptor                        */
      cli;                   /* Client's socket descriptor                      */
  struct sockaddr_in server, /* Server to listen                                */
                     client; /* Client to be written when connection accepted   */
  unsigned int len;          /* Length of sockaddr_in structure                 */
  char mesg[] = "Hello to the world of socket programming!"; /* Mesg. to send   */
  int sent;                  /* Bytes sent to client counter                    */ 
  
  /* Take a TCP socket                                                          */
  if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) { /* Error check */
    perror("socket: ");
    exit(-1);
  }

  /* Fill in server values                                                      */
  server.sin_family = AF_INET;                       /* As discussed in theory  */
  server.sin_port = htons(10000);                    /* Use network byte order  */
  server.sin_addr.s_addr = INADDR_ANY;               /* By to any address       */
  bzero(&server.sin_zero, 8);                        /* Complete padding        */

  len = sizeof(struct sockaddr_in);                  /* Get size of structure   */
  /* Bind socket to port                  */
  if(bind(sock, (struct sockaddr *)&server, len) == -1) { /* Error check        */
  /* Note that server is of sockaddr_in type, hence the casting; as sockaddr
   * is the superstructure containing it.
   */
    perror("bind");
    exit(-1);
  }
  /* Wait for clients (listen)                                                  */
  if(listen(sock, 5) == -1) { /* Error check (might not be needed               */
  /* 5 client back log queue requested                                          */
    perror("listen");
    exit(-1);
  }
  /* Once we start listening, we have to wait for the various clients to
   * connect. And once a client connects we start the processing.
   * In order to do that, lets use a perpetual loop.
   */
  while(1) {
  /* This is not the best way to do it, because server won't be able to exit
   * upon signal, but let's keep it like this to make it simple.
   */
    if((cli = accept(sock, (struct sockaddr *)&client, &len)) == -1) { /* Erchk */
    /* Accept() receives the server socket, to create (write) the client's.
     * All info is to be written to the casted to sockaddr client structure,
     * hence the pointer.
     * Also we need to get (write) the client socket structure length in len.
     */
      perror("accept");
      exit(-1);
    }
    /* Now client socket has been returned and we can talk to it                */
    sent = send(cli, mesg, strlen(mesg), 0); /* Send message to client          */
    printf("Sent %d bytes to client: %s\n", sent, inet_ntoa(client.sin_addr));
    /* Close connection                                                         */
    close(cli);
  }
}
