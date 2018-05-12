//
// Created by marcaurel on 11.05.18.
//

#include "common.h"
#define ERROR (-1)

/* Global constant semaphore read */
sem_t *readSemaphore;
sem_t *writeSemaphore;

/* Global constant for shared memory */
                                // file descriptor for the shared memory (is stored on disk "/dev/shm")
                          // the linked memory address in current address space

/* Report Error and fre ressources */

int main (int argc, char **argv)
{
    char  *sharedMemory;
    size_t buffersize = 0; // buffersize of memory
    int opt;    // option for getop
    char **nonDigits = NULL;

    // check operants with getopt(3)
    while ((opt = getopt(argc,argv,"m:")) != ERROR)  {
        long bufferTemp;
        switch (opt)
        {
            case 'm':
                bufferTemp = strtol(optarg,nonDigits, 10);
                if ((bufferTemp == ERROR) || *nonDigits != NULL)
                {
                    /* error handing */
                }
                buffersize = (size_t)bufferTemp;
                break;
            default:
                fprintf(stderr, "Usage: %s [-m] length\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    /* Initialize Semaphores TODO: change the initial number to a variable*/
    readSemaphore = createSemaphore(buffersize, 'r');

    fprintf(stdout, "Semaphore on adress: %p\n", semaphoreReadName);


    // initialize shared memory
    sharedMemory = createSharedMemory(buffersize);

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
    closeSharedMemory(sharedMemory, buffersize);

    /* Closes the semaphore */
   closeSemaphores(readSemaphore, 'r');

    return EXIT_SUCCESS;
}