/* scanner_socket_threads.c   
  This is an interface to handle data between TCP and UDP sockets. 
  Both TCP and UDP have its own threads. 
  In the current version, scanner can only handle one client, 
  not multiple tablets in the same time.
    
  * Compiling and Execution *
  $ gcc -o exec_s scanner_socket_threads.c -Wall -Wextra -lpthread -I${PWD}/images/
  $ ./exec_s 54322 0 200

  * Debugging *
  gcc -O0 -g -o exec_s scanner_socket_threads.c -Wall -Wextra -lpthread -I${PWD}/images/
  gdb --args  ./exec_s 54322 0 100

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
#include <sys/time.h>

/* Local header files */
#define MAC_DESKTOP_BUILD    /* removed compiling error in following header file */
#include "scan_ctl_protocol.h"

#include "scanner_socket_threads.h"

/* image data header files based on version Sep 24 2015 (001.txt) */
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

/* Structure defintion */
struct epoll_event *ready; 

enum sstate {                    /* socket data states */
     S_ACTIVE,                   /* socket sending data */
     S_TERMINATED                /* socket stop sending data */
};
 
typedef struct _socketThread {       
     pthread_t       tid;         /* ID of this thread */
     int             sfd;         /* socket fd */
     enum sstate     state;       /* socket state (UDP) */
} socketThread;

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
/* functions for threads */
void *tcp_client_handler(void *arg);
void *udp_send_handler();
/* Handle TCP data */
void scan_msg_handler(int fd, UsCtlHeader *buffer ); 
void scan_start_acq_in_handler(int fd, UsCtlHeader *buffer); 
void scan_ack_sent_handler(int fd, UsCtlHeader *buffer); 
void scan_dummy_read_all_handler(int fd, UsCtlHeader *buffer); 
void scan_dummy_ack_sent_with_payload_handler(int fd, UsCtlHeader *buffer); 
void scan_read_one_byte_handler(int fd); 
/* trace */
void data_print_out(int length, char buffer[]); 

/* global variables */
static int startAcqStatus = 0;      /* 0 is OK; 1 already_run, 2 not supported */
static struct sockaddr_in udpAddr; 
socketThread udp_thread;  
int delayInMS = 0;

/* Main Function */
int main(int argc, char *argv[])
{
    /* Data */
    int sockTCPfd, new_sockTCPfd, portno, clilen, epfd;
    int ret_val, a=1;
    struct sockaddr_in tcp_addr, cli_addr;
    pthread_t      tcp_tid; 
    pthread_attr_t thread_attr;
    struct epoll_event event;  

    /* Code */
    check(argc < 4, 
          "ERROR: Only missing argument\nUsage: %s <port> <scan status> <delay time microsecond>",
          argv[0]);

    startAcqStatus = atoi(argv[2]);       
    delayInMS = atoi(argv[3]);

    /* create thread with epoll */
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr,PTHREAD_CREATE_DETACHED); 
    epfd = epoll_create(MAXEVENTS);
    ready = (struct epoll_event*)calloc(MAXEVENTS,sizeof(event));

    /* Create TCP socket */
    sockTCPfd = socket(AF_INET, SOCK_STREAM, 0);
    check(sockTCPfd < 0, "TCP server create %s failed: %s", 
          sockTCPfd, strerror(errno));
    memset((char *) &tcp_addr ,0,sizeof(tcp_addr));
    portno = atoi(argv[1]);
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    tcp_addr.sin_port = htons(portno);
    setsockopt(sockTCPfd, SOL_SOCKET, SO_REUSEADDR, &a, sizeof(int));
    ret_val = bind(sockTCPfd, (struct sockaddr *) &tcp_addr, sizeof(tcp_addr)); 
    check (ret_val < 0, "TCP socket bind %s failed: %s", 
           sockTCPfd, strerror(errno));

    /* Create threads */
    pthread_create(&tcp_tid, &thread_attr, tcp_client_handler, &epfd);
    udp_thread.state = S_TERMINATED; 
    udp_thread.sfd = -1; 
    pthread_create(&udp_thread.tid, &thread_attr, udp_send_handler, NULL);

    printf("Scanner/main(): listen TCP socket\n");
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
/* TCP thread */
void * tcp_client_handler(void *arg) 
{
    /* Data */
    int epfd = *((int *)arg);
    char buffer[TCP_BUF_HDR_SIZE];
    int i,n ;
    ssize_t data_len=1;
    
    /* Code */
    while(1) {
	/* Specifying a timeout of -1 causes epoll_wait() to block 
	* indefinitely, while specifying a timeout equal to zero 
	* cause epoll_wait() to return immediately, even if no 
	* events are available.  (timeout argument currently as -1) 
	*/    
	n = epoll_wait(epfd,ready,MAXEVENTS,-1);    
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
            scan_msg_handler(ready[i].data.fd, (UsCtlHeader *)buffer);                  
            } 
        } /* for loop */     
    } /* while loop */
    return NULL;
} /* tcp_client_handler */

/* UDP thread */
void * udp_send_handler()
{
    /* Data */  
    int i=0; 
    socklen_t slen = sizeof(struct sockaddr_in);
    udpPacket dgram;
    uint32_t block_id, packet_id, offset, copy_size;	
    ssize_t nsent = 0, total_sent=0;
    time_t time_start, time_gap; 
    float throughput;
    char *image[10]; 

    /* Code */ 
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

    for(;;) {
        if ( udp_thread.state == S_ACTIVE ) {
            dgram.blockId = block_id;
            dgram.packetId = packet_id;
            dgram.offset = offset;
            copy_size = sizeof(data01) - offset; /* 160000*2 */
            if (copy_size > IMAGE_BLOCK_SIZE ) {
                copy_size = IMAGE_BLOCK_SIZE;
            }
            memcpy(&(dgram.image), &image[i][offset], copy_size);
            nsent = sendto(udp_thread.sfd, &dgram, DGRAM_SIZE, 0, 
                           (struct sockaddr *)&udpAddr, slen); 
            check (nsent < 0, "sentto() %s failed: %s", udp_thread.sfd, strerror(errno));
            /* Currenly there is no nsent checking for resending.*/
            
            packet_id++;
            offset += copy_size;
            if (offset >= sizeof(data01)) {
                packet_id = 0;
                offset = 0;
                i += 1;
                if (i >= MAX_ROW) {
                    i = 0;
                }
                block_id++;
            } /* if for next picture */
             
            /* Trace for throughputs*/
            time_gap = time(NULL) - time_start;
            total_sent += nsent; 
            if ( time_gap >= TIME_GAP )  {
                printf("udp_send_handler/blockID:%05d; packetId:%3d\n", 
                       dgram.blockId, dgram.packetId);
                throughput = ((double)total_sent/1000000)/time_gap; 
                printf("udp_send_handler/average throughput/%ds: %.2f MB/s = %.2f Mbits/s <==\n", 
                       TIME_GAP, throughput, throughput*8 ); 
                time_start = time(NULL);
                total_sent = 0; 
            }
            usleep(delayInMS); /* currently set via input argument in order to get optimal value */
        } /* if S_ACTIVE (scanning is ON ) */ 
    } /* for loop */
    return NULL;
} /* udp_send_handler */

/* Functions handle TCP messages */
void scan_msg_handler(int fd, UsCtlHeader *buffer )
{   
    printf("\nTransactionId:%d; OpCode:%d; PayLoad length:%d\n", 
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
            udp_thread.state = S_TERMINATED;  
            scan_ack_sent_handler(fd, buffer); 
            close(udp_thread.sfd);
            udp_thread.sfd = -1; 
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
    int n_recv, ret_val; 
    char buf_file[TCP_BUF_SIZE];
    
    /* Code */ 
    n_recv = read(fd,buf_file, buffer->length); 
    printf("scan_start_acq_in_handler/received length:%d; PayLoad length:%d", 
           n_recv, (int)(buffer->length) );	
    data_print_out(n_recv, buf_file);
    
    if ( n_recv == (int)(buffer->length) ) {
        scan_ack_sent_handler(fd, buffer); 
        memcpy(&(tcpMsg_startAcq.ipAddress), &buf_file, n_recv );
        udpAddr.sin_addr.s_addr = htonl(tcpMsg_startAcq.ipAddress);
        udpAddr.sin_port = htons(tcpMsg_startAcq.port); /* 0xd431 54321*/
        printf("\nUsCtlStartAcqIn/UDP IP address: 0x%x; port:0x%x\n", 
               tcpMsg_startAcq.ipAddress,
               tcpMsg_startAcq.port ); 
        /* connect UDP server */
        udp_thread.sfd = socket(AF_INET, SOCK_DGRAM, 0);
        check(udp_thread.sfd < 0, "Scanner: UDP socket create %s failed: %s", 
            udp_thread.sfd, strerror(errno));
        ret_val = connect(udp_thread.sfd, (struct sockaddr *)&udpAddr, sizeof(struct sockaddr)); 
        check(ret_val < 0, "UDP socket connection%s failed: %s", 
             udp_thread.sfd, strerror(errno));
        udp_thread.state = S_ACTIVE; 
        printf("UDP client connected to IP address %s:%d, udp_thread.state=%d\n",
               inet_ntoa(udpAddr.sin_addr), 
               ntohs(udpAddr.sin_port), 
               udp_thread.state); 
    }	
    else
        printf("scan_start_acq_in_handler(): Error/buffer_length is different with recieved\n");
} /* scan_start_acq_in_handler */

void scan_dummy_read_all_handler(int fd, UsCtlHeader *buffer )
{
    /* Data */    
    int n_read, n_recv, n_left; 
    char buf_file[TCP_BUF_SIZE];

    /* Code */
    n_left = buffer->length;     
    while ( n_left > 0 ) {
        printf("scan_dummy_read_all_handler()/rest of data:"); 
        if ( n_left >= TCP_BUF_SIZE ) {
            n_read = TCP_BUF_SIZE;
        }
        else {
            n_read = n_left; 
        }
        bzero(buf_file, sizeof(buf_file)); 
        n_recv = read(fd,buf_file,n_read); 
        check( n_recv < 0, "Read %s failed: %s", fd, strerror(errno));
        data_print_out(n_recv, buf_file);
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
    printf("scan_ack_sent_handler(): Scanner->ACK to event %d:", fd);
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
    printf("scan_dummy_ack_sent_with_payload_handler(): ACK+payload(%d) to client %d:", 
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

/* Trace function */
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


