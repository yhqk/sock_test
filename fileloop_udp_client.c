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

int main(int argc, char *argv[]) 
{

    /* Information about the file. */
    struct stat s;
    int status; 
    size_t  file_size;	
    char buf_text[30], buf_file[BUF_SIZE];
    int sockUDPfd, port, nleft, buf_len, loop, i;
    char new_file[]="copied_file";
    ssize_t nwritten, nreceived, temp_n;
    struct sockaddr_in servaddr;
    struct hostent *host;
    int fd_new; 
    socklen_t len = sizeof(struct sockaddr_in);
    time_t time_start, time_gap; 
    float throughput;

    if (argc < 4) {
	fprintf(stderr, "Usage: %s <host> <port> <times>\n", argv[0]);
	return 1;
    }

    loop = atoi(argv[3]);
    printf("Repeat time: %d\n", loop ); 

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
 
    printf("Client UDP socket is ready %s:%d: ", 
	inet_ntoa(servaddr.sin_addr), 
	ntohs(servaddr.sin_port)); 

    printf("\nEnter file to be transferred:");
    scanf("%s",buf_text);
 
    sendto(sockUDPfd, buf_text, strlen(buf_text)+1, 0,
     (struct sockaddr *)&servaddr, sizeof(struct sockaddr));

    nreceived = recvfrom(sockUDPfd, buf_text, sizeof(buf_text), 0, (struct sockaddr *)&servaddr, &len); 
    if ( nreceived != -1) {
        trim((char *)buf_text); 	
        file_size = atoi(buf_text);
        printf("Found requested file with size %d\n", (int)file_size);

	fd_new = open(new_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	check (fd_new < 0, "open %s failed: %s", argv[2], strerror (errno));

        nleft = file_size;  	    
    	while ( nleft > 0 ) {
            if ( nleft >= BUF_SIZE )
                buf_len = BUF_SIZE; 
	    else {
	        buf_len = nleft;  
	       }
            bzero(buf_file, sizeof(buf_file));
	    nreceived = recvfrom(sockUDPfd, buf_file, buf_len, 0, (struct sockaddr *)&servaddr, &len); 
   	    check( nreceived < 0, "Recieving %s failed: %s", sockUDPfd, strerror(errno));
  	    if ( (nwritten = write(fd_new, buf_file, nreceived)) <= 0) {
                if (nwritten < 0 && errno == EINTR)
                    nwritten = 0;       /* and call write() again */
            	else
                    return(-1);         /* error */
            	}
/*
	    fflush(stdout);
            write(1, buf_file, nreceived);
*/
	    nleft -= nwritten;
            }
    	close(fd_new);

	/* Re-open the check file size */
        fd_new = open(new_file, O_RDONLY);
        check (fd_new < 0, "open %s failed: %s", new_file, strerror(errno));
        status = fstat (fd_new, &s);
        check (status < 0, "stat %s failed: %s",new_file, strerror(errno));
        file_size = s.st_size;
        printf("Copied %s with size %d\n", new_file, (int)file_size);
    	close(fd_new);
	}
 
	if ( loop > 1 ) {
	    temp_n = 0;  
	    time_start = time(NULL); /* seconds */
	    i = 0; 
            bzero(buf_file, sizeof(buf_file));	
	    while ((nreceived = recvfrom(sockUDPfd, buf_file, buf_len, 0, (struct sockaddr *)&servaddr, &len)) != -1) {
	    	temp_n+=nreceived;
		i++;  
	        if ( i%100 == 0 ) {
		    time_gap = time(NULL) - time_start; 
		    throughput = (temp_n/1000)/time_gap; 
	            printf("time gap %i; throughput %.2f KB/s\n", (int)time_gap, throughput);  
		    time_start = time(NULL);
		    temp_n = 0;
		    i = 0; 			
		}
            bzero(buf_file, sizeof(buf_file));	
	    }	
	}
    close(sockUDPfd); /*never reach here*/
 	
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
