//
// Created by marcaurel on 11.05.18.
//
#include "sharedMemory.h"


static void BailOut(const char* progname, const char* message, semaphores* sems, sharedmem* shared);
// file descriptor for the shared memory (is stored on disk "/dev/shm")
// the linked memory address in current address space

/* Report Error and free resources */
void print_usage(const char *progname) {
    fprintf(stderr, "USAGE: %s [-m] length\n", progname);
    exit(EXIT_FAILURE);
}

/* Report Error and free resources
 * Since we are in the receiver process, we are responsable for removing all resources, even in the error case
 */
void BailOut(const char* progname, const char* message, semaphores* sems, sharedmem* shared) {
    if (message != NULL) {
        fprintf(stderr, "%s: %s\n", progname, message);
        removeRessources(sems, shared);
        exit(EXIT_FAILURE);
    }
}
/* size_t = unsigned long typedef stddef. */
size_t checkCommand(int argc, char** argv) {
    int opt;    // option for getop
    int bOptionM = 0;   // Flag for the 'm' option
    int bError = 0;     // Flag for Option Error
    long bufferTemp = 0;
    char* stringInt = NULL;

    if (argc < 2) {
        print_usage(argv[0]);
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
        print_usage(argv[0]);
    }

    if (bError) {
        print_usage(argv[0]);
    }
    if (argc != optind) {
        print_usage(argv[0]);
    }
    if ((errno == ERANGE) || (strcmp(stringInt, "\0") != 0) || (bufferTemp == 0) || (bufferTemp >> 29)) {
        print_usage(argv[0]);
    }

    return (size_t)bufferTemp;
}

int main(int argc, char** argv) {
    semaphores sems;
    sharedmem mem;

    /* buffer size allocated, stackvariable */
    size_t buffersize = 0;

    /* Number of read processes, stack variable */
    size_t readingIndex = 0;
    // initialize the reading int for the shared memory
    int readingInt = 0;

    /* check if no parameters are given */
    buffersize = checkCommand(argc, argv);

    /* open the semaphores */
    if (!getSemaphores(buffersize, &sems)) {
        BailOut("Could not create Semaphore", argv[0], &sems, NULL);
    }

    /* open the shared memory */
    if (!getSharedMem(buffersize, &mem))
        BailOut("Could not create sharedmemory",argv[0], &sems, &mem);

    while (readingInt != EOF) {
        // write index is the same as the read index. writer must wait
        int semaphoreWait = sem_wait(sems.readSemaphore);
        if (semaphoreWait == ERROR) {
            fprintf(stderr, "Error in waiting semaphore, %s\n", strerror(errno));
            BailOut("Could not wait for Semaphore",argv[0], &sems, &mem);
        }
        /* read the next character from buffer for the next iteration */
        readingInt = mem.sharedMemory[readingIndex];
        // print out the char to stdout
        if(readingInt != EOF)
            fputc(readingInt, stdout);

        /* increment the buffer counter */
        readingIndex = (readingIndex + 1) % buffersize;

        int retval = sem_post(sems.writeSemaphore);
        // increment the counter
        if (retval == ERROR) {
            fprintf(stderr, "Error in posting semaphore, %s\n", strerror(errno));
            BailOut("Could not post for Semaphore",argv[0], &sems, &mem);
        }

    }

    /* unmap the memory */
    removeRessources(&sems, &mem);

    return EXIT_SUCCESS;
}
