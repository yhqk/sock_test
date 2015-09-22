/* udp_add_image01.c

   It sends images[160*1000] defined in image01.h over UDP socket from Server to client. 
   arguement: <IP address> <port> <delay time microsecond> 
   
   $ gcc -o exec_a udp_add_image01.c -Wall -I${PWD}/images/
   $ ./exec_a 192.168.1.87 54321 200 

*/

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

#include "data_array_image01.h"

#define DGRAM_SIZE 1472
#define IMAGE_BLOCK_SIZE 1448
#define TIME_GAP  10

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

typedef struct _dgram {
  uint32_t frame;
  uint32_t packet;
  uint16_t res1;
  uint16_t res2;
  uint32_t offset;
  uint64_t timestamp;
  uint16_t image[IMAGE_BLOCK_SIZE];
} DGRAM;


int main(int argc, char *argv[])
{
    struct sockaddr_in    si_other;                             
    int sockUDPfd, slen=sizeof(si_other), portno; 
    uint32_t frame, packet, offset = 0;
    uint32_t copy_size; 	
    ssize_t nsent = 0, total_sent=0;
    uint delay_usec; 
    time_t time_start, time_gap, time_start_per_image; 
    float throughput;
    uint16_t image[160*1000] = {0};
    struct hostent *host;
    int n=1;
//    int i;   	
  
    if (argc < 4) {
	fprintf(stderr,"ERROR, <IP address> <port> <delay time microsecond>\n");
	exit(1);
    }
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

    memcpy(&image, &data01, sizeof(image) );
 
    while(1) {
        // generate proper header.
        DGRAM dgram;
        dgram.frame = frame;
        dgram.packet = packet;
        dgram.offset = offset;

        copy_size = sizeof(image) - offset;
        if (copy_size > IMAGE_BLOCK_SIZE ) copy_size = IMAGE_BLOCK_SIZE;
        memcpy(&(dgram.image), image + offset/2, copy_size);

	nsent = sendto(sockUDPfd, &dgram, DGRAM_SIZE, 0, (struct sockaddr *)&si_other, slen);  	
        check (nsent < 0, "sentto() %s failed: %s", sockUDPfd, strerror(errno));

        packet++;
    	offset += copy_size;

	time_gap = time(NULL) - time_start;
	total_sent += nsent; 
           if ( time_gap >= TIME_GAP )  {
	       throughput = ((double)total_sent/1000000)/time_gap; 
	       printf("==> Average throughput/%dsec: %.2f MB/s = %.2f Mbits/s <==\n", 
                   TIME_GAP, throughput, throughput*8 ); 
	       time_start = time(NULL);
               total_sent = 0; 
	   } 	

    	if (offset >= sizeof(image)) {
       	    packet = 0;
            offset = 0;
      	    frame++;
	    time_gap = time(NULL) - time_start_per_image;
	    if (time_gap > 0 )
	        {
                throughput = ((double)(sizeof(image)*n)/1000000)/time_gap;
		/*  no need to print 1st 5 elements since it is the same data
	        for (i=0; i<5; i++)
   	            {	 
	            printf("%d,",image[i]); 	
	            }	
	        printf("......\n"); */ 
	        printf("Frame No%3d: throughput/(%d images): %.2f MB/s = %.2f Mbits/s\n", 
                    frame, n, throughput, throughput*8 ); 
	    	time_start_per_image = time(NULL);
	        n=1;  
	        }
	    else 
	        {
	        n++; 
	        }
    	}
       	usleep(delay_usec); 
    }

    close(sockUDPfd);
    return(0);
}
