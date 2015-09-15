/* Print out file size 

$ gcc -o exec_a file_read.c -Wall
$ ./exec_a
*/

#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h> 
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#define NUM_FILE   18

static void check (int test, const char * message, ...) {
    if (test) {
        va_list args;
        va_start (args, message);
        vfprintf (stderr, message, args);
        va_end (args);
        fprintf (stderr, "\n");
        exit (EXIT_FAILURE);
    }
}

const char *file_list[NUM_FILE] = {
	"Image01.jpg",
	"Image02.jpg",
	"Image03.jpg",
	"Image04.jpg",
	"Image05.jpg",
	"Image06.jpg",
	"Image07.jpg",
	"Image08.jpg",
	"Image09.jpg",
	"Image10.jpg",
	"RandomNumbers_15KB",
	"RandomNumbers_10KB",
	"RandomNumbers_05KB",
	"RandomNumbers_01KB",
	"RandomNumbers_1024B",
	"RandomNumbers_256B",
	"RandomNumber_1024B.txt",
	"RandomNumber_256B.txt"
	};

int main()
{
    /* Information about the file. */
    struct stat s;
    int status, i; 
    size_t size;
    int fd; 

    for ( i = 0; i< NUM_FILE; i++) {
	fd = open(file_list[i], O_RDONLY);
    	check (fd < 0, "open %s failed: %s", file_list[i], strerror(errno));

    	/* Get the size of the file. */
    	status = fstat (fd, &s);
    	check (status < 0, "stat %s failed: %s", file_list[i], strerror(errno));
    	size = s.st_size;
	printf("read file %s with size %ld\n", file_list[i], size); 
	close(fd);
	}
   
   return(0);
}
