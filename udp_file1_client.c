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
#define MAX_FILESIZE_TO_WRITE 1000000 

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
    int sockUDPfd, port, nleft, buf_len;;
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

    trim((char *)buf_text); 	
    file_size = atoi(buf_text);
    printf("Found requested file with size %d bytes\n", (int)file_size);

    if ( (nreceived != -1) && (file_size < MAX_FILESIZE_TO_WRITE) ) {
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
                if (nwritten < 0 && errno == EINTR) {
                    nwritten = 0;       /* and call write() again */
		    printf(" nwritten < 0 && errno == EINTR\n");
	            } 
            	else {
	    printf(" ERROR\n");
                    return(-1);         /* error */
	}
            	}
//	    fflush(stdout);
//            write(1, buf_file, nreceived);
	    nleft -= nwritten;
	    printf("buf_len %d, nreceived %d, nwritten %d, nleft %d\n", buf_len, (int)nreceived, (int)nwritten, (int)nleft);
            }
    	close(fd_new);
	printf("close copied file\n"); 

	/* Re-open the check file size */
        fd_new = open(new_file, O_RDONLY);
        check (fd_new < 0, "open %s failed: %s", new_file, strerror(errno));
        status = fstat (fd_new, &s);
        check (status < 0, "stat %s failed: %s",new_file, strerror(errno));
        file_size = s.st_size;
        printf("Check copied %s with size %d bytes\n", new_file, (int)file_size);
    	close(fd_new);

 	}
  
        close(sockUDPfd);
	printf("close UDP socket\n");  	
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
