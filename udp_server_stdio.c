/* udp_server_stdio.c

Server receives a line of text from client(s) and echo back via 
UDP socket. 


$ gcc -o exec_s udp_server_stdio.c -Wall -Wextra
$ ./exec_s 5000

ref: http://www.xinotes.net/notes/note/1810/ 

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF_SIZE 1024

int main(int argc, char* argv[]) {
    char buf[BUF_SIZE];
    struct sockaddr_in self, other;	
    socklen_t len = sizeof(struct sockaddr_in);
    int n, sockUDPfd, port;

    n = 1; 

    if (argc < 2) {
	fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	return 1;
    }

    /* initialize socket */
    if ((sockUDPfd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
	perror("socket");
	return 1;
    }

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
    printf("Server UDP socket binded %s:%d\n", 
		inet_ntoa(self.sin_addr), 
		ntohs(self.sin_port));
 
    while ((n = recvfrom(sockUDPfd, buf, BUF_SIZE, 0, (struct sockaddr *)&other, &len)) != -1) {
        printf("Received echo from %s:%d: ", 
		inet_ntoa(other.sin_addr), 
		ntohs(other.sin_port)); 
        fflush(stdout);
        write(1, buf, sizeof(buf));
	/* echo back to client */
	sendto(sockUDPfd, buf, BUF_SIZE, 0, (struct sockaddr *)&other, len); 	
    }

    close(sockUDPfd);
    return 0;
}
