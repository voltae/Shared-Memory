/* Simple receiver program with shared memory */

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>	/* Interface of shared memory */
#include <sys/stat.h> 	/* From mode constants */
#include <fcntl.h>	/* For O_* constants */
#include <unistd.h>		/* ftruncare and close */

#define SHM_NAME "/shm_shared"

int main (int argc, char ** argv)
{
    /* counter */
    int i = 0;
    char c;

    /* open a file descriptor with the given name of the shm */
    int shmfd = shm_open (SHM_NAME, O_WRONLY, 0);
    if (shmfd == -1)
    {
        printf("Error open shm\n");
    }

    /* mapping the shm_memory into the process memory */
    char *memorypointer = mmap (NULL, 16 * sizeof(char), O_RDONLY, MAP_SHARED, shmfd, 0);
    if (memorypointer == NULL)
    {
        printf("error mapping memory\n");
    }

    while (c != EOF)
    {
        c = memorypointer[i++];
         fputc(c, stdout);

    }
    if (munmap( memorypointer, 16 * sizeof (char)) == -1)
    {
        printf("Error in unmapping memory\n");
    }
    if (close(shmfd) == -1)
    {
        printf("error in closing shmfd\n");
    }
    if (shm_unlink(SHM_NAME) == -1)
    {
        printf("error in unlinking shm\n");
    }

    return 0;
}

