#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>

#define NBYTES 4

int main(){
    uint32_t ipAddress =  3232235691; /* C0A800AB 192.168.0.171*/
    uint32_t temp = 54322; 
    uint8_t  octet[NBYTES];    
    int x;
    char ip_string[15];
    char port_string[6];

    struct sockaddr_in antelope;
    char *some_addr;


    sprintf(port_string , "%d", temp);
    printf("sprintf():temp= %s, sizeof(temp)=%d\n",port_string, sizeof(port_string)  );  

    printf("inet_ntoa (reverse):\n");
    inet_aton("192.168.1.8", &antelope.sin_addr); // store IP in antelope
    some_addr = inet_ntoa(antelope.sin_addr); // converted to string
    printf("%s\n", some_addr); // prints "192.168.0.171"

    // replace ip address and print again
    antelope.sin_addr.s_addr = inet_addr("127.0.0.1");
    some_addr = inet_ntoa(antelope.sin_addr); // return the IP
    printf("size %d, %s\n", (int)sizeof(some_addr), some_addr); // prints "10.0.0.1"

    // replace IP address from uint32, and print
    antelope.sin_addr.s_addr = htonl(ipAddress);
    some_addr = inet_ntoa(antelope.sin_addr); // return the IP
    printf("%s\n", some_addr); // prints "192.168.0.171"	

    temp = htonl(ipAddress);  // flip IP address endian
    // temp:ab00a8c0, ipAddress: c0a800ab
    printf("temp:0x%x, ipAddress: %x \n", temp, ipAddress);

    printf("ipAddress (reverse): ");
    for( x = 0; x < NBYTES; x++) {
       printf("%2x ", (ipAddress >> (x * 8)) & (uint8_t)-1);
    } /* ab  0 a8 c0: 171 00 168 192*/
    printf("\n");

    printf("temp as hex (htonl): ");
    for( x = 0; x < NBYTES; x++) {
       printf("%02x ", (temp >> (x * 8)) & (uint8_t)-1);
    } /* c0 a8 00 ab */
    printf("\n\n");

    bzero( ip_string, sizeof(ip_string));
    for (x = 0; x < NBYTES; x++) {
        octet[x] = (ipAddress >> (x * 8)) & (uint8_t)-1;
    }
    for (x = NBYTES - 1; x >= 0; --x) {
        printf("%d", octet[x]);
        sprintf(ip_string, "%d", octet[x]);
	/* strcat(ipstring, a);	 */
        if (x > 0) {
		printf(".");
	strcat(ip_string, ".");
	}
        printf("\nx: %d, ip_string %s, sizeof(ip_string) %d\n", x, ip_string, (int)sizeof(ip_string));
    }
    printf("\n");
    printf("ip_string %s\n", ip_string);
    return 0;
}
