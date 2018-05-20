//
// Created by marcaurel on 11.05.18.
//

#include "sharedMemory.h"

static size_t readParameters(int argc, char* const argv[]);

static void bailOut(const char* programName, const char* message);

void print_usage(const char* porgramName);

/* Number of write processes */
static unsigned int w;

/* Global constant for shared memory */
// file descriptor for the shared memory (is stored on disk "/dev/shm")
// the linked memory address in current address space

int main(int argc, char* argv[]) {
    size_t buffersize = 0;
    semaphores sems;
    sharedmem mem;

    buffersize = readParameters(argc, argv);

    /* open the semaphores */
    sems = getSemaphores(buffersize);
    if(sems.readSemaphore == NULL && sems.writeSemaphore == 0)
        bailOut(argv[0], "Could not create semaphore");

    /* Create shared Memory */
    mem = getSharedMem(buffersize, O_RDWR);
    if(mem.fileDescriptor == 0 || mem.sharedMemory == NULL)
        bailOut(argv[0], "Could not create sharedmemory");
    // initialize the reading char for the shared memory
    short int readingChar;

    while ((readingChar = fgetc(stdin)) != EOF) {
        // write index is the same as the read index. writer must wait
        short int semaphoreWait = sem_wait(sems.writeSemaphore);
        if (semaphoreWait == ERROR) {
            fprintf(stderr, "Error in waiting semaphore, %s\n", strerror(errno));
            bailOut(argv[0], "Could not wait for Semaphore");
        }
        /* read a char from shared memory */
        mem.sharedMemory[w] = readingChar;

        int retval = sem_post(sems.readSemaphore);
        if (retval == ERROR) {
            fprintf(stderr, "Error in posting semaphore, %s\n", strerror(errno));
            bailOut(argv[0], "Could not post for Semaphore");
        }
        // increment the counter
        w = (w + 1) % buffersize;
    }

    /* Signalize the receiver that reading is not yet over EOF! has to be read */
    mem.sharedMemory[w] = EOF;
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
void bailOut(const char* programName, const char* message) {
    if (message != NULL) {
        fprintf(stderr, "%s: %s\n", programName, message);
    }

}