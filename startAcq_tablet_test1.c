/* startAcq_tablet_test1.c
  - Tablet connects TCP as client to server; 
  - Tablet sends Start_Acquisition with UDP IP address/port; 
  - Tablet opens UDP connection as server; 
  - Tablet waits for receiving UDP package. 
  => this version don't handle the Stop_Acquisition

  * Compiling and Execution *
  $ gcc -o exec_c startAcq_tablet_test1.c -Wall
  Usage: ./exec_c <TCP hostname> <TCP port> <UDP hostname> <UDP port>  

  $ ./exec_c localhost 5000 localhost 5001
  or
  $ ./exec_c 192.168.1.46 5000 192.168.1.87 5001

  * Debugging *
  gcc -O0 -g -o exec_c startAcq_tablet_test1.c -Wall
  gdb --args ./exec_c 192.168.1.14 54322 192.168.1.11 54321
    
 */

/* Include header files */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 
#include <unistd.h> 
#include <errno.h>
#include <stdarg.h>
#include <time.h>

/* local included header file */
#define MAC_DESKTOP_BUILD
#include "scan_ctl_protocol.h"

/* Definition */
#define TCP_BUF_SIZE                 256

/* related with Start_Acquition */
#define IMAGE_WIDTH                  1000
#define IMAGE_HEIGHT                 160
#define SIZE_OF_IMAGE_DATA_PAYLOAD   1200
#define IMAGE_FPS                    10

#define IMAGE_BLOCK_SIZE             1448
#define DGRAM_SIZE                   1472
#define TIME_GAP                     10 

/* UDP data structure */
typedef struct _udpPacket {
  uint32_t blockId;
  uint32_t packetId;
  uint16_t spare1;
  uint16_t spare2;
  uint32_t offset;
  uint64_t timestamp;
  uint16_t image[IMAGE_BLOCK_SIZE];
} udpPacket;

/* Local function prototype definitions */
void data_print_out(int length, char buffer[]); 
void tablet_startAcqIn_handler(int fd, uint32_t ip_addr, int port); 

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

/* Main function */
int main(int argc, char *argv[])
{
    /* Data  */
    int sockTCPfd, tcpPortNo, sockUDPfd, udpPortNo; 
    struct sockaddr_in tcpServAddr;
    struct sockaddr_in udpServAddr;
    struct hostent *udpServer;
    struct hostent *tcpServer;      
    char tcp_buf_recv[TCP_BUF_SIZE];
    char udp_buf_recv[DGRAM_SIZE];
    socklen_t slen = sizeof(struct sockaddr_in);
    int n=1, ret_val, a = 1, i = 0;
    int nreceived = 0, temp_n=0;  	
    time_t time_start= 0, time_gap= 0; 
    float throughput;

    /* Code */

    check ( argc<4, "ERROR: Only missing arguments\nUsage: %s <TCP hostname> <TCPport> <UDP hostname> <UDPport>", argv[0]); 
	
    sockTCPfd = socket(AF_INET, SOCK_STREAM, 0);
    check(sockTCPfd < 0, "TCP client create %s failed: %s", sockTCPfd, strerror(errno));

    /* check the hostname */	
    tcpServer = gethostbyname(argv[1]);
    if (tcpServer == NULL) {
	   fprintf(stderr,"ERROR, no such host for TCP\n");
	   exit(0);
    }
    udpServer = gethostbyname(argv[3]);
    if (udpServer == NULL) {
	   fprintf(stderr,"ERROR, no such host for UDP socket in tablet side\n");
	   exit(0);
    }

    /* set TCP socket as client */
    memset((char *) &tcpServAddr, 0,sizeof(tcpServAddr));  
    tcpServAddr.sin_family = AF_INET;
    tcpServAddr.sin_addr.s_addr = inet_addr(argv[1]);
    tcpPortNo = atoi(argv[2]);
    tcpServAddr.sin_port = htons(tcpPortNo);
    ret_val = connect(sockTCPfd,(struct sockaddr *)&tcpServAddr, 
                      sizeof(tcpServAddr)); 
    check (ret_val < 0, "TCP client connect() failed: %s", 
           strerror(errno));    
    printf("Connect TCP as client from %s:%d\n", 
           inet_ntoa(tcpServAddr.sin_addr), 
           ntohs(tcpServAddr.sin_port)); 

    /* set UDP socket as server */
    sockUDPfd = socket(AF_INET, SOCK_DGRAM, 0);
    check(sockUDPfd < 0, "UDP socket create %s failed: %s", 
          sockUDPfd, strerror(errno));
    memset((char *) &udpServAddr, 0,sizeof(udpServAddr));
    udpServAddr.sin_family = AF_INET;
    udpServAddr.sin_addr.s_addr = inet_addr(argv[3]);
    udpPortNo = atoi(argv[4]);
    udpServAddr.sin_port = htons(udpPortNo);
    setsockopt(sockUDPfd, SOL_SOCKET, SO_REUSEADDR, &a, sizeof(int));
    ret_val = bind(sockUDPfd, (struct sockaddr *) &udpServAddr, 
                   sizeof(udpServAddr)); 
    check (ret_val < 0, "internet UDP socket binding%s failed: %s", 
           sockUDPfd, strerror(errno));
    printf("Open UDP socket as server %s:%d\n", 
           inet_ntoa(udpServAddr.sin_addr), 
           ntohs(udpServAddr.sin_port)); 

    /* Send StartAcquisition via TCP socket */	
    tablet_startAcqIn_handler(sockTCPfd, udpServAddr.sin_addr.s_addr, 
                              udpPortNo);
    n = read(sockTCPfd,tcp_buf_recv, TCP_BUF_SIZE);
    data_print_out(n, tcp_buf_recv);

    printf("Starting to receive image from UDP:\n"); 
    nreceived = recvfrom(sockUDPfd, udp_buf_recv, DGRAM_SIZE, 0, 
                         (struct sockaddr *)&udpServAddr, &slen);
    check( nreceived < 0, "Recieving %s failed: %s", 
          sockUDPfd, strerror(errno));
    
    time_start = time(NULL); /* seconds */
    printf("After received from %s:%d\n", 
        inet_ntoa(udpServAddr.sin_addr), 
        ntohs(udpServAddr.sin_port)); 
    
    time_start = time(NULL); /* seconds */
    while (1) {
        bzero(udp_buf_recv, sizeof(udp_buf_recv));
        nreceived = recvfrom(sockUDPfd, udp_buf_recv, DGRAM_SIZE, 0, 
                             (struct sockaddr *)&udpServAddr, &slen); 
        check( nreceived < 0, "Recieving %s failed: %s", 
              sockUDPfd, strerror(errno));
	
        temp_n+=nreceived; 	
        time_gap = time(NULL) - time_start; 
        if ( time_gap >= TIME_GAP ) { 
            printf("\nblockID:%05d; packetId:%3d", 
	        ((udpPacket *)udp_buf_recv)->blockId, 
                ((udpPacket *)udp_buf_recv)->packetId); 
            data_print_out(32, &udp_buf_recv[24]); 	    
            throughput = ((double)temp_n/1000000)/time_gap; 
            printf("%8d recvfrom throughput %.3f MB/s = %.2f Mbits/s, time gap %d sec\n", 
	            	i, throughput, throughput*8, (int)time_gap);  
            temp_n = 0;
            time_start = time(NULL);		
        }
        i++; /* loop counter for recevform */	
    }	
    return(0); 
}

/* Local Function  */
void tablet_startAcqIn_handler(int tcp_fd, uint32_t ip_addr, int port)
{

    /* Data */
    UsCtlStartAcqIn tcpMsg_startAcq;
    int buf_len, data_len, n_recv; 
    char tcp_buf_recv[TCP_BUF_SIZE];

    /* Code */
    /* build and send Start Acquisition */
    buf_len = sizeof(UsCtlStartAcqIn); 
    bzero( (void *)&tcpMsg_startAcq, buf_len);	
    tcpMsg_startAcq.hdr.opcode = USCTL_START_ACQ ;  	
    tcpMsg_startAcq.hdr.length = sizeof(UsCtlStartAcqIn)- sizeof(UsCtlHeader); 
    tcpMsg_startAcq.ipAddress = htonl(ip_addr); 
    tcpMsg_startAcq.port = (uint32_t)port;
    tcpMsg_startAcq.width = IMAGE_WIDTH; 
    tcpMsg_startAcq.height = IMAGE_HEIGHT; 
    tcpMsg_startAcq.bytesPerPacket = SIZE_OF_IMAGE_DATA_PAYLOAD;
    tcpMsg_startAcq.fps = IMAGE_FPS;  
    data_len = write(tcp_fd, &tcpMsg_startAcq, buf_len);
    printf("tablet_startAcq_handler(): tablet sends startAcq with msg length %d to client %d\n", 
        data_len, tcp_fd);
    printf("Tablet UDP: %x port %x\n", tcpMsg_startAcq.ipAddress, tcpMsg_startAcq.port );
      
    /* Read ACK or NACK */
    n_recv = read(tcp_fd,tcp_buf_recv, TCP_BUF_SIZE); 
    check( n_recv < 0, "Read %s failed: %s", tcp_fd, strerror(errno));
    printf("id: 0x%2x; opcode: 0x%2x; payload length: %d; msg length: %d", 
         ((UsCtlHeader *)tcp_buf_recv)->id, 
         ((UsCtlHeader *)tcp_buf_recv)->opcode, 
         ((UsCtlHeader *)tcp_buf_recv)->length, 
         n_recv);
    data_print_out(n_recv, tcp_buf_recv);
    close(tcp_fd);
}

/* Utility Function */
void data_print_out(int length, char buffer[]){

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

