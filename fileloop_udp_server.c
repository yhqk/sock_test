/* fileloop_udp_server.c
   Transfer a file from server to client via UDP socket after 
   client provide valided file name. 
      
$ gcc -o exec_s fileloop_udp_server.c -Wall -Wextra
$ ./exec_s 5000 1
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/mman.h> 
#include <time.h>

#define BUF_SIZE 1024
#define BIG_DATA 40000000

char* trim(char* input);

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

int main(int argc, char* argv[]) 
{
    /* Information about the file. */
    struct stat s;
    int status; 
    size_t  file_size, nleft, buf_len;
    char buf_text[40], buf_size[20];
    struct sockaddr_in self, other;	
    socklen_t len = sizeof(struct sockaddr_in);
    int n, sockUDPfd, fd, port, loop, i, wait_time;
    char *mappedFilePtr, *temp_ptr; 
    ssize_t nsent;
    time_t time_start, time_gap; 
    float throughput;

    n = 1; 
    if (argc < 4) {
	fprintf(stderr, "Usage: %s <port> <loop> <waittime>\n", argv[0]);
	return 1;
    }

    loop = atoi(argv[2]);
    wait_time = atoi(argv[3]);
	
    printf("repeat times %d, time gap for throughput %d seconds\n", loop, wait_time ); 

    /* initialize socket */
    sockUDPfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    check (sockUDPfd < 0, "UDP server socket %s failed: %s", sockUDPfd, strerror(errno));

    /* bind to server port */
    port = atoi(argv[1]);
    memset((char *) &self, 0, sizeof(struct sockaddr_in));
    self.sin_family = AF_INET;
    self.sin_port = htons(port);
    self.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sockUDPfd, (struct sockaddr *) &self, sizeof(self)) == -1) {
	perror("bind");
	return 1;
    }
    printf("UDP server bind\n");

    bzero(buf_text, sizeof(buf_text));
    if ((n = recvfrom(sockUDPfd, buf_text, 40, 0, (struct sockaddr *)&other, &len)) != -1) {
        printf("Received request client from %s:%d: \n", 
		inet_ntoa(other.sin_addr), 
		ntohs(other.sin_port)); 
        trim((char *)buf_text); 

	/* Open the request file */
        fd = open(buf_text, O_RDONLY);
        check (fd < 0, "open %s failed: %s", buf_text, strerror(errno));

        /* Get the size of the file so that client could write to the file
	   It is more or less for checkings whether data read/write via UDP socket
           is corrected or not. It will be not needed in the final product. 
	 */
        status = fstat (fd, &s);
        check (status < 0, "stat %s failed: %s",buf_text, strerror(errno));
        file_size = s.st_size;
        printf("To-be-transfered file %s with size %.3f KB\n", buf_text, (double)file_size/1000);
	bzero(buf_size, sizeof(buf_size));
	sprintf(buf_size, "%d", (int)file_size);
	nsent = sendto(sockUDPfd, buf_size, 20, 0, (struct sockaddr *)&other, len);  	
        check (nsent < 0, "sentto %s failed: %s", sockUDPfd, strerror(errno));

        /* Memory-map the file. */
        mappedFilePtr = mmap(0, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
        check (mappedFilePtr== MAP_FAILED, "mmap %s failed: %s",
           argv[1], strerror (errno));

	sleep( 2 );      
	i = 0; 
	time_start = time(NULL);           
	for ( n = 1; n <loop+1; n++ ) { 
            nleft = file_size;
	    temp_ptr = mappedFilePtr; 
            while ( nleft > 0 ) {
            	if ( nleft >= BUF_SIZE )
                    buf_len = BUF_SIZE; 
	    	else
	            buf_len = nleft;  
            	if ( (nsent = sendto(sockUDPfd, temp_ptr, buf_len, 0, 
			(struct sockaddr *)&other, len)) <= 0) {
                    if (nsent < 0 && errno == EINTR)
	            	nsent = 0;       /* and call write() again */
            	    else
                    	return(-1);         /* error */
	        };  
            	nleft -= nsent;
            	temp_ptr += nsent;		 
	   }	
	   
	   time_gap = time(NULL) - time_start;
           if ( time_gap >= wait_time )  {
	   	throughput = ((double)file_size*(n-i)/1000000)/time_gap; 
	        printf("file counter %7d; throughput %.2f MB/s\n", 
	             n, throughput); 
	        time_start = time(NULL);
 		i = n;   /* save the last value for next calculation */
	   } 	        
	}
    	munmap(mappedFilePtr, file_size);
        close(fd);
     }
 
     close(sockUDPfd);  /* maybe no need to be closed */
     printf("COMPLETED\n");
 
    return(0);
}

char *ltrim(char *s) 
{     
    while(isspace(*s)) s++;     
    return s; 
}  

char *rtrim(char *s) 
{     
    char* back;
    int len = strlen(s);

    if(len == 0)
        return(s); 

    back = s + len;     
    while(isspace(*--back));     
    *(back+1) = '\0';     
    return s; 
}  

char *trim(char *s) 
{     
    return rtrim(ltrim(s));  
} 

