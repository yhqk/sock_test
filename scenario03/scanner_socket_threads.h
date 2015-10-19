/* scanner_socket_threads.h

   Header file used by scanner_socket_threads.c
 
 */

#ifndef SCANNER_SOCKET_THREADS_H
#define SCANNER_SOCKET_THREADS_H    

/* Definitions */ 
/* epoll related parameters */
#define MAXEVENTS         10
#define BACKLOG           5 

/* TCP socket */     
#define TCP_BUF_HDR_SIZE  12
#define TCP_BUF_SIZE      256

/* UDP socket */
#define DGRAM_SIZE        1472
#define IMAGE_BLOCK_SIZE  1448
#define TIME_GAP          10

/* Image size */
#define MAX_ROW           10
#define MAX_COLUMN        160*1000

/* Refer to ultrasound Communication Commands */
typedef struct _udpPacket {
  uint32_t blockId;
  uint32_t packetId;
  uint16_t spare1;
  uint16_t spare2;
  uint32_t offset;
  uint64_t timestamp;
  uint16_t image[IMAGE_BLOCK_SIZE];
} udpPacket;

#endif /* SCANNER_SOCKET_THREADS_H */

