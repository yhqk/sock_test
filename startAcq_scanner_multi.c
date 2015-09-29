/* startAcq_scanner_test1.c   
   

  Compiling and Execution
  $ gcc -o exec_s startAcq_scanner_test1.c -Wall -Wextra -lpthread -I${PWD}/images/
  $ ./exec_s 54322 0 500

  Debugging
  gcc -O0 -g -o exec_s startAcq_scanner_test1.c -Wall -Wextra -lpthread -I${PWD}/images/
  gdb --args  ./exec_s 54322 0 500

  There can be several clients from different termials. 
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>  
#include <sys/un.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/epoll.h>
#include <time.h>

/* local include files */
#define MAC_DESKTOP_BUILD
#include "scan_ctl_protocol.h"

/* image data header files */
#include "data_array_image01.h"
#include "data_array_image02.h"
#include "data_array_image03.h"
#include "data_array_image04.h"
#include "data_array_image05.h"
#include "data_array_image06.h"
#include "data_array_image07.h"
#include "data_array_image08.h"
#include "data_array_image09.h"
#include "data_array_image10.h"

/* Definitions */ 
/* epoll related parameters */
#define MAXEVENTS         64
#define BACKLOG           5 

/* TCP */     
#define TCP_BUF_HDR_SIZE  12
#define TCP_BUF_SIZE      256

/* UDP */
#define DGRAM_SIZE        1472
#define IMAGE_BLOCK_SIZE  1448
#define TIME_GAP          10

/* Image size */
#define MAX_ROW           10
#define MAX_COLUMN        160*1000

/*
uint16_t image_test1[160*1000] = {0};
*/

/* macro */
#define rows_of_array(name)       \
    (sizeof(name   ) / sizeof(name[0][0]) / columns_of_array(name))
#define columns_of_array(name)    \
    (sizeof(name[0]) / sizeof(name[0][0]))

struct epoll_event *ready;  

typedef struct _udpPacket {
  uint32_t blockId;
  uint32_t packetId;
  uint16_t spare1;
  uint16_t spare2;
  uint32_t offset;
  uint64_t timestamp;
  uint16_t image[IMAGE_BLOCK_SIZE];
} udpPacket;


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

/* Local function prototype */

void *tcp_client_handler(void *arg);
void data_print_out(int length, char buffer[]); 
void scan_start_acq_in_handler(int fd, UsCtlHeader *buffer); 
void scan_ack_sent_handler(int fd, UsCtlHeader *buffer); 
void scan_dummy_read_all_handler(int fd, UsCtlHeader *buffer); 
void scan_dummy_ack_sent_with_payload_handler(int fd, UsCtlHeader *buffer); 
void scan_udp_connection_handler(char *hostname, unsigned int portno); 
void scan_udp_data_sent_handler(int udp_fd); 
void scan_read_one_byte_handler(int fd); 

/* Local data */
int startAcqStatus = 0;      /* 0 is OK; 1 already_run, 2 not supported */
int delayMicroSec = 500;     /* 19 Mbits/sec  */

/* Functions */
int main(int argc, char *argv[])
{
    /* Data */
    int sockTCPfd, new_sockTCPfd, portno, clilen, epfd;
    int n, a=1;
    struct sockaddr_in addr_in, cli_addr;
    struct epoll_event event;    
    pthread_t helper_thread;
    pthread_attr_t thread_attr;

    /* Code */
    check(argc < 4, "ERROR: Only missing argument\nUsage: %s <port> <scan status> <delay time microsecond>", argv[0]);

    startAcqStatus = atoi(argv[2]);   
    delayMicroSec = atoi(argv[3]); 

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

    /* Data */
    char buffer[TCP_BUF_HDR_SIZE];
    int i;
    int epfd = *((int *)arg);
    ssize_t data_len=1;

    /* Code */
    printf("Scanner/tcp_client_handler(): TCP socket is binded\n");
    
    while(1) {
	int n = epoll_wait(epfd,ready,MAXEVENTS,-1); 
	for (i = 0; i < n; i++) {
	    memset(buffer,0,TCP_BUF_HDR_SIZE);
	    data_len = read(ready[i].data.fd,buffer,TCP_BUF_HDR_SIZE);
	    /* not then removed from ready list*/
	    if (data_len <= 0) {
		epoll_ctl(epfd, EPOLL_CTL_DEL, ready[i].data.fd, NULL);
		close(ready[i].data.fd);
		printf("epoll event %d terminated\n",ready[i].data.fd); 
	    }	
	    else {
	        printf("TCP server received data with length %d from event %d\n", 
                    (int)data_len, ready[i].data.fd);
	        printf("id: 0x%2x; opcode: 0x%2x; payload length: %d", 
                    ((UsCtlHeader *)buffer)->id, 
                    ((UsCtlHeader *)buffer)->opcode, 
                    ((UsCtlHeader *)buffer)->length);
		data_print_out(data_len, buffer);
	        switch (((UsCtlHeader *)buffer)->opcode) {
		    case USCTL_START_ACQ:
                        scan_start_acq_in_handler(ready[i].data.fd, 
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
	                printf("===> USCTL_SPI_READ <===\n"); 
	                scan_dummy_read_all_handler(ready[i].data.fd,
                            (UsCtlHeader *)buffer); 
                        scan_dummy_ack_sent_with_payload_handler(ready[i].data.fd,
                            (UsCtlHeader *)buffer); 
                        break;

	    	    case USCTL_STOP_ACQ:
	                printf("===> USCTL_STOP_ACQ <===\n"); 
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
    /* Data */
    UsCtlStartAcqIn tcpMsg_startAcq;    
    int n_recv; 
    char buf_file[TCP_BUF_SIZE];

    struct sockaddr_in udpAddr;
    char *s_addr;

    /* Code */ 
    printf("===> START_ACQUISTION\n");
    printf("scan_start_acq_in_handler(): rest of data:");
    n_recv = read(fd,buf_file, buffer->length); 
    data_print_out(n_recv, buf_file);

    printf("n_recv: %d, payload:%d\n", 
	    n_recv,
            (int)(buffer->length) );	
    if (n_recv == (int)(buffer->length))
	{
	scan_ack_sent_handler(fd, buffer);  /* TCP socket closed */

	memcpy(&(tcpMsg_startAcq.ipAddress), &buf_file, n_recv );
        udpAddr.sin_addr.s_addr = htonl(tcpMsg_startAcq.ipAddress);
        s_addr = (char *) malloc(INET_ADDRSTRLEN);
        s_addr = inet_ntoa(udpAddr.sin_addr);
        printf("received UDP IP address: %s/0x%x; port:%x\n", 
	    s_addr, tcpMsg_startAcq.ipAddress,
            tcpMsg_startAcq.port ); 
        scan_udp_connection_handler(s_addr, 
	    tcpMsg_startAcq.port ); 
	free(s_addr); 	
	}
    else
	printf("scan_start_acq_in_handler(): data is missing");
}

void scan_udp_connection_handler(char *hostname, unsigned int portno )
{
    /* Data */  
    int sockUDPfd; 
    int ret_val; 
    struct sockaddr_in udpServAddr;
    struct hostent *udpServer;
    socklen_t slen = sizeof(struct sockaddr_in);
    udpPacket dgram;
    uint32_t block_id=0, packet_id=0, offset=0;
    uint32_t copy_size; 	
    ssize_t nsent = 0, total_sent=0;
    time_t time_start, time_gap, time_start_per_image; 
    float throughput;
    int n=1, i;
    char *image[10]; 

    /* Code */ 

    sockUDPfd = socket(AF_INET, SOCK_DGRAM, 0);
    check(sockUDPfd < 0, "Scanner: UDP socket create %s failed: %s", 
        sockUDPfd, strerror(errno));
    udpServer = gethostbyname(hostname);
    if (udpServer == NULL) {
	fprintf(stderr,"ERROR, no such UDP host\n");
	exit(0);
    }
   
    memset((char *) &udpServAddr, 0,sizeof(udpServAddr));  
    udpServAddr.sin_family = AF_INET;
    udpServAddr.sin_addr.s_addr = inet_addr(hostname);
    udpServAddr.sin_port = htons(portno);
    ret_val = connect(sockUDPfd,(struct sockaddr *)&udpServAddr, sizeof(udpServAddr)); 
    check (ret_val < 0, "UDP socket connection%s failed: %s", 
        sockUDPfd, strerror(errno));
    printf("Connect UDP socket as client %s: %d\n", 
        inet_ntoa(udpServAddr.sin_addr), 
        ntohs(udpServAddr.sin_port)); 	

    /* create data array */
    	
    image[0] = (char *)&data01[0];
    image[1] = (char *)&data02[0];
    image[2] = (char *)&data03[0];
    image[3] = (char *)&data04[0];
    image[4] = (char *)&data05[0];
    image[5] = (char *)&data06[0];
    image[6] = (char *)&data07[0];
    image[7] = (char *)&data08[0];
    image[8] = (char *)&data09[0];
    image[9] = (char *)&data10[0];

    time_start = time(NULL); /* seconds */  
    time_start_per_image = time_start;
    i = 0; 
    offset = 0;
    while(1) {
	        dgram.blockId = block_id;
	        dgram.packetId = packet_id;
	        dgram.offset = offset;
	        copy_size = sizeof(data01) - offset; 
	        if (copy_size > IMAGE_BLOCK_SIZE ) {
	            copy_size = IMAGE_BLOCK_SIZE;
	        }
//                printf("image[i=%d][offset=%d, copy_size=%d] = %p\n", i, offset, copy_size, &image[i][offset]);
//                printf("*(uint16_t *)&image[i][offset] = %d\n", *(uint16_t *)&image[i][offset]);
//                fflush(stdout); 
                memcpy(&(dgram.image), &image[i][offset], copy_size);
	        nsent = sendto(sockUDPfd, &dgram, DGRAM_SIZE, 0, (struct sockaddr *)&udpServAddr, slen); 
//                printf("nsent = %d, nsent-hdr=%d\n", (uint32_t)nsent, (uint32_t)nsent-(DGRAM_SIZE-IMAGE_BLOCK_SIZE));
//                fflush(stdout); 
//             check (nsent < 0, "sentto() %s failed: %s", sockUDPfd, strerror(errno));
// ??? No nsent checking whether it effects on offset and copy_size (resent?)
#if 0 
	        if (dgram.packetId == 0) {
	            data_print_out(32, (char *)&(dgram.image));
                    printf("copy_size:%d; offset :%d\n", 
		       copy_size, 
		       offset );
                }
#endif /* 0 */
	        packet_id++;
#if 0
                if (nsent > (DGRAM_SIZE - IMAGE_BLOCK_SIZE)) {
                    offset += nsent - (DGRAM_SIZE - IMAGE_BLOCK_SIZE);
                }
#else
    	        offset += copy_size;
#endif
	        time_gap = time(NULL) - time_start;
	        total_sent += nsent; 
	        if ( time_gap >= TIME_GAP )  {
	           printf("blockID:%05d; packetId:%3d\n", 
		       dgram.blockId, 
		       dgram.packetId);
	           throughput = ((double)total_sent/1000000)/time_gap; 
	           printf("==> Average throughput/%ds: %.2f MB/s = %.2f Mbits/s <==\n", 
		       TIME_GAP, throughput, throughput*8 ); 
	           time_start = time(NULL);
	           total_sent = 0; 
	        } 	

	        /* assuming same size */
    	        if (offset >= sizeof(data01)) {
       	            packet_id = 0;
	            offset = 0;
                    i += 1;
                    if (i >= MAX_ROW) {
                        i = 0;
                    }
      	            block_id++;

	            time_gap = time(NULL) - time_start_per_image;
	            if (time_gap > 2 ) {
	                throughput = ((double)(sizeof(data01)*n)/1000000)/time_gap;
		        printf("blockId%4d: throughput/(%d images): %.2f MB/s = %.2f Mbits/s\n", 
	                    block_id, n, throughput, throughput*8 ); 
	    	            time_start_per_image = time(NULL);
	                n = 0; 
		    } else {
		        n++; 
	    	    } 
    	        }
       	        usleep(delayMicroSec);
    }
}

void scan_dummy_read_all_handler(int fd, UsCtlHeader *buffer )
{

    /* Data */    
    int n_read, n_recv, n_left; 
    char buf_file[TCP_BUF_SIZE];

    /* Code */
    switch (buffer->opcode) {
	case USCTL_SET_MOTOR_PARAMS:
	    printf("===> USCTL_SET_MOTOR_PARAMS\n"); 
	    break;
 
	case USCTL_SET_ACQU_PARAMS:
	    printf("===> USCTL_SET_ACQU_PARAMS\n"); 
	    break;

	case USCTL_SPI_WRITE:
	    printf("===> USCTL_SPI_WRITE\n"); 
	    break;	

	default:
             printf("scan_dummy_read_all_handler: unknown id %x:\n", 
                  buffer->opcode);
	     break;
    } 

    n_left = buffer->length;     
    while ( n_left > 0 ) {
        printf("scan_dummy_read_all_handler(): rest of data:"); 
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

    /* Data */   
    /* zero payload in ACK with startAcqStatus from argc[2] */	
    char *buf_sent;
    int n_sent; 

    /* Code */
    buf_sent = (char *) malloc(sizeof(UsCtlAck));
    bzero( (void *)buf_sent, sizeof(UsCtlAck));
    ((UsCtlAck *)buf_sent)->hdr.id = buffer->id; 	
    ((UsCtlAck *)buf_sent)->hdr.opcode = buffer->opcode;  	
    ((UsCtlAck *)buf_sent)->hdr.length = 0; 
    ((UsCtlAck *)buf_sent)->status = startAcqStatus; 
    n_sent = write(fd,buf_sent, sizeof(UsCtlAck));
    check( n_sent < 0, "Write %s failed: %s", fd, strerror(errno));
    printf("\nscan_ack_sent_handler(): Scanner->ACK to client %d:", fd);
    data_print_out(n_sent, buf_sent);
    free(buf_sent); 
    scan_read_one_byte_handler(fd);  
    close(fd);
}

void scan_dummy_ack_sent_with_payload_handler(int fd, UsCtlHeader *buffer)
{

    /* data */	
    char *buf_sent; 
    int msg_len, n_sent; 

    /* Code */
    switch (buffer->opcode) {
	case USCTL_GET_VERSION:
	    printf("===> USCTL_GET_VERSION\n"); 
	    msg_len = sizeof(UsCtlGetVersionOut); 
	    break;
 
	case USCTL_GET_MOTOR_PARAMS:
	    printf("===> USCTL_GET_MOTOR_PARAMS\n"); 
	    msg_len = sizeof(UsCtlGetMotorOut); 
	    break;

	case USCTL_GET_ACQU_PARAMS:
	    printf("===> USCTL_GET_ACQU_PARAMS\n"); 
	    msg_len = sizeof(UsCtlGetAcqParamOut); 
	    break;

	case USCTL_GET_BATTERY_STATUS:
	    printf("===> USCTL_GET_BATTERY_STATUS\n"); 
	    msg_len = sizeof(UsCtlGetBatteryStatusOut); 
	    break;

	case USCTL_SPI_READ:
	    printf("USCTL_SPI_READ\n"); 
	    msg_len = sizeof(UsCtlSPIReadOut); 
            break; 

        case USCTL_START_ACQ:
	    printf("===> USCTL_START_ACQ\n"); 
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
    ((UsCtlAck *)buf_sent)->hdr.length = msg_len-sizeof(UsCtlAck);
    ((UsCtlAck *)buf_sent)->status = startAcqStatus; 
    n_sent = write(fd,buf_sent, msg_len);
    check( n_sent < 0, "Write %s failed: %s", fd, strerror(errno));
    printf("scan_dummy_ack_sent_with_payload_handler(): Scanner->ACK+payload(%d) to client %d:", 
         n_sent, fd);
    data_print_out(n_sent, buf_sent);
    free(buf_sent);
    scan_read_one_byte_handler(fd);  
    close(fd);
}

void scan_read_one_byte_handler(int fd)
{
    /* Currently scanner code sends one byte after ACK/NACK */
    /* Data */

    char buf_file[1];
    int n_recv; 

    /* Code */ 
    printf("scan_read_one_byte_handler() before TCP socket closed\n");

    n_recv = read(fd,buf_file, 1); 
    check( n_recv < 0, "Read %s failed: %s", fd, strerror(errno));
}


void data_print_out(int length, char buffer[])
{
    /* Data */
    int i;

    /* Code */
    for ( i = 0; i < length; i++) {
        if (!(i%16)) printf("\n%08x  ", i);
        if (!(i%8)) printf(" ");
        printf("%02x ", (uint8_t)buffer[i]);
    }
    printf("\n"); 
}


