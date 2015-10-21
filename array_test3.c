/* array_test3.c / Yuhong Qiu-Kaikkonen

  The data is ready as files as array and copy to another 
  2-dimension array, so that it can be sent by loop via UDP
  in udp_multi_images.c. 

  data_arrat_image0*.h should be stored in inclued path	

  $ gcc -o exec_a array_test3.c -Wall -I${PWD}/images/
  $ ./exec_a

*/

#include <string.h>  
#include <stdlib.h>  //exit
#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>

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

#define MAX_ROW    11
#define MAX_COLUMN 160*1000
#define TRACE_DATA 5

#define rows_of_array(name)       \
    (sizeof(name   ) / sizeof(name[0][0]) / columns_of_array(name))
#define columns_of_array(name)    \
    (sizeof(name[0]) / sizeof(name[0][0]))

int main(int argc, char *argv[])
{
    int i, j, row, column; 
    uint16_t image[MAX_ROW+2][MAX_COLUMN] = {{0},{0}};

    row = rows_of_array(image); 
    column = columns_of_array(image); 

    /*memcpy(&(image[1][0]), &data00, sizeof(data00)); */
    printf("image/rows_of_array: %d, columns_of_array: %d\n",row, column);
    memcpy(&(image[1][0]), &data01[0], column*sizeof(uint16_t));
    printf("size of data01 %d, size of image[1] %d\n", 
        (int)sizeof(data01), (int)sizeof(image[1]) );

    memcpy(&(image[2][0]), &data02[0], column*sizeof(uint16_t) );
    memcpy(&(image[3][0]), &data03[0], column*sizeof(uint16_t));
    memcpy(&(image[4][0]), &data04[0], column*sizeof(uint16_t));
    memcpy(&(image[5][0]), &data05[0], column*sizeof(uint16_t));
    memcpy(&(image[6][0]), &data06[0], column*sizeof(uint16_t));    
    memcpy(&(image[7][0]), &data07[0], column*sizeof(uint16_t));
    memcpy(&(image[8][0]), &data08[0], column*sizeof(uint16_t));
    memcpy(&(image[9][0]), &data09[0], column*sizeof(uint16_t));
    memcpy(&(image[10][0]), &data10[0], column*sizeof(uint16_t));

    for (i=0; i < row; i++) 
	{
	printf("image%2d: ",i); 
        for (j=0; j<TRACE_DATA; j++)
	    {	 
	    printf("%d,",image[i][j]); 	
	    }	
	printf("......\n");     
	}

    return(0); 

}
