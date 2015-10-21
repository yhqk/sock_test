/* echo_tcp_client.c
  TCP client to server data01 and receive echo back message  
  Server 5 times. (just avoid loop)

  Compiling and Execution
  $ gcc -o exec_c echo_tcp_client.c -Wall 
  $ ./exec_c localhost 5000  
  or
  $ ./exec_c 192.168.1.46 5000

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

#include "scanner_tablet_msg.h"

#define BUF_SIZE 256

uint16_t data01[] = {
    31535,32060,31798,31613,31434,32665,31631,32033,31914,32564,
    31394,31730,31320,31632,31646,31521,31436,32361,31555,31780,
    31507,32945,32049,31992,31801,33013,32068,31272,31505,33085,
    31476,31520,34520,34859,30142,30174,32037,33707,33492,35103,
    33261,23805,24041,19824,31023,49144,33209,16384,24788,41839
}; 

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
    int sockTCPfd, portno; 
    struct sockaddr_in serv_addr;
    struct hostent *server;      
    char buffer_recv[BUF_SIZE];
    int i, n, ret_val;
  
    n = 1;
    check ( argc<3 , "ERROR: Only missing arguments\nUsage: %s <hostname> <port>", argv[0]); 
	
    portno = atoi(argv[2]);
    sockTCPfd = socket(AF_INET, SOCK_STREAM, 0);
    check(sockTCPfd < 0, "TCP client create %s failed: %s", sockTCPfd, strerror(errno));

    server = gethostbyname(argv[1]);
    if (server == NULL) {
	fprintf(stderr,"ERROR, no such host\n");
	exit(0);
    }
   
    memset((char *) &serv_addr, 0,sizeof(serv_addr));  
    serv_addr.sin_family = AF_INET;
    memcpy((char *)&serv_addr.sin_addr.s_addr,(char *)server->h_addr, 
	   server->h_length);
    serv_addr.sin_port = htons(portno);  
    ret_val = connect(sockTCPfd,(struct sockaddr *)&serv_addr, sizeof(serv_addr)); 
    check (ret_val < 0, "TCP client connect(): %s", strerror(errno));    
	
    while (n>0) {
	printf("\nSending TCP data01 with length: %d", (int)sizeof(data01));
	for ( i = 0; i < sizeof(data01); i++) {
            if (!(i%16)) printf("\n%08lx  ", (long unsigned int)i);
            if (!(i%8)) printf(" ");
	    printf("%02x ", ((uint8_t*)&data01)[i]);
	}
        printf("\n");
	n = write(sockTCPfd,data01,sizeof(data01));
        check(n < 0, "TCP client write %s failed: %s", sockTCPfd, strerror(errno));	
	printf("The number of bytes actually written: %d\n", n);

	n = read(sockTCPfd,buffer_recv, BUF_SIZE);
	printf("\nThe number of bytes actually read due echo from TCP server:%d", n);	
	if (n > 0) {
	    for( i = 0; i < n; i++) {
	        if (!(i%16)) printf("\n%08lx  ", (long unsigned int)i);
                if (!(i%8)) printf(" ");
                printf("%02x ", (uint8_t)(buffer_recv[i]));
	    }
	}	
        else {
            printf("read error\n");
	}
	printf("\n"); 
    }
    return 1; 
}
