//
// Created by marcaurel on 11.05.18.
//

#include "sharedMemory.h"

static size_t readParameters(int argc, char* const argv[]);

static void bailOut(const char* programName, const char* message, size_t size, semaphores* sems, sharedmem* shared);

void print_usage(const char* porgramName);


/* Global constant for shared memory */
// file descriptor for the shared memory (is stored on disk "/dev/shm")
// the linked memory address in current address space

int main(int argc, char* argv[]) {
    semaphores sems;
    sharedmem mem;
    unsigned int sharedMemoryIndex = 0;
    int readingInt;

    /* Parameters */
    size_t buffersize = readParameters(argc, argv);

    /* Semaphores */
    sems = getSemaphores(buffersize);
    if(sems.readSemaphore == NULL && sems.writeSemaphore == NULL)
        bailOut(argv[0], "Could not create semaphore", buffersize, &sems, NULL);

    /* Sharedmemory */
    mem = getSharedMem(buffersize);
    if(mem.fileDescriptor == 0 || mem.sharedMemory == NULL)
        bailOut(argv[0], "Could not create sharedmemory", buffersize, &sems, &mem);

    /* read from stdin and write to shared memory */
    while ((readingInt = fgetc(stdin)) != EOF) {
        // write index is the same as the read index. writer must wait
        int semaphoreWait = sem_wait(sems.writeSemaphore);
        if (semaphoreWait == ERROR) {
            fprintf(stderr, "Error in waiting semaphore, %s\n", strerror(errno));
            bailOut(argv[0], "Could not wait for Semaphore", buffersize, &sems, &mem);
        }
        /* write a char to shared memory */
        //mem.sharedMemory[sharedMemoryIndex] = readingInt;
        // using memcpy instead of direct
        if (memcpy((mem.sharedMemory + sharedMemoryIndex), &readingInt, 1) == NULL)
        {
            bailOut(argv[0], "Could not write ot memory", buffersize, &sems, &mem);
        }

        int retval = sem_post(sems.readSemaphore);
        if (retval == ERROR) {
            fprintf(stderr, "Error in posting semaphore, %s\n", strerror(errno));
            bailOut(argv[0], "Could not post for Semaphore", buffersize, &sems, &mem);
        }
        // increment the counter
        sharedMemoryIndex = (sharedMemoryIndex + 1) % buffersize;
    }

    /* Send EOF to the receiver. We are done. */
    mem.sharedMemory[sharedMemoryIndex] = EOF;
    sem_post(sems.readSemaphore);

    return EXIT_SUCCESS;
}

static size_t readParameters(const int argc, char* const argv[]) {
    int opt;    // option for getop
    int bOptionM = 0;   // Flag for the 'm' option
    int bError = 0;     // Flag for Option Error
    long bufferTemp = 0;
    char* stringInt = NULL;

    /* check if no paramters are given */
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
                print_usage(argv[0]);
                break;
            default:
                assert (0);   /* should never be reached */
                break;
        }
    }

    if (bError) {
        print_usage(argv[0]);
    }
    if (argc != optind) {
        print_usage(argv[0]);
    }   //TODO: revisit. hardcoding the 29 is probably not the way to go
    if ((errno == ERANGE) || (strcmp(stringInt, "\0") != 0) || (bufferTemp == 0) || (bufferTemp >> 29)) {
        print_usage(argv[0]);
    }

    return (size_t) bufferTemp;
}

/* Report Error and free ressources */
void print_usage(const char* porgramName) {
    fprintf(stderr, "USAGE: %s [-m] length\n", porgramName);
    exit(EXIT_FAILURE);
}

/* Report Error and allocate ressources
 * Since we are in the sender process, we are responsable for allocating all ressources,
 */
void bailOut(const char* programName, const char* message, size_t size, semaphores* sems, sharedmem* shared) {
    removeRessources(sems, shared);
    if (message != NULL)
        fprintf(stderr, "%s: %s\n", programName, message);
    exit(EXIT_FAILURE);
}