/* array_test.c

   It sends images[160*1000] defined in image01.h 

*/

#include <string.h>  
#include <stdlib.h>  //exit
#include <stdint.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>

#include "data_array_image01.h"
#include "data_array_image02.h"
#include "data_array_image03.h"
#include "data_array_image04.h"
#include "data_array_image05.h"
#include "data_array_image06.h"
#include "data_array_image07.h"
#include "data_array_image08.h"
#include "data_array_image09.h"
#include "data_array_image10.h"

#define DGRAM_SIZE 1472
#define IMAGE_BLOCK_SIZE 1448
#define TIME_GAP  10


#define rows_of_array(name)       \
    (sizeof(name   ) / sizeof(name[0][0]) / columns_of_array(name))
#define columns_of_array(name)    \
    (sizeof(name[0]) / sizeof(name[0][0]))


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

void diep(char *s)
{
    perror(s);
    exit(1);
}

typedef struct _dgram {
  uint32_t frame;
  uint32_t packet;
  uint16_t res1;
  uint16_t res2;
  uint32_t offset;
  uint64_t timestamp;
  uint16_t image[IMAGE_BLOCK_SIZE];
} DGRAM;


int main(int argc, char *argv[])
{

    int i, j;
/*	
    i = columns_of_array(image); 
    j = rows_of_array(image); 	*/
    printf("columns_of_array: %ll\n", image[0,][0] ); 
	
    return(0);
}
