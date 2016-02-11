/*      A simple Echo Client
 *      ./client ip port
 *      Takes IP + Port form input
 *      Connects to the TCP server already listening on that IP + Port
 *      Sends to server various strings that user enters on stdin
 *      
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
#include <string.h>

#define ERROR -1      /* Error number                                           */
#define BUFFER 1024   /* Buffer size                                            */

main(int argc, char **argv) {
  struct sockaddr_in remote_server; /* Contains IP + port of remote server      */
  int sock;
  char input[BUFFER];  /* User input container                                  */
  char output[BUFFER]; /* What it is send from server                           */
  int len;

  /* In the most basic sense, a client consist of only 4 calls: 
   * 1. Socket, to establish the communication endpoint;
   * 2. Connect, to engage w/the remote server;
   * 3. Set of send/rcv calls to talk to it;
   */
  
  /* Create the communication endpoint (as usual) */
  if((sock = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) {
    perror("Socket");
    exit(-1);
  }

  /* Fill the address structure w/server information */
  remote_server.sin_family = AF_INET;
  remote_server.sin_port = htons(atoi(argv[2]));      /* First argument is IP   */
  remote_server.sin_addr.s_addr = inet_addr(argv[1]); /* Second arg is port#    */
  bzero(&remote_server.sin_zero, 8);

  /* Connect to server */
  if((connect(sock, (struct sockaddr *)&remote_server, sizeof(struct sockaddr_in))) == ERROR) {
  /* Connect accept same args as usual, except for the fact that the structure
   * points to remote server.
   * Check for success is also required
   */
    perror("Connect");
    exit(-1);
  }
  /* Communication loop                                                         */
  while(1) { /* Better signal handling required                                 */

    fgets(input, BUFFER, stdin); /* Issue input from user. Give smthng to send  */
    send(sock, input, strlen(input), 0); /* Send line as provided to server     */
    
    len = recv(sock, output, BUFFER, 0); /* Receive from server sent line       */
    output[len] = '\0';                  /* Nullify received to print safely    */
    printf("%s\n", output);              /* Show what we received               */
  }
  close(sock);
}
