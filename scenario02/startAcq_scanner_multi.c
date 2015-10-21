/* startAcq_scanner_multi.c   
  - Scanner starts TCP server; 
  - Wait clients sending Start_Acquisition with UDP IP address/port; 
  - Reply all messages from TCP with ACK (OK)
  - Connect UDP to tablet as client; 
  - Send 10 images via UDP socket as loop. 
  => this version don't handle the Stop_Acquisition
   

  Compiling and Execution
  $ gcc -o exec_s startAcq_scanner_multi.c -Wall -Wextra -lpthread -I${PWD}/images/
  $ ./exec_s 54322 0 500

  Debugging
  gcc -O0 -g -o exec_s startAcq_scanner_multi.c -Wall -Wextra -lpthread -I${PWD}/images/
  gdb --args  ./exec_s 54322 0 500

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

/* Local header files */
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

/* Structure defintion */
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
void scan_msg_handler(int fd, UsCtlHeader *buffer ); 

void scan_start_acq_in_handler(int fd, UsCtlHeader *buffer); 
void scan_ack_sent_handler(int fd, UsCtlHeader *buffer); 
void scan_dummy_read_all_handler(int fd, UsCtlHeader *buffer); 
void scan_dummy_ack_sent_with_payload_handler(int fd, UsCtlHeader *buffer); 
void scan_udp_connection_handler(char *hostname, unsigned int portno); 
void scan_udp_data_sent_handler(int udp_fd); 

void data_print_out(int length, char buffer[]); 
void scan_read_one_byte_handler(int fd); 

/* global variables */
int startAcqStatus = 0;      /* 0 is OK; 1 already_run, 2 not supported */
int delayMicroSec = 500;     /* 19 Mbits/sec  */
int sockUDPfd = -1;          /* No sockUDPfd*/    

/* Main Function */
int main(int argc, char *argv[])
{
    /* Data */
    int sockTCPfd, new_sockTCPfd, portno, clilen, epfd;
    int ret_val, a=1;
    struct sockaddr_in addr_in, cli_addr;
    struct epoll_event event;    
    pthread_t helper_thread;
    pthread_attr_t thread_attr;

    /* Code */
    check(argc < 4, 
          "ERROR: Only missing argument\nUsage: %s <port> <scan status> <delay time microsecond>",
          argv[0]);

    startAcqStatus = atoi(argv[2]);   
    delayMicroSec = atoi(argv[3]); 

    /* create thread with epoll */
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr,PTHREAD_CREATE_DETACHED);

    epfd = epoll_create(MAXEVENTS);
    ready = (struct epoll_event*)calloc(MAXEVENTS,sizeof(event));

    sockTCPfd = socket(AF_INET, SOCK_STREAM, 0);
    check(sockTCPfd < 0, "TCP server create %s failed: %s", 
          sockTCPfd, strerror(errno));

    memset((char *) &addr_in ,0,sizeof(addr_in));
    portno = atoi(argv[1]);
    addr_in.sin_family = AF_INET;
    addr_in.sin_addr.s_addr = INADDR_ANY;
    addr_in.sin_port = htons(portno);
    setsockopt(sockTCPfd, SOL_SOCKET, SO_REUSEADDR, &a, sizeof(int));
    ret_val = bind(sockTCPfd, (struct sockaddr *) &addr_in, sizeof(addr_in)); 
    check (ret_val < 0, "TCP socket bind %s failed: %s", 
           sockTCPfd, strerror(errno));

    pthread_create(&helper_thread, &thread_attr, tcp_client_handler, &epfd);	
    while(1) {
        listen(sockTCPfd,BACKLOG);
        clilen = sizeof(cli_addr);
        new_sockTCPfd = accept(sockTCPfd,  
                               (struct sockaddr *) &cli_addr, 
                               (socklen_t *)&clilen);
        check (new_sockTCPfd < 0, 
               "TCP socket accept %s failed: %s", sockTCPfd, 
               strerror(errno));	
        event.data.fd = new_sockTCPfd;
        event.events = EPOLLIN;
        epoll_ctl (epfd, EPOLL_CTL_ADD, new_sockTCPfd, &event);
    }
    return 0;
} /* main */

/* Local functions */
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
                printf("Scanner/TCP receives data with length %d from event %d\n", 
                       (int)data_len, ready[i].data.fd);
                scan_msg_handler(ready[i].data.fd, (UsCtlHeader *)buffer); 
            } 
        } /* for loop */
    } /* while loop */
} /* tcp_client_handler */

void scan_msg_handler(int fd, UsCtlHeader *buffer )
{   
    printf("TransactionId: 0x%2x; OpCode:0x%2x; PayLoad length:%d\n", 
           buffer->id, buffer->opcode, buffer->length);
    
    switch (buffer->opcode) {
        case USCTL_GET_VERSION :
            printf("===>USCTL_GET_VERSION<===\n");
            scan_dummy_ack_sent_with_payload_handler(fd, buffer); 
        break;         

        case USCTL_SET_MOTOR_PARAMS :
            printf("===>USCTL_SET_MOTOR_PARAMS<===\n");        
            scan_dummy_read_all_handler(fd, buffer); 
            scan_ack_sent_handler(fd, buffer); 
        break; 
        
        case USCTL_GET_MOTOR_PARAMS :
            printf("===>USCTL_GET_MOTOR_PARAMS<===\n");
            scan_dummy_ack_sent_with_payload_handler(fd, buffer); 
            break; 
        
        case USCTL_SET_ACQU_PARAMS :
            printf("===>USCTL_SET_ACQU_PARAMS<===\n");
            scan_dummy_read_all_handler(fd, buffer); 
            scan_ack_sent_handler(fd, buffer); 
        break;         

        case USCTL_GET_ACQU_PARAMS :
            printf("===>USCTL_GET_ACQU_PARAMS<===\n");
            scan_dummy_ack_sent_with_payload_handler(fd, buffer);  
        break; 
        
        case USCTL_GET_BATTERY_STATUS :
            printf("===>USCTL_GET_BATTERY_STATUS<===\n");        
            scan_dummy_ack_sent_with_payload_handler(fd, buffer);
        break; 

        case USCTL_SPI_WRITE :
            printf("===>USCTL_SPI_READ<===\n");
            scan_dummy_read_all_handler(fd, buffer); 
            scan_ack_sent_handler(fd, buffer);                                                      
        break; 
        
        case USCTL_SPI_READ :
            printf("===>USCTL_SPI_READ<===\n"); 
            scan_dummy_read_all_handler(fd, buffer);
            scan_dummy_ack_sent_with_payload_handler(fd, buffer);
        break;

        case USCTL_START_ACQ :
            printf("===>USCTL_START_ACQ<===\n");
            scan_start_acq_in_handler(fd, buffer); 
        break;   

        case USCTL_STOP_ACQ:
            printf("===>USCTL_STOP_ACQ<===\n"); 
            scan_ack_sent_handler(fd, buffer); 
        break;
                                                     
        default:
            printf("===>Unknown message<===\n"); 
            scan_dummy_read_all_handler(fd, buffer); 
            scan_ack_sent_handler(fd, buffer); 
        break;
    }   
} /* scan_msg_handler */

void scan_start_acq_in_handler(int fd, UsCtlHeader *buffer )
{  
    /* Data */
    UsCtlStartAcqIn tcpMsg_startAcq;    
    int n_recv; 
    char buf_file[TCP_BUF_SIZE];
    struct sockaddr_in udpAddr;
    char *s_addr; 
    
    /* Code */ 
    printf("scan_start_acq_in_handler(): rest of data:");
    n_recv = read(fd,buf_file, buffer->length); 
    data_print_out(n_recv, buf_file);
    printf("   Received data length: %d; PayLoad length:%d\n", 
           n_recv, (int)(buffer->length) );	
    if ( n_recv == (int)(buffer->length) ) {
        scan_ack_sent_handler(fd, buffer); 
	memcpy(&(tcpMsg_startAcq.ipAddress), &buf_file, n_recv );
        udpAddr.sin_addr.s_addr = htonl(tcpMsg_startAcq.ipAddress);
        s_addr = (char *) malloc(INET_ADDRSTRLEN);
        s_addr = inet_ntoa(udpAddr.sin_addr);
        printf("received UDP IP address: %s/0x%x; port:%x\n", 
               s_addr, tcpMsg_startAcq.ipAddress,
               tcpMsg_startAcq.port ); 
        scan_udp_connection_handler(s_addr, tcpMsg_startAcq.port ); 
        free(s_addr); 	
	}
    else
        printf("scan_start_acq_in_handler(): Error/length is different wwith recieved");
} /* scan_start_acq_in_handler */

void scan_udp_connection_handler(char *hostname, unsigned int portno )
{
    /* Data */  
    int ret_val, i; 
    struct sockaddr_in udpServAddr;
    struct hostent *udpServer;
    socklen_t slen = sizeof(struct sockaddr_in);
    udpPacket dgram;
    uint32_t block_id, packet_id, offset, copy_size;	
    ssize_t nsent = 0, total_sent=0;
    time_t time_start, time_gap; 
    float throughput;
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
    check(ret_val < 0, "UDP socket connection%s failed: %s", 
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

    time_start = time(NULL);
    i = 0; 
    offset = 0;
    block_id = 0; 
    packet_id=0; 
    while(1) { 
        dgram.blockId = block_id;
        dgram.packetId = packet_id;
        dgram.offset = offset;
        copy_size = sizeof(data01) - offset; /* 160000*2 */
        if (copy_size > IMAGE_BLOCK_SIZE ) {
            copy_size = IMAGE_BLOCK_SIZE;
        }
        memcpy(&(dgram.image), &image[i][offset], copy_size);
        nsent = sendto(sockUDPfd, &dgram, DGRAM_SIZE, 0, (struct sockaddr *)&udpServAddr, slen); 
//      check (nsent < 0, "sentto() %s failed: %s", sockUDPfd, strerror(errno));
// ??? No nsent checking whether it effects on offset and copy_size (resent?)
        packet_id++;

    	offset += copy_size;
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
    	if (offset >= sizeof(data01)) {
       	    packet_id = 0;
            offset = 0;
            i += 1;
            if (i >= MAX_ROW) {
                i = 0;
            }
            block_id++;
        }
        usleep(delayMicroSec);
    }
} /* scan_udp_connection_handler */

void scan_dummy_read_all_handler(int fd, UsCtlHeader *buffer )
{

    /* Data */    
    int n_read, n_recv, n_left; 
    char buf_file[TCP_BUF_SIZE];

    /* Code */
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
} /* scan_dummy_read_all_handler */

void scan_ack_sent_handler(int fd, UsCtlHeader *buffer )
{

    /* Data */   
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
} /* scan_ack_sent_handler */

void scan_dummy_ack_sent_with_payload_handler(int fd, UsCtlHeader *buffer)
{

    /* data */	
    char *buf_sent; 
    int msg_len, n_sent; 

    /* Code */
    switch (buffer->opcode) {
        case USCTL_GET_VERSION:
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
} /* scan_dummy_ack_sent_with_payload_handler */

/* Utility functions */
void scan_read_one_byte_handler(int fd)
{
    /* Data */

    char buf_file[1];
    int n_recv; 

    /* Code */ 
    printf("scan_read_one_byte_handler() before TCP socket closed\n");

    n_recv = read(fd,buf_file, 1); 
    check( n_recv < 0, "Read %s failed: %s", fd, strerror(errno));
} /* scan_read_one_byte_handler */


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
} /* data_print_out */


