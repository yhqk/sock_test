#include <string.h>  
#include <stdlib.h>  //exit
#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include "test_data1.h"
#include "test_data2.h"
#include "test_data3.h"

#define g_rgDialogRows   2
#define g_rgDialogCols   7

static char* g_rgDialog[g_rgDialogRows][g_rgDialogCols] =
{
    { " ",  " ",    " ",    " 494", " 210", " Generic Sample Dialog", " " },
    { " 1", " 330", " 174", " 88",  " ",    " OK",        " " },
};

/* 2nd attempt
static uint16_t data1[9] = {31535,32060,31798,31613,31434,32665,31631,32033,31914};
static uint16_t data2[9] = {31495,31347,32629,32196,32399,31816,32962,32360,32384};
static uint16_t data3[9] = {32239,31738,32460,31772,31859,32086,32546,32113,32977};
*/
/* 1st attempt
static uint16_t image[g_rgDialogRows+1][g_rgDialogCols+2] = 
{
    {31535,32060,31798,31613,31434,32665,31631,32033,31914},
    {31495,31347,32629,32196,32399,31816,32962,32360,32384},
    {32239,31738,32460,31772,31859,32086,32546,32113,32977},
};
*/

#define rows_of_array(name)       \
    (sizeof(name   ) / sizeof(name[0][0]) / columns_of_array(name))
#define columns_of_array(name)    \
    (sizeof(name[0]) / sizeof(name[0][0]))

int main(int argc, char *argv[])
{

    int i, j, row, column; 
    uint16_t image[4][9];

    uint16_t data0[9]={0};
	
    row = (int)(rows_of_array(g_rgDialog)); 
    column = columns_of_array(g_rgDialog); 
    printf("g_rgDialog: rows_of_array %d\n",row); 
    printf("g_rgDialog: columns_of_array %d\n",column); 
    assert(rows_of_array(g_rgDialog) == 2);
    assert(columns_of_array(g_rgDialog) == 7);

    	
    memcpy(&(image[0][0]), &data0, sizeof(data0));
    memcpy(&(image[1][0]), &data1, sizeof(data1));
    memcpy(&(image[2][0]), &data2, sizeof(data2));
    memcpy(&(image[3][0]), &data3, sizeof(data3));
		
    row = rows_of_array(image); 
    column = columns_of_array(image); 
	
    printf("image: rows_of_array %d, image[0][1] %d\n",row, image[0][1]); 	
    printf("image: columns_of_array %d\n",column); 
    for (i=0; i < row; i++) 
	{
	printf("image: row%2d: ",i); 
        for (j=0; j<column; j++)
	    {	 
	    printf("%d,",image[i][j]); 	
	    }	
	printf("\n");     
	}

return(0); 

}
