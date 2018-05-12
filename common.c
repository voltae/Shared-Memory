//
// Created by marcaurel on 12.05.18.
//

/* Code for initializing all resources goes here */
//
// Created by marcaurel on 11.05.18.
//

#include "common.h"


/* global because close need the file descriptor for shared memory as well */
static int fileDescr_sm;                                    // file descriptor for the shared memory (is stored on disk "/dev/shm")
char semaphoreReadName[NAMELLENGTH];  // Semaphore read Name with own annuminas uid
char semaphoreWriteName[NAMELLENGTH];  // Semaphore write Name with own annuminas uid
char sharedMemoryName[NAMELLENGTH];

/* Report Error and free ressources */
void BailOut(const char *message)
{
    if (message != NULL)
    {
        fprintf(stderr, "%s\n", message);
    }
}

extern sem_t *createSemaphore (size_t buffersize, char type)
{
    sem_t *returnSemaphore;


    int uid = getuid();  // These functions are always successful. man7

    switch (type)
    {

        case 'r':
        {
            snprintf(semaphoreReadName, 13, "/sem_%d", 1000 * uid + 0);
            /* Initialize Semaphores TODO: change the initial number to a variable*/
            returnSemaphore = sem_open(semaphoreReadName, O_CREAT | O_EXCL, S_IRWXU, buffersize);
            if (returnSemaphore == SEM_FAILED)
            {
                /* Error message */
                fprintf(stderr, "Error in creating semaphore, %s\n", strerror(errno));
                BailOut("Could not create Semaphore");
                returnSemaphore = NULL;
            }
            break;
        }
        case 'w':
        {
            snprintf(semaphoreWriteName, 13, "/sem_%d", 1000 * uid + 1);
            /* Initialize Semaphores TODO: change the initial number to a variable*/
            returnSemaphore = sem_open(semaphoreWriteName, O_CREAT | O_EXCL, S_IRWXU, buffersize);
            if (returnSemaphore == SEM_FAILED)
            {
                /* Error message */
                fprintf(stderr, "Error in creating semaphore, %s\n", strerror(errno));
                BailOut("Could not create Semaphore");
                returnSemaphore = NULL;
            }
            break;
        }
    }
    fprintf(stdout, "Semaphore on adress: %p\n", semaphoreReadName);
    return returnSemaphore;
}

extern char *createSharedMemory (size_t buffersize)
{

    /* pointer to the shared memory, to be returned */
    char *sharedMemory = NULL;

    /* create the name for the shared memory */
    int uid = getuid();
    snprintf(sharedMemoryName, 13, "/shm_%d", 1000*uid+0);

    // initialize shared memory
    fileDescr_sm = shm_open(sharedMemoryName, O_CREAT | O_EXCL | O_RDWR, S_IRWXU);
    if (fileDescr_sm == ERROR)
    {
        fprintf(stderr, "Error in creating shared memory, %s\n", strerror(errno));
        BailOut("Could not create shared memory");
    }

    // fixing the shared memory to a define size (coming from the agrv)
    if (ftruncate(fileDescr_sm, buffersize) == ERROR)
    {
        fprintf(stderr, "Error in truncating shared memory, %s\n", strerror(errno));
        BailOut("Could not truncate shared memory");
    }

    int pagesize = sysconf(_SC_PAGE_SIZE);
    printf("Pagesize: %d\n", pagesize);
    /*blend the shared memory in current memory */
    /* TODO: buffersize should be size of page */

    sharedMemory = mmap(NULL, buffersize, PROT_READ | PROT_WRITE, MAP_SHARED, fileDescr_sm, 0);
    if (sharedMemory == MAP_FAILED)
    {
        fprintf(stderr, "Error in mapping memory, %s\n", strerror(errno));
        BailOut("Could not map the memory");
    }

    return sharedMemory;
}

extern isError closeSharedMemory (char *sharedMemory, size_t buffersize)
{
    /* unmap the memory */
    /* TODO: unmap falis every time, but the memory is deleted */
    int unmapReturn = munmap(sharedMemory, buffersize);
    if (unmapReturn == ERROR)
    {
        fprintf(stderr, "Error in unmapping memory, %s\n", strerror(errno));
        BailOut("Could not unmap memory\n");
        //return ERROR;
    }

    /* close the memory file */
    int closeMemory = close(fileDescr_sm);
    if (closeMemory == ERROR)
    {
        fprintf(stderr, "Error in closing memory file, %s\n", strerror(errno));
        BailOut("Could not close memory file\n");
        return ERROR;
    }

    /* unlink the memory file from /dev/shm/ */
    if (shm_unlink(sharedMemoryName) == ERROR)
    {
        fprintf(stderr, "Error in unlinking memory file, %s\n", strerror(errno));
        BailOut("Could not unlink memory file\n");
        return ERROR;
    }

    return SUCCESS;
}
extern isError closeSemaphores (sem_t *semaphore, char type)
{
    /* Closes the semaphore */

    isError returnValue;
    switch (type)
    {
        case 'r':
        {
            int semaphore_close;
            semaphore_close = sem_close(semaphore);
            if (semaphore_close == -1)
            {
                fprintf(stderr, "Error in closing read semaphore, %s\n", strerror(errno));
                BailOut("Could not close read Semaphore\n");
                returnValue = ERROR;
            }

            /* unlink the semaphore, if not done, error EEXIST */
            if (sem_unlink(semaphoreReadName) == -1)
            {
                fprintf(stderr, "Error in unlinking read semaphore, %s\n", strerror(errno));
                BailOut("Could not unlink read Semaphore\n");
                returnValue = ERROR;
            }
            else
                returnValue = SUCCESS;
            break;
        }
        case 'w':
        {
            int semaphore_close;
            semaphore_close = sem_close(semaphore);
            if (semaphore_close == -1)
            {
                fprintf(stderr, "Error in closing write semaphore, %s\n", strerror(errno));
                BailOut("Could not close write Semaphore\n");
                returnValue = ERROR;
            }

            /* unlink the semaphore, if not done, error EEXIST */
            if (sem_unlink(semaphoreWriteName) == -1)
            {
                fprintf(stderr, "Error in unlinking write semaphore, %s\n", strerror(errno));
                BailOut("Could not unlink write Semaphore\n");
                returnValue = ERROR;
            }
            else
                returnValue = SUCCESS;
            break;
        }
    }
    return returnValue;
}