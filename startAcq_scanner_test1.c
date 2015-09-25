/* startAcq_scanner_test1.c   
   

  Compiling and Execution
  $ gcc -o exec_s startAcq_scanner_test1.c -Wall -Wextra -lpthread
  $ ./exec_s 54322 timegap(?)  (TCP port)

  There can be several clients from different termials. 
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>  
#include <sys/un.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/epoll.h>

/*
#include "scanner_tablet_msg.h"
*/

#define MAC_DESKTOP_BUILD
#include "scan_ctl_protocol.h"

#define MAXEVENTS         64
#define BACKLOG           5      
#define TCP_BUF_HDR_SIZE  12
#define TCP_BUF_SIZE      256
#define TIME_GAP          10

uint16_t image_test1[160*1000] = {0};

struct epoll_event *ready;  

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

/* function prototype */

void *tcp_client_handler(void *arg);
void data_print_out(int length, char buffer[]); 
void scan_start_acq_in_handler(int fd, UsCtlHeader *buffer); 
void scan_ack_sent_handler(int fd, UsCtlHeader *buffer); 
void scan_dummy_read_all_handler(int fd, UsCtlHeader *buffer); 
void scan_dummy_ack_sent_with_payload_handler(int fd, UsCtlHeader *buffer); 
int startAcqStatus = 0; 

int main(int argc, char *argv[])
{
    int sockTCPfd, new_sockTCPfd, portno, clilen, epfd;
    int n, a=1;
    struct sockaddr_in addr_in, cli_addr;
    struct epoll_event event;    
    pthread_t helper_thread;
    pthread_attr_t thread_attr;

    check(argc < 3, "ERROR: Only missing argument\nUsage: %s <port> <scan status>", argv[0]);

    startAcqStatus = atoi(argv[2]);   /* update teh status */

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

    /* data */
    char buffer[TCP_BUF_HDR_SIZE];
    int i;
    int epfd = *((int *)arg);
    ssize_t data_len=1;

    /* code */
    printf("tcp_client_handler(): TCP socket is binded\n");
    
    while(1) {
	int n = epoll_wait(epfd,ready,MAXEVENTS,-1); 
	for (i = 0; i < n; i++) {
	    memset(buffer,0,TCP_BUF_HDR_SIZE);
	    data_len = read(ready[i].data.fd,buffer,TCP_BUF_HDR_SIZE);
	    /* not then removed from ready list*/
	    if (data_len <= 0) {
		epoll_ctl(epfd, EPOLL_CTL_DEL, ready[i].data.fd, NULL);
		close(ready[i].data.fd);
		printf("???epoll event %d terminated\n",ready[i].data.fd); 
	    }	
	    else {
	        printf("\nTCP server received data with length %d from event %d\n", 
                    (int)data_len, ready[i].data.fd);
	        printf("id: %x; opcode: %x; payload length: %x", 
                    ((UsCtlHeader *)buffer)->id, 
                    ((UsCtlHeader *)buffer)->opcode, 
                    ((UsCtlHeader *)buffer)->length);
		data_print_out(data_len, buffer);
	        switch (((UsCtlHeader *)buffer)->opcode) {
		    case USCTL_START_ACQ:
		        printf("START_ACQUISTION\n");
                        scan_start_acq_in_handler(ready[i].data.fd, 
                            (UsCtlHeader *)buffer); 
/*	                scan_ack_sent_handler(ready[i].data.fd, 
                            (UsCtlHeader *)buffer); */
			scan_dummy_ack_sent_with_payload_handler(ready[i].data.fd, 
                            (UsCtlHeader *)buffer);
		        break;

		    case USCTL_GET_VERSION :
		    case USCTL_GET_MOTOR_PARAMS :
		    case USCTL_GET_ACQU_PARAMS :
                    case USCTL_GET_BATTERY_STATUS:
                        /* tablet -> scanner no payload, ACK with payload */
                        scan_dummy_ack_sent_with_payload_handler(ready[i].data.fd,
                            (UsCtlHeader *)buffer); 
                        break; 

		    case USCTL_SPI_READ :
	                /* payload for both directions */
	                scan_dummy_read_all_handler(ready[i].data.fd,
                            (UsCtlHeader *)buffer); 
                        scan_dummy_ack_sent_with_payload_handler(ready[i].data.fd,
                            (UsCtlHeader *)buffer); 
                        break;

	    	    case USCTL_STOP_ACQ:
		        /* no payload for both direction */
                        scan_ack_sent_handler(ready[i].data.fd, 
                            (UsCtlHeader *)buffer); 
	   	        break;

		    case USCTL_SET_MOTOR_PARAMS :
		    case USCTL_SET_ACQU_PARAMS :
		    case USCTL_SPI_WRITE :
	            default: 
	                /* tablet -> scanner with payload, ACK without payload */
	                scan_dummy_read_all_handler(ready[i].data.fd,
                            (UsCtlHeader *)buffer); 
	                scan_ack_sent_handler(ready[i].data.fd, 
                            (UsCtlHeader *)buffer); 
		        break;
	        }
  	    }
	} 
    }   
}

void scan_start_acq_in_handler(int fd, UsCtlHeader *buffer )
{
    UsCtlStartAcqIn tcpMsg_startAcq;    
    int n_recv; 
    char buf_file[TCP_BUF_SIZE];
 
    printf("scan_start_acq_in_handler(): read rest of data");

    n_recv = read(fd,buf_file, buffer->length); 
    data_print_out(n_recv, buf_file);
    /* decode UDP IP address and */	
    if (n_recv == (int)(buffer->length) )
	{
	memcpy(&(tcpMsg_startAcq.ipAddress), &buf_file, n_recv );
	}
    else
	printf("scan_start_acq_in_handler(): data is missing");
    /* decoding UDP IP address*/	
}


void scan_dummy_read_all_handler(int fd, UsCtlHeader *buffer )
{

    /* read all payload data */	    
    int n_read, n_recv, n_left; 
    char buf_file[TCP_BUF_SIZE];
 
    n_left = buffer->length;     
    while ( n_left > 0 ) {
        if ( n_left >= TCP_BUF_SIZE )
            n_read = TCP_BUF_SIZE; 
	else
	    n_read = n_left; 
	bzero(buf_file, sizeof(buf_file)); 
        n_recv = read(fd,buf_file,n_read); 
  	check( n_recv < 0, "Read %s failed: %s", fd, strerror(errno));
        n_left -= n_recv; 
	}
}

void scan_ack_sent_handler(int fd, UsCtlHeader *buffer )
{

    /* zero payload in ACK with startAcqStatus from argc[2] */	
    UsCtlAck tcpMsg_ack;    
    int msg_len, n_sent; 

    msg_len = sizeof(UsCtlAck); 
    bzero( (void *)&tcpMsg_ack, msg_len);
    tcpMsg_ack.hdr.id = buffer->id; 	
    tcpMsg_ack.hdr.opcode = buffer->opcode;  	
    tcpMsg_ack.hdr.length = 0;  /* payload as zero */
    tcpMsg_ack.status = startAcqStatus; 
    n_sent = write(fd, &tcpMsg_ack, msg_len);
    check( n_sent < 0, "Write %s failed: %s", fd, strerror(errno));
    printf("scan_ack_sent_handler(): Scanner sends ACK with msg length %d to client %d\n", 
        n_sent, fd);
    close(fd);
}

void scan_dummy_ack_sent_with_payload_handler(int fd, UsCtlHeader *buffer)
{

    /* data */	
    char *buf_sent; 
    int msg_len, n_sent; 

    switch (buffer->opcode) {
	case USCTL_GET_VERSION :
	    msg_len = sizeof(UsCtlGetVersionOut); 
	    break;
 
	case USCTL_GET_MOTOR_PARAMS:
	    msg_len = sizeof(UsCtlGetMotorOut); 
	    break;

	case USCTL_GET_ACQU_PARAMS:
	    msg_len = sizeof(UsCtlGetAcqParamOut); 
	    break;

	case USCTL_GET_BATTERY_STATUS:
	    msg_len = sizeof(UsCtlGetBatteryStatusOut); 
	    break;

	case USCTL_SPI_READ:
	    msg_len = sizeof(UsCtlSPIReadOut); 
            break; 

        case USCTL_START_ACQ:
	    msg_len = sizeof(UsCtlStartAcqOut); 
            break; 	

	default:
             printf("scan_ack_sent_with_payload_handler(): unknown id %x:\n", 
                  buffer->opcode);
	     break;
    }
    buf_sent = (char *) malloc(msg_len);
    bzero( (void *)buf_sent, msg_len);
    ((UsCtlAck *)buf_sent)->hdr.id = buffer->id; 	
    ((UsCtlAck *)buf_sent)->hdr.opcode = buffer->opcode;  	
    ((UsCtlAck *)buf_sent)->hdr.length = msg_len-sizeof(UsCtlAck);  /* payload as zero */
    ((UsCtlAck *)buf_sent)->status = startAcqStatus; 
    n_sent = write(fd,buf_sent, msg_len);
    check( n_sent < 0, "Write %s failed: %s", fd, strerror(errno));
    printf("scan_dummy_ack_sent_with_payload_handler(): Scanner sends ACK with msg length %d to client %d\n", 
        n_sent, fd);
    data_print_out(n_sent, buf_sent);
    close(fd);
}

void data_print_out(int length, char buffer[]){
    int i;

    for ( i = 0; i < length; i++) {
        if (!(i%16)) printf("\n%08x  ", i);
        if (!(i%8)) printf(" ");
        printf("%02x ", (uint8_t)buffer[i]);
    }
    printf("\n"); 
}


