/*      A simple Echo Server
 *      ./server port_no
 *      Takes a string on input, and send same string back to client.
 *      Listens on "port_no" upon start
 */
#include <stdio.h>      /* System calls required for syscalls                   */
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <error.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>

#define ERROR -1      /* Error number                                           */
#define MAX_CLIENTS 2 /* Tells kernel max clients to keep on wait queue         */
#define MAX_DATA 1024 /* Size of buffer to both send/recv                       */

int main(int argc, char **argv) {
  struct sockaddr_in server;                     /* Server socket address       */
  struct sockaddr_in client;                     /* Client socket address       */
  int sock;                                      /* Server socket descriptor    */
  int new;                                       /* Socket desc. for clients    */
  int sockaddr_len = sizeof(struct sockaddr_in); /* Size of socket addr structs */
  int data_len;                                  
  char data[MAX_DATA];

  /* Create a TCP socket (endpoint)                                             */
  if((sock = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) { /* Error check        */
    perror("Server socket: ");
    exit(-1);
  }

  /* Fill server address data structure                                         */
  server.sin_family = AF_INET;
  /* Server listen port number comes from argv string, which is converted to
   * big edian ascii and then to big edian short.
   */
  server.sin_port = htons(atoi(argv[1]));
  /* Instructs kernel to listen on all ifaces for the given port                */
  server.sin_addr.s_addr = INADDR_ANY; 
  bzero(&server.sin_zero, 8);

  /* Assign address to endpoint                                                 */
  if((bind(sock, (struct sockaddr *)&server, sockaddr_len)) == ERROR) { /* Chck */
    perror("Bind: ");
    exit(-1);
  }

  /* Get ready for at most MAX_CLIENT client connections */
  if((listen(sock, MAX_CLIENTS)) == ERROR) {
    perror("Listen");
    exit(-1);
  }
  
  /* Main loop in which we accept connections and start echoing                 */
  while(1) { /* Better signal handling required */
    if((new = accept(sock, (struct sockaddr *)&client, &sockaddr_len)) == ERROR) {
    /* To wait for connection we use accept() which is a blocking call. That
     * means that until a client connects to server, execution flow is stuck.
     * In addition accept() receives the client structure address to fill it
     * with the new client's IP/Port. The idea is that structure remains empty
     * until accept() returns.
     * Finally accept returns the client's socket descriptor to which write
     * whenever sending data to it is required.
     */
      perror("Accept: ");
      exit(-1);
    }
    /* Display clients information from client address structure. Note cnverts. */
    printf("New Client connected from port no %d and IP %s\n",
	   ntohs(client.sin_port),
	   inet_ntoa(client.sin_addr));
    data_len = 1;
    /* Keep in loop until client disconnects */
    while(data_len) {
      /* Receive data from client */
      data_len = recv(new, data, MAX_DATA, 0);
      /* As this is an echo server, allow client to talk first, reading from
       * socket. Also specify to store received string on "data", and the
       * maximum allowed. Last, recv returns how many bytes it has read from
       * socket, storing that in data_len.
       */
      if(data_len) {
      /* While we receive something from client, we send that back to it        */
	send(new, data, data_len, 0); /* This sends data to client w/no changes */
	data[data_len] = '\0';        /* Protect printf from hang by nullifying */
	printf("Sent mesg: %s", data); /* Show what we've sent                  */
      /* When client disconnects data_len = -1, hence loop will not go on       */
      }
    }
    printf("Client disconnected\n");      
    close(new);
    /* Now, go back to accept() state                                           */
  }
  return 0;
}
