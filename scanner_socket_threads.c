/* startAcq_scanner_test2.c   
  - Scanner starts TCP server; 
  - Wait clients sending Start_Acquisition with UDP IP address/port; 
  - Reply all messages from TCP with ACK (OK)
  - Connect UDP to tablet as client; 
  - Send image via UDP socket as loop. 
  => this version don't handle the Stop_Acquisition
    
  * Compiling and Execution *
  $ gcc -o exec_s startAcq_scanner_test2.c -Wall -Wextra -lpthread -I${PWD}/images/
  $ ./exec_s 54322 0 500

  * Debugging *
  gcc -O0 -g -o exec_s startAcq_scanner_test2.c -Wall -Wextra -lpthread -I${PWD}/images/
  gdb --args  ./exec_s 54322 0 500

  There can be several clients from different termials. 
 */

/* Header files */
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
#include <memory.h>

/* Local header files */
#define MAC_DESKTOP_BUILD
#include "scan_ctl_protocol.h"

#include "data_array_image01.h"

/* Definitions */ 
/* epoll related parameters */
#define MAXEVENTS         64
#define BACKLOG           5 

/* TCP socket */     
#define TCP_BUF_HDR_SIZE  12
#define TCP_BUF_SIZE      256

/* UDP socket */
#define DGRAM_SIZE        1472
#define IMAGE_BLOCK_SIZE  1448
#define TIME_GAP          10

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

typedef struct ipAddress {
  uint32_t ip_address;
  uint32_t udp_port;
} ipAddress;

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

void *scan_udp_connection_handler(void *argv); 
void scan_udp_data_sent_handler(int udp_fd); 

void data_print_out(int length, char buffer[]); 
void scan_read_one_byte_handler(int fd); 

/* global variables */
int startAcqStatus = 0;      /* 0 is OK; 1 already_run, 2 not supported */
int delayMicroSec = 500;     /* delay for sending each UDP packet */
int sockUDPfd = -1;             
int sockTCPfd = -1; 
int epoll_fd; 

/* Main Function */
int main(int argc, char *argv[])
{
    /* Data */
    int new_sockTCPfd, portno, clilen;
    int ret_val, a=1;
    struct sockaddr_in addr_in, cli_addr;
    struct epoll_event event;    
    pthread_t tcp_thread;
    pthread_attr_t thread_tcp_attr;

    /* Code */
    check(argc < 4, 
          "ERROR: Only missing argument\nUsage: %s <port> <scan status> <delay time microsecond>",
          argv[0]);

    startAcqStatus = atoi(argv[2]);   
    delayMicroSec = atoi(argv[3]); 

    /* create thread with epoll */
    pthread_attr_init(&thread_tcp_attr);
    pthread_attr_setdetachstate(&thread_tcp_attr,PTHREAD_CREATE_DETACHED);

    epoll_fd = epoll_create(MAXEVENTS);
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

    ret_val = pthread_create(&tcp_thread, &thread_tcp_attr, tcp_client_handler, &epoll_fd);
    if (!ret_val) {
        pthread_join(tcp_thread, NULL);
        }
	
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
        epoll_ctl (epoll_fd, EPOLL_CTL_ADD, new_sockTCPfd, &event);
        printf("===> main/epfd %d, sockTCPfd %d\n", new_sockTCPfd, sockTCPfd); 
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
    int n_recv, ret; 
    char buf_file[TCP_BUF_SIZE];
    struct sockaddr_in udpAddr;
    void *s_addr[2]; 
    pthread_t udp_thread;   
    
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
        s_addr[0] = malloc(INET_ADDRSTRLEN);
        s_addr[0] = inet_ntoa(udpAddr.sin_addr);
        s_addr[1] = malloc(15);
        sprintf(s_addr[1], "%d", tcpMsg_startAcq.port );
        printf("received UDP IP address: %s/0x%x; port:%s\n", 
               (char *)s_addr[0], tcpMsg_startAcq.ipAddress,
               (char *)s_addr[1]);
        ret = pthread_create(&udp_thread, NULL, 
                       scan_udp_connection_handler, s_addr);
        if (!ret) {
            pthread_join(udp_thread, NULL);
            }
    }
    else
        printf("scan_start_acq_in_handler(): Error/length is different wwith recieved");
} /* scan_start_acq_in_handler */

void *scan_udp_connection_handler(void *argv)
{
    /* Data */  
    int ret_val; 
    struct sockaddr_in udpServAddr;
    struct hostent *udpServer;
    socklen_t slen = sizeof(struct sockaddr_in);
    char **args = (char **)argv;

    udpPacket dgram;
    uint32_t block_id=0, packet_id=0, offset=0, copy_size;	
    ssize_t nsent = 0, total_sent=0;
    time_t time_start, time_gap; 
    float throughput;
 
    /* Code */ 
    sockUDPfd = socket(AF_INET, SOCK_DGRAM, 0);
    check(sockUDPfd < 0, "Scanner: UDP socket create %s failed: %s", 
          sockUDPfd, strerror(errno));
    udpServer = gethostbyname(args[0]);
    if (udpServer == NULL) {
        fprintf(stderr,"ERROR, no such UDP host\n");
        exit(0);
    }
   
    memset((char *) &udpServAddr, 0,sizeof(udpServAddr));  
    udpServAddr.sin_family = AF_INET;
    udpServAddr.sin_addr.s_addr = inet_addr(args[0]);
    udpServAddr.sin_port = htons(atoi(args[1]));
    ret_val = connect(sockUDPfd,(struct sockaddr *)&udpServAddr, sizeof(udpServAddr)); 
    check(ret_val < 0, "UDP socket connection%s failed: %s", 
          sockUDPfd, strerror(errno));
    printf("Connect UDP socket as client %s: %d\n", 
           inet_ntoa(udpServAddr.sin_addr), 
           ntohs(udpServAddr.sin_port)); 	

    time_start = time(NULL); 
    while(sockUDPfd) { 
        dgram.blockId = block_id;
        dgram.packetId = packet_id;
        dgram.offset = offset;
        copy_size = sizeof(data01) - offset; /* 160000*2 */
        if (copy_size > IMAGE_BLOCK_SIZE ) {
            copy_size = IMAGE_BLOCK_SIZE;
        }

        memcpy(&(dgram.image), &(data01[offset/2]), copy_size); 
        nsent = sendto(sockUDPfd, &dgram, DGRAM_SIZE, 0, 
                       (struct sockaddr *)&udpServAddr, slen); 
// ??? reset  check (nsent < 0, "sentto() %s failed: %s", sockUDPfd, strerror(errno));
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
      	    block_id++;
    	}
       /*	usleep(delayMicroSec); */

    	int new_sockTCPfd, clilen;
    	struct sockaddr_in cli_addr;
        //set of socket descriptors
        fd_set active_fd_set;
        struct timeval tv;
        int retval;
        struct epoll_event event;  
    char buffer[TCP_BUF_HDR_SIZE];
    int i;
    ssize_t data_len=1;

/* wrong stuck */
        int n = epoll_wait(epoll_fd,ready,MAXEVENTS,-1); 
        for (i = 0; i < n; i++) {
           memset(buffer,0,TCP_BUF_HDR_SIZE);
            data_len = read(ready[i].data.fd,buffer,TCP_BUF_HDR_SIZE);
            /* not then removed from ready list*/
            if (data_len <= 0) {
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, ready[i].data.fd, NULL);
                close(ready[i].data.fd);
                printf("epoll event %d terminated\n",ready[i].data.fd); 
            }	
            else {
                printf("Scanner/TCP receives data with length %d from event %d\n", 
                       (int)data_len, ready[i].data.fd);
                scan_msg_handler(ready[i].data.fd, (UsCtlHeader *)buffer); 
            } 
        } /* for loop */

#if 0
        listen(sockTCPfd,BACKLOG);
        /* Initialize the set of active sockets. */
        FD_ZERO (&active_fd_set);
        FD_SET (sockTCPfd, &active_fd_set);

        /* Wait up to five seconds. */
        tv.tv_sec = 0;
        tv.tv_usec = delayMicroSec;

        retval = select(7, &active_fd_set, NULL, NULL, &tv);
        /* Don't rely on the value of tv now! */

        if (retval == -1)
            perror("select()");
        else if (retval) {
            printf("Data is available now.\n");
            clilen = sizeof(cli_addr);
        new_sockTCPfd = accept(sockTCPfd,  
                               (struct sockaddr *) &cli_addr, 
                               (socklen_t *)&clilen);
        check (new_sockTCPfd < 0, 
               "TCP socket accept %s failed: %s", sockTCPfd, 
               strerror(errno));	
        event.data.fd = new_sockTCPfd;
        event.events = EPOLLIN;
        epoll_ctl (epoll_fd, EPOLL_CTL_ADD, new_sockTCPfd, &event);
        printf("===> scan_udp_connection_handler/epfd %d\n", new_sockTCPfd);
        }
/*
        else
            printf("No data within 500 useconds.\n");
*/
#endif 
    }
    //close(sockUDPfd); 
    free(argv); 	
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


