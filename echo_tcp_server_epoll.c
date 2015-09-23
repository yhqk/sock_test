/* echo_tcp_server_epoll.c   
  TCP socket server is handled as thread with epoll, and received 
  data from TCP client, then echo back client 5 times. 

  Compiling and Execution
  $ gcc -o exec_s echo_tcp_server_epoll.c -Wall -Wextra -lpthread
  $ ./exec_s 54322

  There can be several clients from different termials. 
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>   // close, read
#include <sys/un.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/epoll.h>

#define MAXEVENTS 64
#define BACKLOG 5
#define BUF_SIZE 256
#define MAX_ECHO 5

/* Local functions */
static void check (int test, const char * message, ...)
{
    if (test) {
        va_list args;
        va_start (args, message);
        vfprintf (stderr, message, args);
        va_end (args);
        fprintf (stderr, "\n");
        exit (EXIT_FAILURE);
    }
}

void *tcp_client_handler(void *arg);
struct epoll_event *ready;  

int main(int argc, char *argv[])
{
    int sockTCPfd, new_sockTCPfd, portno, clilen, epfd;
    int n, a=1;
    struct sockaddr_in addr_in, cli_addr;
    struct epoll_event event;    
    pthread_t helper_thread;
    pthread_attr_t thread_attr;

    check(argc < 2, "ERROR: Only missing argument\nUsage: %s <port>", argv[0]);

    /* create thread with epoll */
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr,PTHREAD_CREATE_DETACHED);
    epfd = epoll_create(MAXEVENTS);
    ready = (struct epoll_event*)calloc(MAXEVENTS,sizeof(event));

    sockTCPfd = socket(AF_INET, SOCK_STREAM, 0);
    check(sockTCPfd < 0, "TCP server create %s failed: %s", sockTCPfd, strerror(errno));

    memset((char *) &addr_in ,0,sizeof(addr_in));
    portno = atoi(argv[1]);
    addr_in.sin_family = AF_INET;
    addr_in.sin_addr.s_addr = INADDR_ANY;
    addr_in.sin_port = htons(portno);
    setsockopt(sockTCPfd, SOL_SOCKET, SO_REUSEADDR, &a, sizeof(int));
    n = bind(sockTCPfd, (struct sockaddr *) &addr_in, sizeof(addr_in)); 
    check (n < 0, "TCP socket bind %s failed: %s", sockTCPfd, strerror(errno));

    pthread_create(&helper_thread, &thread_attr, tcp_client_handler, &epfd);	
    while(1) {
	listen(sockTCPfd,BACKLOG);
	clilen = sizeof(cli_addr);
	new_sockTCPfd = accept(sockTCPfd,  
			(struct sockaddr *) &cli_addr, 
			(socklen_t *)&clilen);
        check (new_sockTCPfd < 0, "TCP socket accept %s failed: %s", 
	       sockTCPfd, strerror(errno));	
	event.data.fd = new_sockTCPfd;
	event.events = EPOLLIN;
	epoll_ctl (epfd, EPOLL_CTL_ADD, new_sockTCPfd, &event);
    }
    return 0;
}

void * tcp_client_handler(void *arg) {
    char buffer[BUF_SIZE];
    int i, j, counter = 0;
    int epfd = *((int *)arg);
    ssize_t data_len=1;

    printf("tcp_client_handler(): TCP socket is binded\n");
    
    while(1) {
	int n = epoll_wait(epfd,ready,MAXEVENTS,-1); 
	for (i = 0; i < n; i++) {
	    memset(buffer,0,BUF_SIZE);
	    data_len = read(ready[i].data.fd,buffer,BUF_SIZE);
	    /* not then removed from ready list*/
	    if (data_len <= 0) {
		epoll_ctl(epfd, EPOLL_CTL_DEL, ready[i].data.fd, NULL);
		close(ready[i].data.fd);
		printf("epoll event %d terminated\n",ready[i].data.fd);
	    }	
	    else {
	        printf("\nTCP server received data with length %d from event %d", 
                    (int)data_len, ready[i].data.fd);
	        for( j = 0; j < data_len; j++) {
		    if (!(j%16)) printf("\n%08x  ", j);
		    if (!(j%8)) printf(" ");
		    printf("%02x ", (uint8_t)(buffer[j]));
		}
                printf("\n"); 
		if ( counter <= MAX_ECHO ) {	
		    data_len = write(ready[i].data.fd,buffer,data_len);
	            printf("No%2d: Echo data from TCP server data lenght %d to event%d\n", 
	                counter, (int)data_len, ready[i].data.fd);
		    counter++; 
		}
		else printf("No more echo, Ctril-C to stop TCP server\n"); 			
	    }
	} 
    }   
}
