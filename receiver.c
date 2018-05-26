//
// Created by marcaurel on 11.05.18.
//
#include "sharedMemory.h"


static void BailOut(const char* message, semaphores* sems, sharedmem* shared);

/* Number of read processes */
static size_t r;

/* size of resources */
size_t buffersize = 0;

static const char* szCommand = "<not yet set>";

/* Global constant for shared memory */
// file descriptor for the shared memory (is stored on disk "/dev/shm")
// the linked memory address in current address space

/* Report Error and free resources */
void print_usage(void) {
    fprintf(stderr, "USAGE: %s [-m] length\n", szCommand);
    exit(EXIT_FAILURE);
}

/* Report Error and free resources
 * Since we are in the receiver process, we are responsable for removing all resources, even in the error case
 */
void BailOut(const char* message, semaphores* sems, sharedmem* shared) {
    if (message != NULL) {
        fprintf(stderr, "%s: %s\n", szCommand, message);
        removeRessources(sems, shared);
        exit(EXIT_FAILURE);
    }
}
/* size_t = unsigned long typedef stddef. */
size_t checkCommand(int argc, char** argv) {
    /* store the progam name */
    szCommand = argv[0];

    int opt;    // option for getop
    int bOptionM = 0;   // Flag for the 'm' option
    int bError = 0;     // Flag for Option Error
    long bufferTemp = 0;
    char* stringInt = NULL;

    if (argc < 2) {
        print_usage();
    }
    // check operants with getopt(3)
    while ((opt = getopt(argc, argv, "m:")) != -1) {
        switch (opt) {
            case 'm': {
                if (bOptionM) {
                    bError = 1;
                    break;
                }
                bOptionM = 1;
                bufferTemp = strtol(optarg, &stringInt, 10);
                break;
            }
            case '?':
                bError = 1;
                break;
            default:
                assert (0);   /* should never be reached */
                break;
        }
    }

    if (optopt == 'm')    /* parameter 'm' with no argument */
    {
        print_usage();
    }

    if (bError) {
        print_usage();
    }
    if (argc != optind) {
        print_usage();
    }
    if ((errno == ERANGE) || (strcmp(stringInt, "\0") != 0) || (bufferTemp == 0) || (bufferTemp >> 29)) {
        print_usage();
    }

    return (size_t)bufferTemp;
}

int main(int argc, char** argv) {
    semaphores sems;
    sharedmem mem;
    /* check if no paramters are given */
    buffersize = checkCommand(argc, argv);

    /* open the semaphores */
    sems = getSemaphores(buffersize);
    if (sems.readSemaphore == NULL || sems.writeSemaphore == NULL) {
        BailOut("Could not create Semaphore", &sems, NULL);
    }

    // initialize the reading int for the shared memory
    int readingInt;

    /* open the shared memory */
    mem = getSharedMem(buffersize);
    if (mem.fileDescriptor == 0 || mem.sharedMemory == NULL)
        BailOut("Could not create sharedmemory", &sems, &mem);
    /* Semaphore wait process */

    r = 0;
    while (1) {
        // write index is the same as the read index. writer must wait
        int semaphoreWait = sem_wait(sems.readSemaphore);
        if (semaphoreWait == ERROR) {
            fprintf(stderr, "Error in waiting semaphore, %s\n", strerror(errno));
            BailOut("Could not wait for Semaphore", &sems, &mem);
        }
        /* read a char from shared memory */
        readingInt = mem.sharedMemory[r];

        /* is end of file detected? */
        if (readingInt == EOF) {
            break;
        }

        // print out the char to stdout

        fputc(readingInt, stdout);
        int retval = sem_post(sems.writeSemaphore);
        // increment the counter
        r = (r + 1) % buffersize;

        if (retval == ERROR) {
            fprintf(stderr, "Error in posting semaphore, %s\n", strerror(errno));
            BailOut("Could not post for Semaphore", &sems, &mem);
        }
    }

    /* unmap the memory */
    removeRessources(&sems, &mem);

    return EXIT_SUCCESS;
}
