/* socket_udp_server_thread.c   
  Server is handled as thread with epoll
  Server receives textline (size 256) from client(s)
  and print out via stdio 

  Compiling and Execution
  $ gcc -o exec_s socket_udp_server_thread.c -Wall -Wextra -lpthread
  $ ./exec_s 5000 

Maybe no use for this epoll
http://stackoverflow.com/questions/3959295/multithreading-udp-server-with-epoll

  There can be several clients from different terminals. 
 */

#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>   // close, read
#include <sys/un.h>

#define MAXEVENTS 64
#define BACKLOG 5
#define BUF_SIZE 256

void error(char *msg)
{
    perror(msg);
    exit(1);
}

void *client_handler(void *arg);
struct epoll_event *ready;  

int main(int argc, char *argv[])
{
    int sockUDPfd, portno, epfd;
    int a = 1;
    struct sockaddr_in addr_in;
    socklen_t len = sizeof(struct sockaddr_in);
    struct epoll_event event;    
    pthread_t helper_thread;
    pthread_attr_t thread_attr;

    /* Check arguments */
    if (argc < 2) {
	fprintf(stderr,"ERROR, no port provided\n");
	exit(1);
    }

    /* create thread with epoll */
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr,PTHREAD_CREATE_DETACHED);
    epfd = epoll_create(MAXEVENTS);
    ready = (struct epoll_event*)calloc(MAXEVENTS,sizeof(event));

    sockUDPfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockUDPfd < 0)        
	error("ERROR internet UDP socket create");
    memset((char *) &addr_in ,0,sizeof(addr_in));
    portno = atoi(argv[1]);
    addr_in.sin_family = AF_INET;
    addr_in.sin_addr.s_addr = INADDR_ANY;
    addr_in.sin_port = htons(portno);
    setsockopt(sockUDPfd, SOL_SOCKET, SO_REUSEADDR, &a, sizeof(int));
    if (bind(sockUDPfd, (struct sockaddr *) &addr_in, sizeof(addr_in)) < 0) 
	error("ERROR internet UDP socket binding");

    pthread_create(&helper_thread, &thread_attr, client_handler, &epfd);	
    while(1) {
	listen(sockUDPfd,BACKLOG);  /* listen is not part of UDP no point of epoll*/
/*	n = recvfrom(sockUDPfd, buf, BUF_SIZE, 0, (struct sockaddr *)&addr_in, &len); */
	event.data.fd = sockUDPfd;
	event.events = EPOLLIN;
	epoll_ctl(epfd, EPOLL_CTL_ADD, sockUDPfd, &event);
    }
    return 0;
}

void * client_handler(void *arg) {
    char buffer[BUF_SIZE];
    int i;
    int epfd = *((int *)arg);
    ssize_t a; 

    printf("Server:client_handler(): UDP socket is binded\n");
    
    while(1) {
	int n = epoll_wait(epfd,ready,MAXEVENTS,-1); 
	for (i = 0; i < n; i++) {
	    memset(buffer,0,BUF_SIZE);
	    a = read(ready[i].data.fd,buffer,BUF_SIZE);
	    /* not then removed from ready list*/
	    if (a <= 0) {
		epoll_ctl(epfd, EPOLL_CTL_DEL, ready[i].data.fd, NULL);
		close(ready[i].data.fd);
		printf("epoll event %d terminated\n",ready[i].data.fd);
	    }	
	    else {
		if (strlen(buffer) > 30) {
		    /* In case client is just press enter timestamp with size of 30*/
		    printf("%s",buffer);	
		}
	    }
	} 
    }   
}
