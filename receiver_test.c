//
// Created by marcaurel on 11.05.18.
//

#include <stdio.h>
#include <stdlib.h>  // for EXIT_SUCCESS etc

#include <semaphore.h>  // for semaphores
#include <fcntl.h>      // For O_* constants
#include <sys/stat.h>   // For mode constants
#include <sys/mman.h>   // for shared memory
#include <unistd.h>     // for ftruncate() truncation shared memory

#include <errno.h>      // for errno global constant
#include <string.h>     // for strerror



#define ERROR (-1)
#define MAPSIZE 20
/* Global constant semaphore read */
sem_t *readSemaphore;
const char *semaphoreReadName = "/semaphore_read";

/* Global constant for shared memory */
int fileDescr_sm;                                    // file descriptor for the shared memory (is stored on disk "/dev/shm")
const char *sharedMemoryName = "/shared_memory";     // file name of the storing memory
char  *sharedMemory;                           // the linked memory address in current address space

/* Report Error and fre ressources */
void BailOut(const char *message)
{
    if (message != NULL)
    {
        fprintf(stderr, "%s\n", message);
    }
}

int main (int argc, char **argv)
{
    // check operants with getopt(3)
    if (argc > 0)
    {
        printf("argv: %s", argv[1]);
    }

    /* Initialize Semaphores TODO: change the initial number to a variable*/
    readSemaphore = sem_open(semaphoreReadName, O_CREAT|O_EXCL, S_IRWXU, MAPSIZE);
    if (readSemaphore == SEM_FAILED)
    {
        /* Error message */
        fprintf(stderr, "Error in creating semaphore, %s\n", strerror(errno));
        BailOut("Could not create Semaphore");
    }

    fprintf(stdout, "Semaphore on adress: %p\n", semaphoreReadName);


    // initialize shared memory
    fileDescr_sm = shm_open(sharedMemoryName, O_CREAT|O_EXCL|O_RDWR, S_IRWXU);
    if (fileDescr_sm == ERROR)
    {
        fprintf(stderr, "Error in creating shared memory, %s\n", strerror(errno));
        BailOut("Could not create shared memory");
    }

    // fixing the shared memory to a define size (coming from the agrv)
    if (ftruncate(fileDescr_sm, MAPSIZE) == ERROR)
    {
        fprintf(stderr, "Error in truncating shared memory, %s\n", strerror(errno));
        BailOut("Could not truncate shared memory");
    }

    int pagesize = sysconf(_SC_PAGE_SIZE);
    printf("Pagesize: %d\n", pagesize);
    /*blend the shared memory in current memory */
    /* TODO: MAPSIZE should be size of page */
    int pa_offset = MAPSIZE & (pagesize - 1);

     sharedMemory =  mmap(NULL, MAPSIZE, PROT_READ|PROT_WRITE,MAP_SHARED, fileDescr_sm, 0);
    if (sharedMemory == MAP_FAILED)
    {
        fprintf(stderr, "Error in mapping memory, %s\n", strerror(errno));
        BailOut("Could not map the memory");
    }

        /* Semaphore wait process */

    int semaphoreWait; 
    semaphoreWait = sem_wait(readSemaphore);
    if (semaphoreWait == ERROR)
    {
        fprintf(stderr, "Error in waiting semaphore, %s\n", strerror(errno));
        BailOut("Could not wait for Semaphore");
    }

    printf("Entering in the critical region\n");

    /* write to memory */
    sharedMemory = "This is a test";


    /* reading from memory */
    fprintf(stdout, "Read from memory: %s\n", sharedMemory);
    /* semaphore post process */
    int semaphorePost;
    semaphorePost = sem_post(readSemaphore);
    if (semaphorePost == ERROR)
    {
        fprintf(stderr, "Error in post semaphore, %s\n", strerror(errno));
        BailOut("Could not post for Semaphore\n");
    }

    printf("Exit the critical region\n");


    /* unmap the memory */
    int unmapReturn = munmap(sharedMemory, MAPSIZE);
    if (unmapReturn == ERROR)
    {
        fprintf(stderr, "Error in unmapping memory, %s\n", strerror(errno));
        BailOut("Could not unmap memory\n");
    }

    /* close the memory file */
    int closeMemory = close(fileDescr_sm);
    if (closeMemory == ERROR)
    {
        fprintf(stderr, "Error in closing memory file, %s\n", strerror(errno));
        BailOut("Could not close memory file\n");
    }

    /* unlink the memory file from /dev/shm/ */
    if (shm_unlink(sharedMemoryName) == ERROR)
    {
        fprintf(stderr, "Error in unlinking memory file, %s\n", strerror(errno));
        BailOut("Could not unlink memory file\n");
    }

    /* Closes the semaphore */
    int semaphore_close;
    semaphore_close = sem_close(readSemaphore);
    if (semaphore_close == -1)
    {
        fprintf(stderr, "Error in closing semaphore, %s\n", strerror(errno));
        BailOut("Could not close Semaphore\n");
    }

     /* unlink the semaphore, if not done, error EEXIST */
    if (sem_unlink(semaphoreReadName) == -1)
    {
        fprintf(stderr, "Error in unlinking semaphore, %s\n", strerror(errno));
        BailOut("Could not unlink Semaphore\n");
    }

    return EXIT_SUCCESS;
}