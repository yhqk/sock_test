/* udp_client_stdio.c

Client(s) read(s) a line from stdio and sends to Server via 
UDP socket

$ gcc -o exec_c udp_client_stdio.c -Wall -Wextra
$ ./exec_c localhost 5000      

ref: http://www.xinotes.net/notes/note/1810/ 

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    struct sockaddr_in server;
    socklen_t len = sizeof(struct sockaddr_in);
    char buf[BUF_SIZE];
    struct hostent *host;
    int n, sockUDPfd, port;

    n = 1;  // in order to go to the 1st while loop

    if (argc < 3) {
	fprintf(stderr, "Usage: %s <host> <port> \n", argv[0]);
	return 1;
    }

    host = gethostbyname(argv[1]);
    if (host == NULL) {
	perror("gethostbyname");
	return 1;
    }

    port = atoi(argv[2]);

    /* initialize socket */
    if ((sockUDPfd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
	perror("UDP socket");
	return 1;
    }

    /* initialize server addr */
    memset((char *) &server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr = *((struct in_addr*) host->h_addr);

    printf("Client UDP socket is ready %s:%d: ", 
	inet_ntoa(server.sin_addr), 
	ntohs(server.sin_port)); 
    /* send message 
    if (sendto(sockUDPfd, argv[3], strlen(argv[3]), 0, (struct sockaddr *) &server, len) == -1) {
	perror("sendto()");
	return 1;
    }
    */
    /* receive echo.
    ** for single message, "while" is not necessary. But it allows the client 
    ** to stay in listen mode and thus function as a "server" - allowing it to 
    ** receive message sent from any endpoint.
    */
    while ( n > 0 ) { 
	/* allow to send the more messages */
	memset(buf,0,BUF_SIZE);
	if ( fgets(buf,BUF_SIZE,stdin) != NULL) {
	    if (sendto(sockUDPfd, buf, BUF_SIZE, 0, (struct sockaddr *) &server, len) == -1) {
	    	perror("Client sendto()");
	    	return 1;
		}
	}
	n = recvfrom(sockUDPfd, buf, BUF_SIZE, 0, (struct sockaddr *)&server, &len); 
	if ( n != -1) {
	    printf("Received echo from %s:%d: ", 
		inet_ntoa(server.sin_addr), 
		ntohs(server.sin_port)); 
	    fflush(stdout);
	    write(1, buf, sizeof(buf));
	}
    }
	
    close(sockUDPfd);
    return 0;
}
