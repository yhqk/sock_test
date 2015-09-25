/* startAcq_ablet_test1.c
  TCP client to server startAcq

  Compiling and Execution
  $ gcc -o exec_c startAcq_ablet_test1.c -Wall
  Usage: ./exec_c <TCP hostname> <TCP port> <UDP hostname> <UDP port>  
  $ ./exec_c localhost 5000 localhost 5001
  or
  $ ./exec_c 192.168.1.46 5000 192.168.1.87 5001

  There can be several clients from different termials. 
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <unistd.h> 
#include <errno.h>
#include <stdarg.h>

#define MAC_DESKTOP_BUILD
#include "scan_ctl_protocol.h"

#define TCP_BUF_SIZE 256
#define DGRAM_SIZE 1472
#define IMAGE_BLOCK_SIZE 1448

uint16_t data01[] = {
    31535,32060,31798,31613,31434,32665,31631,32033,31914,32564,
    31394,31730,31320,31632,31646,31521,31436,32361,31555,31780,
    31507,32945,32049,31992,31801,33013,32068,31272,31505,33085,
    31476,31520,34520,34859,30142,30174,32037,33707,33492,35103,
    33261,23805,24041,19824,31023,49144,33209,16384,24788,41839
}; 

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

void error(char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockTCPfd, tcpPortNo, udpPortNo; 
    struct sockaddr_in tcpServAddr;
    struct sockaddr_in udpServAddr;
    struct hostent *udpServer;
    struct hostent *tcpServer;      
    char buffer_recv[TCP_BUF_SIZE];
    int n, ret_val;	

    n = 1;
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

    /* set TCP socket */
    memset((char *) &tcpServAddr, 0,sizeof(tcpServAddr));  
    tcpServAddr.sin_family = AF_INET;
    memcpy((char *)&tcpServAddr.sin_addr.s_addr,(char *)tcpServer->h_addr, 
	   tcpServer->h_length);
    tcpPortNo = atoi(argv[2]);
    tcpServAddr.sin_port = htons(tcpPortNo);
    ret_val = connect(sockTCPfd,(struct sockaddr *)&tcpServAddr, sizeof(tcpServAddr)); 
    check (ret_val < 0, "TCP client connect(): %s", strerror(errno));    

    tablet_startAcqIn_handler(sockTCPfd, tcpServAddr.sin_addr.s_addr, tcpPortNo);
	
    memset((char *) &udpServAddr, 0,sizeof(udpServAddr));  
    udpServAddr.sin_family = AF_INET;
    memcpy((char *)&udpServAddr.sin_addr.s_addr,(char *)udpServer->h_addr, 
	   udpServer->h_length);
    udpPortNo = atoi(argv[4]);
    udpServAddr.sin_port = htons(udpPortNo);
    n = read(sockTCPfd,buffer_recv, TCP_BUF_SIZE);
    data_print_out(n, buffer_recv);
    return 1; 
}

void tablet_startAcqIn_handler(int fd, uint32_t ip_addr, int port)
{

    /*uint32_t ipAddress =  3232235691; */
    UsCtlStartAcqIn tcpMsg_startAcq;
    UsCtlStartAcqIn *temp_ptr;
    int buf_len, data_len, n_recv; 
    char buf_file[256];

    /* Make message */
    buf_len = sizeof(UsCtlStartAcqIn); 
    bzero( (void *)&tcpMsg_startAcq, buf_len);	
    tcpMsg_startAcq.hdr.opcode = USCTL_START_ACQ ;  	
    tcpMsg_startAcq.hdr.length = sizeof(UsCtlStartAcqIn)- sizeof(UsCtlHeader);  /* payload as zero */
    tcpMsg_startAcq.ipAddress = ip_addr; 
    tcpMsg_startAcq.port = port;
    tcpMsg_startAcq.width = 160; 
    tcpMsg_startAcq.height = 1000; 
    tcpMsg_startAcq.bytesPerPacket = IMAGE_BLOCK_SIZE;

    data_len = write(fd, &tcpMsg_startAcq, buf_len);
    printf("tablet_startAcq_handler(): tablet sends startAcq with msg length %d to client %d\n", 
        data_len, fd);

	printf("\nSending TCP data01 with length: %d\n", data_len);
	temp_ptr = &tcpMsg_startAcq; 
	printf("opco: %x\n", temp_ptr->hdr.opcode);
        printf("Tablet IP address: %x port %x\n", tcpMsg_startAcq.ipAddress, tcpMsg_startAcq.port );
        printf("\n");

    n_recv = read(fd,buf_file, 256); 
    check( n_recv < 0, "Read %s failed: %s", fd, strerror(errno));
    printf("id: %x; opcode: %x; payload length: %x", 
         ((UsCtlHeader *)buf_file)->id, 
         ((UsCtlHeader *)buf_file)->opcode, 
         ((UsCtlHeader *)buf_file)->length);
   data_print_out(n_recv, buf_file);
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

