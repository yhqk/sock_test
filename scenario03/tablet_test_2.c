/* tablet_test_2.c
  It is used as simulated tablet to make unit test in host machine 
  for scanner_socket_threads.c  

  Use case 2: (TCP messages when UDP is receiving alll the time)
   1) Eyelife application open and "Connect to Emulator" and set with 
      following parameters. 
        - UDP IP address(tablet IP address)
        - TCP IP address(scanner IP address)
        - TCP port
        
   2) Starting scanning
        - USCTL_START_ACQ (UDP IP address and port passed to scanner)
        - wait for 5 seconds
     
   3) Motor control page
      - USCTL_SET_MOTOR_PARAMS
      - USCTL_GET_MOTOR_PARAMS
  
   4) UltraSound Control page
      - USCTL_GET_ACQU_PARAMS
         a) Load binary data: USCTL_SPI_READ
            USCTL_SPI_WRITE (?)
         b) Set ultraSound parameter: 
            - USCTL_GET_ACQU_PARAMS
            - USCTL_SET_ACQU_PARAMS
            - USCTL_GET_ACQU_PARAMS 

   5) Battery/HVC/Filter: USCTL_GET_BATTERY_STATUS
    
   6) Get version: USCTL_GET_VERSION

   7) - USCTL_SET_MOTOR_PARAMS
      - USCTL_STOP_ACQ
   User can either rerun the tablet_test_2 again or stop testing also 
   from scanner side. 
  
  * Compiling and Execution *
  $ gcc -o exec_c tablet_test_2.c -Wall -lpthread

  Usage: ./exec_c <TCP hostname> <TCP port> <UDP hostname> <UDP port>  
  $ ./exec_c 192.168.1.14 54322 192.168.1.11 554321

  * Run with debugging *
  gcc -O0 -g -o exec_c tablet_test_2.c -Wall
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
#include <pthread.h>

/* local included header file */
#define MAC_DESKTOP_BUILD
#include "scan_ctl_protocol.h"

/* Definition */
#define TCP_BUF_SIZE                 256

/* related with Start_Acqusition */
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

/* Global variable */

int trans_id = 0; 
static struct sockaddr_in udpServAddr; 
int sockUDPfd = -1; 
int udp_data_req = 0; 

/* Local function prototype definitions */
/* UDP thread */
void *udp_server_handler();

void tablet_startAcqIn_handler(struct sockaddr *tcp_sAddr, struct sockaddr *udp_sAddr); 
void tablet_dummy_in_handler(struct sockaddr *tcp_sAddr, uint32_t opcode); 
void tablet_recv_ack_handler(int tcp_fd); 
/* trace */
void data_print_out(int length, char buffer[]); 

/* check functions */    
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
    /* TCP data */
    int tcpPortNo;  
    struct sockaddr_in tcpServAddr;
    /* UDP data */
    int udpPortNo; 
    /* thread */
    pthread_t      udp_tid; 
    pthread_attr_t thread_attr;

    /* Code */

    /* ===> step (1) <=== */
    check ( argc<4, 
           "ERROR: Only missing arguments\nUsage: %s <TCP hostname> <TCPport> <UDP hostname> <UDPport>",
           argv[0]); 

    /* set TCP socket as client */
    memset((char *) &tcpServAddr, 0,sizeof(tcpServAddr));  
    tcpServAddr.sin_family = AF_INET;
    tcpServAddr.sin_addr.s_addr = inet_addr(argv[1]);
    check( tcpServAddr.sin_addr.s_addr == INADDR_NONE, 
           "Bad TCP address %s", argv[1]);
    tcpPortNo = atoi(argv[2]);
    tcpServAddr.sin_port = htons(tcpPortNo);

     /* set UDP socket as server */
    memset((char *) &udpServAddr, 0,sizeof(udpServAddr));
    udpServAddr.sin_family = AF_INET;
    udpServAddr.sin_addr.s_addr = inet_addr(argv[3]);
    check( udpServAddr.sin_addr.s_addr == INADDR_NONE, 
           "Bad UDP address %s", argv[3]);
    udpPortNo = atoi(argv[4]);
    udpServAddr.sin_port = htons(udpPortNo);
    printf("UDP socket as server with IP as %s:%d\n", 
           inet_ntoa(udpServAddr.sin_addr), 
           ntohs(udpServAddr.sin_port)); 

    /* ===> step (2) <=== */
    /* Send StartAcquisition via TCP socket */	
    printf("\n ===> step (2): USCTL_START_ACQ open UDP data receiving <===\n");   
    tablet_startAcqIn_handler((struct sockaddr *)&tcpServAddr, (struct sockaddr *)&udpServAddr);
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr,PTHREAD_CREATE_DETACHED); 
    pthread_create(&udp_tid, &thread_attr, udp_server_handler, NULL);    
    sleep(15); /* UDP thread is receiving data */

    /* ===> step (4a) <=== */
    printf("\n ===> step (4a): USCTL_SPI_READ/WRITE <===\n");   
    tablet_dummy_in_handler((struct sockaddr *)&tcpServAddr, USCTL_SPI_READ);
    tablet_dummy_in_handler((struct sockaddr *)&tcpServAddr, USCTL_SPI_WRITE);    
    tablet_dummy_in_handler((struct sockaddr *)&tcpServAddr, USCTL_SPI_READ);
    sleep(15); /* UDP thread is receiving data */


    /* ===> step (4b) <=== */
    printf("\n ===> step (4b): USCTL_GET/SET_ACQU_PARAMS <===\n");   
    tablet_dummy_in_handler((struct sockaddr *)&tcpServAddr, USCTL_GET_ACQU_PARAMS);
    tablet_dummy_in_handler((struct sockaddr *)&tcpServAddr, USCTL_SET_ACQU_PARAMS);    
    tablet_dummy_in_handler((struct sockaddr *)&tcpServAddr, USCTL_GET_ACQU_PARAMS);
    sleep(15); /* UDP thread is receiving data */
 
    /* ===> step (5) <=== */
    printf("\n ===> step (5): USCTL_GET_BATTERY_STATUS <===\n");   
    tablet_dummy_in_handler((struct sockaddr *)&tcpServAddr, USCTL_GET_BATTERY_STATUS);
    sleep(15); /* UDP thread is receiving data */

    /* ===> step (6) <=== */
    printf("\n ===> step (6): USCTL_GET_VERSION <===\n");   
    tablet_dummy_in_handler((struct sockaddr *)&tcpServAddr, USCTL_GET_VERSION);
    sleep(15); /* UDP thread is receiving data */

    /* ===> step (7) <=== */
    printf("\n ===> step (7): USCTL_STOP_ACQ <===\n");   
    tablet_dummy_in_handler((struct sockaddr *)&tcpServAddr, USCTL_SET_MOTOR_PARAMS);    
    tablet_dummy_in_handler((struct sockaddr *)&tcpServAddr, USCTL_STOP_ACQ);    
    printf("\n ===> tablet_test_2.c is done! <===\n");     
    printf(" ===> Ready to start next TC or stop scanner side by Ctrl-C <===\n\n"); 
    
    return(0); 
}

/* Local Function  */

void * udp_server_handler()
{
    /* Data */

    socklen_t slen = sizeof(struct sockaddr_in);

    char udp_buf_recv[DGRAM_SIZE];
    int ret_val, a = 1, i = 0;
    int nreceived = 0, temp_n=0;  	
    time_t time_start= 0, time_gap= 0; 
    float throughput;
  
    /* Code */

    time_start = time(NULL); /* seconds */
    while (1) {
        if ( udp_data_req == 1 ) {
            if ( sockUDPfd == -1 ) {
                sockUDPfd = socket(AF_INET, SOCK_DGRAM, 0);
                check(sockUDPfd < 0, "UDP socket create %s failed: %s", 
                sockUDPfd, strerror(errno));
                setsockopt(sockUDPfd, SOL_SOCKET, SO_REUSEADDR, &a, sizeof(int));
                ret_val = bind(sockUDPfd, (struct sockaddr *) &udpServAddr, 
                               sizeof(udpServAddr)); 
                check (ret_val < 0, "internet UDP socket binding%s failed: %s", 
                       sockUDPfd, strerror(errno));
                printf("udp_server_handler/Starting to receive image from UDP\n"); 
            }
            else {
                /* UDP */
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
                    i++; 	
                } /* TIME_GAP */
            } /* else */
        } /* udp_data_req == 1*/
    } /* while */ 
        return NULL;
} /* udp_server_handler */

void tablet_startAcqIn_handler(struct sockaddr *tcp_sAddr, struct sockaddr *udp_sAddr)
{

    /* Data */
    UsCtlStartAcqIn tcpMsg_startAcq;
    int buf_len, data_len, retval; 
    int tcp_fd; 

    /* Code */
    tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    check(tcp_fd < 0, "TCP client create %s failed: %s", tcp_fd, strerror(errno));
    retval = connect(tcp_fd, tcp_sAddr, sizeof(struct sockaddr)); 
    check (retval < 0, "TCP client connect() failed: %s", 
           strerror(errno));    
    printf("\nConnect to TCP IPaddress %s:%d with fd=%d \n", 
           inet_ntoa(((struct sockaddr_in *)tcp_sAddr)->sin_addr), 
           ntohs(((struct sockaddr_in *)tcp_sAddr)->sin_port), tcp_fd); 

    /* build and send Start Acquisition */
    buf_len = sizeof(UsCtlStartAcqIn); 
    bzero( (void *)&tcpMsg_startAcq, buf_len);	
    tcpMsg_startAcq.hdr.id = trans_id;
    trans_id++; 
    tcpMsg_startAcq.hdr.opcode = USCTL_START_ACQ ;  	
    tcpMsg_startAcq.hdr.length = sizeof(UsCtlStartAcqIn)- sizeof(UsCtlHeader); 
    tcpMsg_startAcq.ipAddress = htonl(((struct sockaddr_in *)udp_sAddr)->sin_addr.s_addr); 
    tcpMsg_startAcq.port = htons(((struct sockaddr_in *)udp_sAddr)->sin_port);

    tcpMsg_startAcq.width = IMAGE_WIDTH; 
    tcpMsg_startAcq.height = IMAGE_HEIGHT; 
    tcpMsg_startAcq.bytesPerPacket = SIZE_OF_IMAGE_DATA_PAYLOAD;
    tcpMsg_startAcq.fps = IMAGE_FPS;  
    data_len = write(tcp_fd, &tcpMsg_startAcq, buf_len);
    printf("tablet_startAcq_handler(): ->scanner msg_len=%d from TCP client %d\n", 
        data_len, tcp_fd);
    printf("Tablet UDP/ipAddress:0x%x, port:0x%x\n", tcpMsg_startAcq.ipAddress, tcpMsg_startAcq.port );
    data_print_out(data_len, (char *)&tcpMsg_startAcq);
      
    tablet_recv_ack_handler(tcp_fd); 
    udp_data_req = 1; 
} /* tablet_startAcqIn_handler */

void tablet_dummy_in_handler(struct sockaddr *tcp_sAddr, uint32_t opcode)
{
    /* data */	
    char *buf_sent; 
    int msg_len = 0, n_sent; 
    int retval; 
    int tcp_fd; 

    /* Code */
    switch (opcode) {
        case USCTL_GET_VERSION:
            msg_len = sizeof(UsCtlGetVersionIn); 
	    break;

        case USCTL_SET_MOTOR_PARAMS:
            msg_len = sizeof(UsCtlSetMotorIn); 
	    break;
 
        case USCTL_GET_MOTOR_PARAMS:
            msg_len = sizeof(UsCtlGetMotorIn); 
	    break;

        case USCTL_SET_ACQU_PARAMS:
            msg_len = sizeof(UsCtlSetAcqParamIn); 
	    break;

        case USCTL_GET_ACQU_PARAMS:
            msg_len = sizeof(UsCtlGetAcqParamIn); 
	    break;

        case USCTL_GET_BATTERY_STATUS:
            msg_len = sizeof(UsCtlGetBatteryStatusIn); 
	    break;

        case USCTL_SPI_WRITE:
            msg_len = sizeof(UsCtlSPIWriteIn); 
        break; 

        case USCTL_SPI_READ:
            msg_len = sizeof(UsCtlSPIReadIn); 
        break; 

        case USCTL_STOP_ACQ:
            msg_len = sizeof(UsCtlStopAcqIn); 
            udp_data_req = 0; 
            close(sockUDPfd); 
            sockUDPfd = -1; 
        break; 	
      
        case USCTL_FIRMWARE_UPDATE:
	     msg_len = sizeof(UsCtlFirmwareUpdateIn); 
        break;
  
        default:
            printf("tablet_dummy_in_handler: unknown opcode %x:\n", opcode);
        break;
    }
    
    if ( msg_len > 0 ) {
        tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
        check(tcp_fd < 0, "TCP client create %s failed: %s", tcp_fd, strerror(errno));
        retval = connect(tcp_fd, tcp_sAddr, sizeof(struct sockaddr)); 
        check (retval < 0, "TCP client connect() failed: %s", strerror(errno));    
        printf("\nConnect TCP server with IP address %s:%d, as client %d\n", 
               inet_ntoa(((struct sockaddr_in *)tcp_sAddr)->sin_addr), 
               ntohs(((struct sockaddr_in *)tcp_sAddr)->sin_port), 
               tcp_fd); 
        buf_sent = (char *) malloc(msg_len);
        bzero( (void *)buf_sent, msg_len);
        ((UsCtlHeader *)buf_sent)->id = trans_id;
        trans_id++; 
        ((UsCtlHeader *)buf_sent)->opcode = opcode;  	
        ((UsCtlHeader *)buf_sent)->length = msg_len-sizeof(UsCtlHeader);
        n_sent = write(tcp_fd, buf_sent, msg_len);
        check( n_sent < 0, "Write %s failed: %s", tcp_fd, strerror(errno));
        printf("tablet_dummy_in_handler(): ->scanner opCode=%d data_length=%d:", opcode, n_sent);
        data_print_out(n_sent, buf_sent);
        free(buf_sent);
        tablet_recv_ack_handler(tcp_fd); 
    }
} /* tablet_dummy_in_handler() */

void tablet_recv_ack_handler(int tcp_fd)
{

    /* Data */	 
    int msg_len = 0; 
    int n_recv; 
    char tcp_buf_recv[TCP_BUF_SIZE];
    char tcp_buf_sent_one[1];

    /* Code */
    n_recv = read(tcp_fd, tcp_buf_recv, TCP_BUF_SIZE); 
    check( n_recv < 0, "Read %s failed: %s", tcp_fd, strerror(errno));
    printf("-> ACK as id:%d; opCode:%d; payload length:%d; msg length:%d", 
	   ((UsCtlHeader *)tcp_buf_recv)->id, 
	   ((UsCtlHeader *)tcp_buf_recv)->opcode, 
	   ((UsCtlHeader *)tcp_buf_recv)->length, 
	   n_recv);
    data_print_out(n_recv, tcp_buf_recv);

    msg_len = sizeof(tcp_buf_sent_one); 
    bzero( (void *)&tcp_buf_sent_one, msg_len);	
    write(tcp_fd, &tcp_buf_sent_one, msg_len);
    printf("tablet_recv_ack_handler: send one byte before close TCP socket %d\n", 
	   tcp_fd);
    close(tcp_fd);
} /* tablet_recv_ack_handler */

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
} /* data_print_out */

