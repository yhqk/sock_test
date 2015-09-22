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
    uint16_t image[MAX_ROW][MAX_COLUMN];
    /* zero at beginning */	
    uint16_t data00[MAX_COLUMN]={0};

    row = rows_of_array(image); 
    column = columns_of_array(image); 

    memcpy(&(image[1][0]), &data00, sizeof(data00));
    printf("image/rows_of_array: %d, columns_of_array: %d\n",row, column);

    memcpy(&(image[1][0]), &data01, column);
    memcpy(&(image[2][0]), &data02, column);
    memcpy(&(image[3][0]), &data03, column);
    memcpy(&(image[4][0]), &data04, column);
    memcpy(&(image[5][0]), &data05, column);
    memcpy(&(image[6][0]), &data06, column);
    memcpy(&(image[7][0]), &data07, column);
    memcpy(&(image[8][0]), &data08, column);
    memcpy(&(image[9][0]), &data09, column);
    memcpy(&(image[10][0]), &data10, column);

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
