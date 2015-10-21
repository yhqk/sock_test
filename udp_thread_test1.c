/* 
http://stackoverflow.com/questions/26110732/sending-udp-messages-between-two-threads-in-the-same-c-program-linux
*/
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/time.h>

const int ipTx = 1, ipRx = 2, portTx = 3, portRx = 4, TxRate = 5;
const int wait_five = 5, num_msgs = 20, wait_ten = 10, wait_twenty = 20;
const int SEC_TO_MILLI = 1000;
int *message;

void *send_msg( void * );
void *receive_msg( void * );

int main( int argc, char *argv[] )
{
    message = 0;
    pthread_t sendThread, recieveThread;
    int sendFail, recFail;

    sendFail = pthread_create(&sendThread, NULL, send_msg, (void*) argv);
    if(sendFail)
    {
        printf("Error creating send thread\n");
    }

    recFail = pthread_create(&recieveThread, NULL, receive_msg, (void*) argv);
    if(recFail)
    {
        printf("Error creating receive thread\n");
    }

    pthread_join( sendThread, NULL);
    pthread_join( recieveThread, NULL);

    printf("Send thread and receive thread done. Program terminating.\n");

    return 0;
}

void *send_msg( void *argv)
{
    char **args = (char **)argv;
    int sendSocket;
    if((sendSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("cannot create send socket");
        return;
    }
    struct sockaddr_in myAddr;
    memset((char *)&myAddr, 0, sizeof(myAddr));
    myAddr.sin_family = AF_INET;
    myAddr.sin_addr.s_addr = inet_addr(args[ipTx]);
    int port = htons(atoi(args[portTx]));
    myAddr.sin_port = htons(port);
    if (bind(sendSocket, (struct sockaddr *)&myAddr, sizeof(myAddr)) < 0) 
    {
        perror("send socket bind failed");
        return;
    }

    struct sockaddr_in recAddr;
    memset((char*)&recAddr, 0, sizeof(recAddr));
    recAddr.sin_family = AF_INET; 
    recAddr.sin_port = htons(atoi(args[portRx]));
    recAddr.sin_addr.s_addr = inet_addr(args[ipRx]);

    printf("Sleeping for 5 seconds\n");
    sleep(wait_five);

    int sendRate = (intptr_t) args[TxRate];
    sendRate /= SEC_TO_MILLI;
    int i;
    for(i = 0; i < num_msgs; i++)
    {
        printf("I am TX and I am going to send a %i\n", message);
        if(sendto(sendSocket, message, sizeof(message), 0, (struct sockaddr *)&recAddr, sizeof(recAddr) ) < 0)
        {
            perror("sendto failed");
            return;
        }
        *message++;
        sleep(sendRate);
    }
    printf("Sleeping for 10 seconds\n");
    sleep(wait_ten);
}

void *receive_msg( void *argv)
{
    const int BUFF_SIZE = 2048;

    char **args = (char **)argv;

    struct sockaddr_in myAddress;
    struct sockaddr_in remoteAddress;
    socklen_t addressLength = sizeof(myAddress);
    int recvLength;
    int receiveSocket;
    unsigned int buf[BUFF_SIZE];

    if((receiveSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("cannot create receive socket\n");
        return;
    }

    memset((char*)&myAddress, 0, sizeof(myAddress));
    myAddress.sin_family = AF_INET;
    myAddress.sin_addr.s_addr = inet_addr(args[ipRx]);;
    myAddress.sin_port = htons(atoi(args[portRx]));

    struct timeval rec_timeout;
    rec_timeout.tv_sec = wait_twenty;
    rec_timeout.tv_usec = 0;

    if(setsockopt(receiveSocket, SOL_SOCKET, SO_RCVTIMEO, (const void *)&rec_timeout, sizeof(rec_timeout)) < 0)
    {
        perror("cannot set timeout option\n");
        return;
    }

    if(bind(receiveSocket, (const struct sockaddr *)&myAddress, sizeof(myAddress)) < 0)
    {
        perror("cannot bind receive socket\n");
        return;
    }

    for(;;)
    {
        printf("waiting on port %d\n", atoi(args[portRx]));
        recvLength = recvfrom(receiveSocket, buf, BUFF_SIZE, 0, (struct sockaddr*)&remoteAddress, &addressLength);
        printf("received %d bytes\n", recvLength);
        if (recvLength > 0)
        {
            buf[recvLength] = 0;
            printf("I am RX and I got a \"%d\"\n", buf);
        }
    }
}
