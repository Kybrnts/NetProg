/*
 * Nweb23.c
 * This is Kybrnts's modified version of Nigel Griffiths's Nweb.
 * This is the result of learn/play around w/the original code (as author itself recommends).
 * Several comments added for better understanding, hence the increased file length.
 * Also read()/write() calls upon sockets were replaced by the recommended send()/recv().
 * Future improvements may be required.
 * Compile w/gcc -xc -std=c89 -Wall -pedantic -o nweb23_${OS} nweb23.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define VERSION 23
#define BUFSIZE 8096
#define ERROR 42
#define LOG 44
#define FORBIDDEN 403
#define NOTFOUND 404

/* Define some shorthands (Avoid using "struct" for each cast                                       */
typedef unsigned short usint;
typedef struct sockaddr _sockaddr;
typedef struct sockaddr_in _sockaddr_in;

struct {                 /* Anon struc declaration w/two members, for single file type storage.     */
  char *ext;             /* String member for file extension;                                       */
  char *filetype;        /* String member for file type description.                                */
} extensions[] = {       /* Global stack array of available files. Simple list implementation.      */
  {"gif", "image/gif" },  
  {"jpg", "image/jpg" }, 
  {"jpeg","image/jpeg"},
  {"png", "image/png" },  
  {"ico", "image/ico" },  
  {"zip", "image/zip" },  
  {"gz",  "image/gz"  },  
  {"tar", "image/tar" },  
  {"htm", "text/html" },  
  {"html","text/html" },  
  {0,0} };               /* Null-char terminated strings for both members signals end of list       */
                         /* Remember: char *string = 0 is same as *string = "" or *string = '\0'    */

void logger(int type, char *s1, char *s2, int socket_fd) {
/* Log messages.. */
  int fd = 0;
  char logbuffer[BUFSIZE*2] = "";

  switch (type) {
  case ERROR:   /* Is this an error scenario?                                                       */
    /* Store a message in buffer, comprising arguments s1 and s2 messages.                          */
    /* The global variable "errno" contains the matching value of last syscall error on failure     */
    /* This value is as well included in the error message, together w/the PID of the parent proc.  */
    sprintf(logbuffer, "ERROR: %s:%s Errno= %d exiting pid= %d", s1, s2, errno, getpid());
    break;
  case FORBIDDEN: /*  Forbidden resource selected ?                                                 */
    /* Explain this to client by talking to it through the received file descriptor                 */
    /* Change: We replaced original write() with send(), as is the recommended way.                 */
    send(socket_fd,
	 "HTTP/1.1 403 Forbidden\nContent-Length: 185\n"
	 "Connection: close"
	 "Content-Type: text/html\n\n"
	 "<html><head>\n"
	 "<title>403 Forbidden</title>\n"
	 "</head><body>\n"
	 "<h1>Forbidden</h1>\n"
	 "The requested URL, file type or operation is not allowed on this simple static file webserver.\n"
	 "</body></html>\n",
	 271,
	 0);
    /* Also report error to message buffer, to later print it in log file (when possible)           */
    sprintf(logbuffer, "FORBIDDEN: %s:%s",s1, s2); 
    break;
  case NOTFOUND: /* Unavailable resource selected?                                                  */
    /* Proceed as with 403 forbidden                                                                */
    send(socket_fd,
	 "HTTP/1.1 404 Not Found\n"
	  "Content-Length: 136\nConnection: close\n"
	  "Content-Type: text/html\n\n"
	  "<html><head>\n"
	  "<title>404 Not Found</title>\n"
	  "</head><body>\n"
	  "<h1>Not Found</h1>\n"
	  "The requested URL was not found on this server.\n"
	  "</body></html>\n",
	 244,
	 0);
    /* And also with message buffer                                                                 */
    sprintf(logbuffer, "NOT FOUND: %s:%s", s1, s2); 
    break;
  case LOG: /* Default behaviour                                                                    */
    /* Simply put message in logfile                                                                */
    sprintf(logbuffer, " INFO: %s:%s:%d", s1, s2, socket_fd);
    break;
  }
  /* Try to print to log file the error message. File modes are selected via constants.             */
  /* No checks here, nothing can be done with a failure anyway                                      */
  if((fd = open("nweb.log", O_CREAT| O_WRONLY | O_APPEND, 0644)) >= 0) { /* Try to open file        */
    write(fd, logbuffer, strlen(logbuffer));  /* Try to write a line to file                        */
    write(fd, "\n", 1); /* Print line terminator character as last byte of the line                 */
    close(fd); /* Close file                                                                        */
  }
  /* If message type matches failure, then exit the entire process (Usually a client process)       */
  if(type == ERROR || type == NOTFOUND || type == FORBIDDEN)
    exit(3);
}

/* This is a child web server process, so we can exit on errors                                     */
void web(int fd, int hit) {
  int j = 0, file_fd = 0, buflen = 0;
  long i = 0, ret = 0, len = 0;
  char *fstr = NULL;
  static char buffer[BUFSIZE+1]; /* Static so zero filled                                           */


  /* Here is where the "dialog" w/client starts *************************************************** */
  ret = recv(fd, buffer, BUFSIZE, 0); /* Read Web request in one go                                 */
  if(ret == 0 || ret == -1) /* Read failure stop now                                                */
    logger(FORBIDDEN, "failed to read browser request", "", fd);
  if(ret > 0 && ret < BUFSIZE)/* Return code is valid chars                                         */
    buffer[ret] = 0; /* Terminate the buffer                                                        */
  else buffer[0] = 0;
  for(i = 0; i < ret; i++)/* Remove CF and LF characters                                            */
    if(buffer[i] == '\r' || buffer[i] == '\n')
      buffer[i]='*';
  logger(LOG, "request", buffer, hit);
  /* Check received string. We only support case insensitive gets                                   */
  if(strncmp(buffer, "GET ", 4) && strncmp(buffer, "get ", 4)) /* String validation check           */
    logger(FORBIDDEN, "Only simple GET operation supported", buffer, fd); /* Log/error & exit child */
  for(i = 4; i < BUFSIZE; i++) /* Null terminate after the second space to ignore extra stuff       */
    if(buffer[i] == ' ') { /* String is "GET URL " + lots of other stuff                            */
      buffer[i] = 0;
      break;
    }
  for(j = 0; j < i-1; j++) /* Check for illegal parent directory use ..                             */
    if(buffer[j] == '.' && buffer[j+1] == '.')
      logger(FORBIDDEN, "Parent directory (..) path names not supported", buffer, fd);
  /* Convert no file name to index file                                                             */
  if(!strncmp(&buffer[0], "GET /\0", 6) || !strncmp(&buffer[0], "get /\0", 6))
    strcpy(buffer, "GET /index.html");

  /* Work out the file type and check we support it                                                 */
  buflen = strlen(buffer);
  fstr = (char *)0;
  for(i = 0; extensions[i].ext != 0; i++) {
    len = strlen(extensions[i].ext);
    if(!strncmp(&buffer[buflen-len], extensions[i].ext, len)) {
      fstr =extensions[i].filetype;
      break;
    }
  }
  if(fstr == 0)
    logger(FORBIDDEN,"file extension type not supported",buffer,fd);

  if((file_fd = open(&buffer[5],O_RDONLY)) == -1) /* Open the file for reading                      */
    logger(NOTFOUND, "failed to open file",&buffer[5],fd);
  logger(LOG,"SEND",&buffer[5],hit);
  len = (long)lseek(file_fd, (off_t)0, SEEK_END); /* Lseek to the file end to find the length       */
  lseek(file_fd, (off_t)0, SEEK_SET); /* lseek back to the file start ready for reading             */
  /* Prepare an long log/buffer message                                                             */
  sprintf(buffer, 
	  "HTTP/1.1 200 OK\n"
	  "Server: nweb/%d.0\n"
	  "Content-Length: %ld\n"
	  "Connection: close\n"
	  "Content-Type: %s\n\n",
	  VERSION,
	  len,
	  fstr); /* Header + a blank line                                                           */
  logger(LOG, "Header", buffer, hit); /* Send buffer to log file                                    */
  send(fd, buffer, strlen(buffer), 0); /* Send buffer to client                                     */

  /* send file in 8KB block - last block may be smaller                                             */
  while ((ret = read(file_fd, buffer, BUFSIZE)) > 0)
    send(fd, buffer, ret, 0); /* Send single 8KB chunk to client                                    */
  sleep(1); /* Allow socket to drain before signalling the socket is closed                         */
  close(fd);
  exit(1); /* Nothing else to be send, hence exit child process                                     */
}

int main(int argc, char *argv[]) {
/* Main server process function.
 * This will accept two arguments from CMD line: port number, and web directory.
 */
  int i = 0,        /* Generic index storage                                                        */
    port = 0,       /* Local TCP port number to listen to                                           */
    pid = 0,        /*                                                                              */
    listenfd = 0,   /* Socket server file descriptor                                                */
    socketfd = 0,   /* Client socket file descriptor                                                */
    hit= 0;         /*                                                                              */
  socklen_t length; /* Socket structures length storage                                             */

  static _sockaddr_in serv_addr; /* Server-to-listen address. Static, so initialised to zeros       */
  static _sockaddr_in cli_addr;  /* Inbound client addresses. Static, so initialised to zeros       */

  /* Did we invoke server w/wrong number of arguments or with the help options? */
  if(argc < 3  || argc > 3 || !strcmp(argv[1], "-?")) {
    printf("hint: nweb Port-Number Top-Directory\t\tversion %d\n\n" /* Display help message         */
	   "\tNWeb is a small and very safe mini web server\n"
	   "\tNWeb only servers out file/web pages with extensions named below\n"
	   "\t and only from the named directory or its sub-directories.\n"
	   "\tThere is no fancy features = safe and secure.\n\n"
	   "\tExample: nweb 8181 /home/nwebdir &\n\n"
	   "\tOnly Supports:", VERSION);
    for(i = 0; extensions[i].ext != 0; i++) /* For all avail exts (stop when null char found in ext */
      printf(" %s", extensions[i].ext);     /* Display file extension                               */

    /* Additional info message                                                                      */
    printf("\n\tNot Supported: URLs including \"..\", Java, Javascript, CGI\n"
		 "\tNot Supported: directories / /etc /bin /lib /tmp /usr /dev /sbin\n"
		 "\tNo warranty given or implied\n"
	         "\tNigel Griffiths <nag@uk.ibm.com>\n"
	         "\tKybernetes <correodelkybernetes@gmail.com>\n");
    return 0;
  }
  /* Avoid using system directories as web                                                          */
  if(!strncmp(argv[2],"/"   ,2 ) || !strncmp(argv[2],"/etc", 5 ) || /* No / or /etc                 */
     !strncmp(argv[2],"/bin",5 ) || !strncmp(argv[2],"/lib", 5 ) || /* No /bin or /lib              */
     !strncmp(argv[2],"/tmp",5 ) || !strncmp(argv[2],"/usr", 5 ) || /* and so forth..               */
     !strncmp(argv[2],"/dev",5 ) || !strncmp(argv[2],"/sbin",6) ){
    printf("ERROR: Bad top directory %s, see nweb -?\n", argv[2]);
    return 3;
  }
  /* Try to change working directory to that of provided by second command line argument            */
  /* Remember: chdir is a Syscall that returns -1 on failure.                                       */
  if(chdir(argv[2]) == -1){ 
    printf("ERROR: Can't Change to directory %s\n", argv[2]);
    return 4;
  }
  
  /* Become deamon + unstoppable and no zombies children (= no wait())                              */
  if(fork() != 0)
  /* This splits execution in two. One process is parent 
   * Remember: 
   * If fork() returns a negative value, the creation of a child process was unsuccessful.
   * fork() returns a zero to the newly created child process.
   * fork() returns a positive value, the process ID of the child process, to the parent.
   */
    return 0; /* Parent returns OK to shell. This is never executed by child.                       */
  /* Since parent returned, all remaining code is excusively executed by child.******************** */
  /* Ignore subsequent child signals see "man signal()" for additional info.                        */
  signal(SIGCLD, SIG_IGN); /* Ignore child death                                                    */
  signal(SIGHUP, SIG_IGN); /* Ignore terminal hangups                                               */   
  for(i = 0; i < 32; i++)
    close(i); /* Close open files. (Why 32?)                                                        */
  setpgrp();  /* Break away from process group see "man setpgrp() for additional info.              */

  logger(LOG, "Neb starting", argv[1], getpid()); /* Log start-up message                           */
  /* Setup the network socket                                                                       */
  /* Tries to create servere's endpoint communication where to listen for clients                   */
  if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    logger(ERROR, "System call", "socket", 0); /* Log error on failure                              */
  port = atoi(argv[1]); /* ASCII to Integer convertion of cmd line provided port number             */
  if(port < 0 || port > 60000) /* Port value check                                                  */
    logger(ERROR, "Invalid port number (try 1->60000)", argv[1], 0); /* Log error and exit          */
  /* Listen address member set up                                                                   */
  serv_addr.sin_family = AF_INET; /* Address family conventional value (no other needed)            */
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* Bind to 0.0.0.0 (Note byte order convertion)    */
  serv_addr.sin_port = htons(port); /* Set port address (Again byte order convertion)               */
  /* Sin_family member is only used by the kernel to determine what type of address the structure 
   * contains, so it must be in Host Byte Order. Also, since sin_family does not get sent out on
   * the network, it can be stored in Host Byte Order.
   * Sin_addr and sin_port get encapsulated in the packet at the IP and UDP layers, respectively.
   * Thus, they must be in Network Byte Order.
   */
  
  /* Try to bind stored address to the socket                                                       */
  if(bind(listenfd, (_sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) /* Error check                 */
    logger(ERROR, "System call", "bind", 0); /* Log error and exit on bind() failure                */
  /* Due to the complexity of referring to sockaddr sturct members, the sockaddr_in structure was
   * created. However bind(), accept(), connect() require a sockaddr one. Therefore the sockaddr_in
   * was created also to be casted for those functions argument. In that way at the "program level"
   * we are able to access sockaddr bytes in a desirable easy way, while with a simple casting the
   * information remains usable for those functions.
   */
  
  /* Become ready for incoming client's connections. Also request a backlog queue of 64 connections */
  if(listen(listenfd, 64) < 0) /* Error check                                                       */
    logger(ERROR, "System call", "listen", 0); /* Log error and exit upon listen() failure          */
  
  /* Main server infinite loop. For each connection attempt (hit)...                                */
  for(hit = 1; ; hit++) { /* Count each hit                                                         */
    length = sizeof(cli_addr); /* Get initial size of structure                                     */
    if((socketfd = accept(listenfd, (_sockaddr *)&cli_addr, &length)) < 0)
      logger(ERROR, "System call", "accept", 0);
    /* Remember:
     * Accept() is a "blocking call". That means that until a client connects to server execution
     * flow will stuck. In addition it receives the client structure address to fill it with the
     * new client's IP/Port. The idea is that structure remains empty until accept() returns.
     * Moreover, length initial value is modified to contain the actual size in bytes of the address
     * returned.
     * Finally client's file descriptor to which read/write during "talk" is returned.
     */
    
    /* Now a client is connected ****************************************************************** */
    /* Again try to split execution in one child process */
    if((pid = fork()) < 0) /* Error check                                                           */
      logger(ERROR, "System call", "fork", 0); /* Log error and exit program upon failure           */
    else
      if(pid == 0) { /* This block is executed only by child as fork returned 0 to it.              */
	close(listenfd); /* Child no longer needs listen fd, as it won't accept more connextions    */
	web(socketfd, hit); /* Start web process that talks w/client using corresponding socket     */
      }else /* This block is executed by parent as for returned != 0 to it                          */
	close(socketfd); /* Parent server process does not need to talk to client                   */
    /* Now flow returns to the main loop, awaiting in the next accept() call.                       */
  }
}
