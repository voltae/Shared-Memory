//
// Created by marcaurel on 11.05.18.
//

#include "common.h"
#define ERROR (-1)

/* Global constant semaphore read */
//char semaphoreReadName[NAMELLENGTH];  // Semaphore read Name with own annuminas uid
//char semaphoreWriteName[NAMELLENGTH];  // Semaphore write Name with own annuminas uid
//char sharedMemoryName[NAMELLENGTH];     // Shared memory name

static sem_t *readSemaphore_local = NULL;
static sem_t *writeSemaphore_local = NULL;
static char *sharedMemory_local = NULL;

unsigned int r;
unsigned int w;

static char *szCommand = "<not yet set>";

/* Global constant for shared memory */
// file descriptor for the shared memory (is stored on disk "/dev/shm")
// the linked memory address in current address space

/* Report Error and free ressources */
void print_usage(void)
{
    fprintf(stderr, "Usage: %s [-m] length\n", szCommand);
    exit(EXIT_FAILURE);
}
int main (int argc, char **argv)
{
    /* store the progam name */
    szCommand = argv[0];

    size_t buffersize = 0; // buffersize of memory
    int opt;    // option for getop
    //char **nonDigits = NULL;
    long bufferTemp;

    /* check if agruments are more 1 */
    if (argc < 2)
    {
        print_usage();
    }
    // check operants with getopt(3)
    while ((opt = getopt(argc,argv,"m:")) != -1)  {
        switch (opt)
        {
            case 'm':
                bufferTemp = strtol(optarg,NULL, 10);
                if ((bufferTemp == ERROR) || bufferTemp < 1)
                {
                    /* error handing */
                    print_usage();
                }
                buffersize = (size_t)bufferTemp;
                break;
            default:
                print_usage();
                break;
        }
    }

    if (strcmp(semaphoreReadName, "\0") == 0)
    {
        setRessourcesName();
    }

    /* Initialize Semaphores TODO: change the initial number to a variable*/
    if (readSemaphore_local == NULL)
    { readSemaphore_local = sem_open(semaphoreReadName, 0); }

    /* Initialize Semaphores TODO: change the initial number to a variable*/
    if (writeSemaphore_local == NULL)
    { writeSemaphore_local = sem_open(semaphoreWriteName, 0); }

    // initialize the reader index
    r = buffersize - 1;

    // initialize the reading char for the shared memory
    char readingChar;

    // initialize shared memory
    if (sharedMemory_local == NULL)
    {
        int shmfd = shm_open(sharedMemoryName, O_RDONLY, S_IRWXU);
        if (shmfd == ERROR)
        {
            fprintf(stderr, "Error in creating shared memory, %s\n", strerror(errno));
            BailOut("Could not create shared memory");
        }

        // fixing the shared memory to a define size (coming from the agrv)
        if (ftruncate(shmfd, buffersize * sizeof(char)) == ERROR)
        {
            fprintf(stderr, "%s: Error in truncating shared memory: %lu, %s\n", szCommand, buffersize* sizeof(char), strerror(errno));
            BailOut("Could not truncate shared memory");
        }

        /*blend the shared memory in current memory */
        /* TODO: buffersize should be size of page */

        /* blend the memory in READ ONLY */
        sharedMemory_local = mmap(NULL, buffersize * sizeof(char), PROT_READ, MAP_SHARED, shmfd, 0);
        if (sharedMemory_local == MAP_FAILED)
        {
            fprintf(stderr, "%s: Error in mapping memory, %s\n", szCommand, strerror(errno));
            BailOut("Could not map the memory");
        }

    }

    /* Semaphore wait process */

    int retVal;
    retVal = sem_wait(readSemaphore_local);
    if (retVal == ERROR)
    {
        fprintf(stderr, "%s: Error in waiting semaphore, %s\n", szCommand, strerror(errno));
        BailOut("Could not wait for Semaphore");
    }

    printf("Entering in the critical region\n");

    while (1)
    {
        // write index is the same as the read index. writer must wait
        if (((w + 1) % buffersize) == r)
        {
            int semaphoreWait;
            semaphoreWait = sem_wait(writeSemaphore_local);
            if (semaphoreWait == ERROR)
            {
                fprintf(stderr, "Error in waiting semaphore, %s\n", strerror(errno));
                BailOut("Could not wait for Semaphore");
            }
        }

        if (sharedMemory_local[r] == EOF)
        {
            exit(1);
        }

        // print out the char to stdout
        readingChar = sharedMemory_local[r];
        fputc(readingChar, stdout);

                // increment the counter
        r = (r + 1) % buffersize;

        int sem_write;
        // if the writer process is asleep, wake it up
        sem_getvalue(writeSemaphore_local,&sem_write);
        if (sem_write == 0)
        {
            int retval = sem_post(writeSemaphore_local);
            {
                if (retval == ERROR)
                {
                    fprintf(stderr, "Error in posting semaphore, %s\n", strerror(errno));
                    BailOut("Could not post for Semaphore");
                }
            }
        }
    }


    /* unmap the memory */
    closeSharedMemory(sharedMemory_local);

    /* Closes the semaphore */
    closeSemaphores();

    return EXIT_SUCCESS;
}