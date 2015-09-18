/* udp_file1_client.c
   To transfer a file from server to client via UDP sockets after 
   client provide valided file name. 


$ gcc -o exec_c udp_file1_server.c -Wall -Wextra

$ ./exec_c localhost 5000
or 
$ ./exec_c 146.123.1.1 5000
*/


#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h> 
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <time.h>

#define BUF_SIZE 1024 
#define BIG_DATA  40000000 
#define STORE_MIN   300000
#define TIME_GAP 5

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

int main(int argc, char *argv[]) {

    /* Information about the file. */
    struct stat s;
    int status; 
    size_t  file_size;	
    char buf_text[30], buf_file[BUF_SIZE];
    int sockUDPfd, port, nleft, buf_len, loop, i=1;
    char new_file[]="copied_file";
    ssize_t nwritten, nreceived, temp_n=0;
    struct sockaddr_in servaddr;
    struct hostent *host;
    int fd_new; 
    socklen_t len = sizeof(struct sockaddr_in);
    time_t time_start= 0, time_gap= 0; 
    float throughput;

    if (argc < 4) {
	fprintf(stderr, "Usage: %s <host> <port> <times>\n", argv[0]);
	return 1;
    }

    loop = atoi(argv[3]);
    printf("Repeat times: %d\n", loop ); 

    host = gethostbyname(argv[1]);
    if (host == NULL) {
	   perror("gethostbyname");
	   return 1;
    }
 
    sockUDPfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    check (sockUDPfd < 0, "UDP server socket %s failed: %s", sockUDPfd, strerror(errno));
 
    port = atoi(argv[2]); 
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);  // Port address
    servaddr.sin_addr = *((struct in_addr*) host->h_addr);
 
    printf("Client UDP socket is ready %s:%d ", 
	inet_ntoa(servaddr.sin_addr), 
	ntohs(servaddr.sin_port)); 

    printf("\nEnter file to be transferred: ");
    scanf("%s",buf_text);
 
    sendto(sockUDPfd, buf_text, strlen(buf_text)+1, 0,
        (struct sockaddr *)&servaddr, sizeof(struct sockaddr));

    nreceived = recvfrom(sockUDPfd, buf_text, sizeof(buf_text), 0, (struct sockaddr *)&servaddr, &len); 
    check( nreceived < 0, "Recieving %s failed: %s", sockUDPfd, strerror(errno));
    trim((char *)buf_text); 	
    file_size = atoi(buf_text);
    printf("Found requested file with size %.3f KB\n", (double)file_size/1000);
    if ( file_size < STORE_MIN ) {
        fd_new = open(new_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        check (fd_new < 0, "open %s failed: %s", argv[2], strerror (errno));
        nleft = file_size;      
	time_start = time(NULL); /* seconds */   		
    	while ( nleft > 0 ) {
            if ( nleft >= BUF_SIZE )
                buf_len = BUF_SIZE; 
            else 
	           buf_len = nleft;  
            bzero(buf_file, sizeof(buf_file));
            nreceived = recvfrom(sockUDPfd, buf_file, buf_len, 0, (struct sockaddr *)&servaddr, &len); 
            check( nreceived < 0, "Recieving %s failed: %s", sockUDPfd, strerror(errno));
            nwritten = write(fd_new, buf_file, nreceived); 
   	    check( nwritten < 0, "Write %s failed: %s", fd_new, strerror(errno));
            if (nwritten < 0 && errno == EINTR) {
                nwritten = 0;       /* and call write() again */
            }
            nleft -= nwritten;
    
	    /* For VM is too slow to access memory set 30 sec timeout*/
            time_gap = time(NULL) - time_start;
            if ( time_gap > 30 ) { 
                nleft=0; 
            }	

        }   
    	close(fd_new);

	/* Re-open the check file size */
        fd_new = open(new_file, O_RDONLY);
        check (fd_new < 0, "open %s failed: %s", new_file, strerror(errno));
        status = fstat (fd_new, &s);
        check (status < 0, "stat %s failed: %s",new_file, strerror(errno));
        file_size = s.st_size;
        printf("Reopen/Check %s with size %.3f KB\n", new_file, (double)file_size/1000);
    	close(fd_new);
    }
    else {
	printf("Not store file over %d, just check throughput.\n", STORE_MIN);
        loop = 2;  
        } 	
  
	time_start = time(NULL); /* seconds */
	while ( loop > 1 ) {
            bzero(buf_file, sizeof(buf_file));	
	    nreceived = recvfrom(sockUDPfd, buf_file, BUF_SIZE, 0, (struct sockaddr *)&servaddr, &len); 
   	    check( nreceived < 0, "Recieving %s failed: %s", sockUDPfd, strerror(errno));
	    temp_n+=nreceived; 	
            time_gap = time(NULL) - time_start;
	    if ( time_gap > TIME_GAP ) { 
		    throughput = ((double)temp_n/1000000)/time_gap; 
		    printf("%10d recvfrom throughput %.3f MB/s, time gap %d sec\n", 
	            	i, throughput, (int)time_gap);  
		    temp_n = 0;
                    time_start = time(NULL);		
	    }
	    i++; /* loop counter for recevform */	
	}
    	close(sockUDPfd); /*never reach here unless there is another timer*/
 	
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
