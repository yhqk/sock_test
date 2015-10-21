/* udp_multi_images.c / Yuhong Qiu-Kaikkonen

   It sends 11 different image[160*1000] defined in local header files over 
   UDP socket from Server to client. Throughput is also caculated.  
   The 1st image is set as zero. 
   
   arguement: <IP address> <port> <delay time microsecond> 
   
   $ gcc -o exec_a udp_multi_images.c -Wall -I${PWD}/images/
   $ ./exec_a 192.168.1.87 54321 200 

*/

/* header files */
#include <string.h>  
#include <stdlib.h>  //exit
#include <stdint.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>

/* local header files */
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

/* constants */
#define DGRAM_SIZE 1472
#define IMAGE_BLOCK_SIZE 1448
#define TIME_GAP  10

#define MAX_ROW    11
#define MAX_COLUMN 160*1000
#define TRACE_DATA 5

/* macro */
#define rows_of_array(name)       \
    (sizeof(name   ) / sizeof(name[0][0]) / columns_of_array(name))
#define columns_of_array(name)    \
    (sizeof(name[0]) / sizeof(name[0][0]))

/* local data structure */
typedef struct _dgram {
    uint32_t frame;
    uint32_t packet;
    uint16_t res1;
    uint16_t res2;
    uint32_t offset;
    uint64_t timestamp;
    uint16_t image[IMAGE_BLOCK_SIZE];
} DGRAM;

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

void diep(char *s)
{
    perror(s);
    exit(1);
}

/* Main function */

int main(int argc, char *argv[])
{

    /* Data structure */	
    struct sockaddr_in    si_other;                             
    int sockUDPfd, slen=sizeof(si_other), portno; 
    DGRAM dgram;
    uint32_t frame, packet, offset = 0;
    uint32_t copy_size; 	
    uint delay_usec; 
    time_t time_start, time_gap, time_start_per_image; 
    float throughput;
    ssize_t nsent = 0, total_sent_1=0, total_sent_2=0;
    struct hostent *host;
    int i, j, row, column, n=1; 
    uint16_t image[MAX_ROW][MAX_COLUMN] = {{0},{0}};
  
    /* Code */

    if (argc < 4) {
	fprintf(stderr,"ERROR, <IP address> <port> <delay time microsecond>\n");
	exit(1);
    }

    row = rows_of_array(image); 
    column = columns_of_array(image); 

    portno = atoi(argv[2]);
    delay_usec = atoi(argv[3]);
    host = gethostbyname(argv[1]);
    if (host == NULL) {
        perror("gethostbyname");
        return 1;
    }

    sockUDPfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    check (sockUDPfd < 0, "UDP server socket %s failed: %s", sockUDPfd, strerror(errno));

    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(portno);
    si_other.sin_addr = *((struct in_addr*) host->h_addr);

    time_start = time(NULL); /* seconds */  
    time_start_per_image = time_start;

    memcpy(&(image[1][0]), &data01, column*sizeof(uint16_t));
    memcpy(&(image[2][0]), &data02, column*sizeof(uint16_t));
    memcpy(&(image[3][0]), &data03, column*sizeof(uint16_t));
    memcpy(&(image[4][0]), &data04, column*sizeof(uint16_t));
    memcpy(&(image[5][0]), &data05, column*sizeof(uint16_t));
    memcpy(&(image[6][0]), &data06, column*sizeof(uint16_t));
    memcpy(&(image[7][0]), &data07, column*sizeof(uint16_t));
    memcpy(&(image[8][0]), &data08, column*sizeof(uint16_t));
    memcpy(&(image[9][0]), &data09, column*sizeof(uint16_t));
    memcpy(&(image[10][0]), &data10, column*sizeof(uint16_t));

    while(1) {
	for (i=0; i < row; i++) 
	    {
            dgram.frame = frame;
            dgram.packet = packet;
            dgram.offset = offset;
            copy_size = sizeof(image[i]) - offset;
            if (copy_size > IMAGE_BLOCK_SIZE ) copy_size = IMAGE_BLOCK_SIZE;
            memcpy(&(dgram.image), image[i] + offset/2, copy_size);

	    nsent = sendto(sockUDPfd, &dgram, DGRAM_SIZE, 0, (struct sockaddr *)&si_other, slen);  	
            check (nsent < 0, "sentto() %s failed: %s", sockUDPfd, strerror(errno));

            packet++;
    	    offset += copy_size;

	    time_gap = time(NULL) - time_start;
	    total_sent_1 += nsent; 
            if ( time_gap >= TIME_GAP )  {
 	        printf("==> Sending image%2d: ",i); 
                for (j=0; j<TRACE_DATA; j++) {	 
	           printf("%d,",image[i][j]); 	
	        }	
	        printf("......\n");  
	        throughput = ((double)total_sent_1/1000000)/time_gap; 
	        printf("==> Average throughput/%dsec: %.2f MB/s = %.2f Mbits/s <==\n", 
                    TIME_GAP, throughput, throughput*8 ); 
	        time_start = time(NULL);
                total_sent_1 = 0; 
	     } 	

    	    if (offset >= sizeof(image[i])) {
       	        packet = 0;
                offset = 0;
      	        frame++;
	        time_gap = time(NULL) - time_start_per_image;
		total_sent_2 += sizeof(image[i]); 
	        if (time_gap >= 5 ) { /* unknow which throughput is more preferable on the card*/
                    throughput = ((double)total_sent_2/1000000)/time_gap;
	            printf("Frame No%3d: throughput/(%d images): %.2f MB/s = %.2f Mbits/s\n", 
                        frame, n, throughput, throughput*8 ); 
	    	    time_start_per_image = time(NULL);
		    n=1; 				   
                    total_sent_2 = 0; /* it might not needed since image size is same*/    
	         }
	        else {
                   n++; 
	           }
    	     }
       	     usleep(delay_usec); 
	}
    }

    close(sockUDPfd);
    return(0);
}
