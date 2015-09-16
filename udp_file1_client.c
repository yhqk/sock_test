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
    char buf_text[30], buf_file[BUF_SIZE];
    int sockUDPfd, port, file_size, nleft, buf_len;;
    char new_file[]="copied_file";
    ssize_t nwritten, nreceived;
    struct sockaddr_in servaddr;
    struct hostent *host;
    int fd_new; 
    socklen_t len = sizeof(struct sockaddr_in);

    if (argc < 3) {
	fprintf(stderr, "Usage: %s <host> <port> \n", argv[0]);
	return 1;
    }

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
        printf("Found requested file and start open and get file size\n");
        trim((char *)buf_text); 	
        file_size = atoi(buf_text);
        printf("file size %d\n", file_size);

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
	    fflush(stdout);
            write(1, buf_file, nreceived);
	    nleft -= nwritten;
            }
    	close(fd_new);
	printf("\n"); 

	/* while loop to received more data */

 	}
  
    close(sockUDPfd);
 	
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
