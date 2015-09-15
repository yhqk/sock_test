/* Copy the binary file to another file using mmap */
/* gcc -o exec_a file_read_write_mmap.c -Wall -Wextra */

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

#define BUF_SIZE   1024

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

int main(int argc, char *argv[]) {

    /* Information about the file. */
    struct stat s;
    int status; 
    size_t  size, nleft, buf_len;
    ssize_t nwritten;
    int fd, fd_new; 
    char *mappedFilePtr; 
    char *temp_ptr; 

    /* Check arguments */
    if (argc < 3) {
	fprintf(stderr,"ERROR, sourceFile destFile\n");
	exit(1);
    }

    fd = open(argv[1], O_RDONLY);
    check (fd < 0, "open %s failed: %s", argv[1], strerror(errno));

    /* Get the size of the file. */
    status = fstat (fd, &s);
    check (status < 0, "stat %s failed: %s", argv[1], strerror(errno));
    size = s.st_size;

    printf("read file size %ld\n", size); 
 
    /* Memory-map the file. */
    mappedFilePtr = mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
    check (mappedFilePtr== MAP_FAILED, "mmap %s failed: %s",
           argv[1], strerror (errno));

    fd_new = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
    check (fd_new < 0, "open %s failed: %s", argv[2], strerror (errno));

    /* read return value and write return value check */
    nleft = size; 
    temp_ptr = mappedFilePtr; 
	    
    while ( nleft > 0 ) {
        if ( nleft >= BUF_SIZE )
            buf_len = BUF_SIZE; 
	else
	    buf_len = nleft;  
        if ( (nwritten = write(fd_new, temp_ptr, buf_len)) <= 0) {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0;       /* and call write() again */
            else
                return(-1);         /* error */
        }
/*
       printf("write size %ld\n", nwritten);   
       printf("left file size to write %ld\n", nleft);  
*/
       nleft -= nwritten;
       temp_ptr += nwritten;
    }

    munmap(mappedFilePtr, size);

    printf("COMPLETED\n");
    close(fd);
    close(fd_new);  
 
    return 0;
}
