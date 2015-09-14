/* socket_tcp_client.c
  A client enters textline to server and also receive echo back message  
  Server is handled as thread with epoll

  Compiling and Execution
  $ gcc -o exec_c socket_tcp_client.c -Wall
  $ ./exec_c localhost 5000  
  or
  $ ./exec_c 192.168.1.46 5000

  There can be several clients from different termials. 
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/time.h>
#include <time.h>
#include <unistd.h> 

#define BUF_SIZE 256

void error(char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;  // old the way.     
    char buffer[BUF_SIZE];
    char buffer_sent[BUF_SIZE];
    char buffer_recv[BUF_SIZE];
    int clientId, len; 
    char str_id[15];
    struct timeval tv;
    struct tm* ptm;
    char time_string[35];

    n = 1;  // in order to go to the 1st while loop
    
    if (argc < 3) {
	fprintf(stderr,"usage %s hostname port\n", argv[0]);
	exit(0);
    }
	
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
	error("ERROR opening socket");
    }

    server = gethostbyname(argv[1]);
    if (server == NULL) {
	fprintf(stderr,"ERROR, no such host\n");
	exit(0);
    }
   
    memset((char *) &serv_addr, 0,sizeof(serv_addr));  
    serv_addr.sin_family = AF_INET;
    memcpy((char *)&serv_addr.sin_addr.s_addr,(char *)server->h_addr, 
	   server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,
		sizeof(serv_addr)) < 0) {
	error("ERROR connecting");
    }

    srand(time(0));     
    clientId = rand() % 100;  // clientID 01-99
    sprintf(str_id, "client%02d:", clientId); 

    while (n > 0) {
	printf("Please enter the text from %s ", str_id);
        bzero( buffer_sent, BUF_SIZE);
        bzero( buffer_recv, BUF_SIZE);
	/* Obtain the time of day, and convert it to a tm struct. */
	gettimeofday (&tv, NULL);
	ptm = localtime (&tv.tv_sec);
	strftime (time_string, sizeof (time_string), 
		    "%Y-%m-%d %H:%M:%S", ptm);
	/* Add client ID and time stamp */
	strcpy(buffer_sent, str_id); 
	strcat(buffer_sent, time_string);
	strcat(buffer_sent, " ");
	/* get order */
	len = BUF_SIZE-strlen(buffer_sent);
	memset(buffer,0,len);
	fgets(buffer,len,stdin); 
	strcat(buffer_sent, buffer);
	n = write(sockfd,buffer_sent,strlen(buffer_sent));
	if (n < 0) {
	    error("ERROR writing to socket");
	}  
	n = read(sockfd,buffer_recv, BUF_SIZE);
	if (n > 0) {
	    printf("***Echo from Server***: %s",buffer_recv);
	    }	
        else {
            printf("nothing to read");
	}
    }
    return 1; 
}
