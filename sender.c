//
// Created by marcaurel on 11.05.18.
//


#include "sharedMemory.h"

static bool readParameters(const int argc, char* const argv[], size_t* bufferSize);
static bool transcribe(semaphores* sems, sharedmem* mem, const size_t buffersize);

/* Global constant for shared memory */
// file descriptor for the shared memory (is stored on disk "/dev/shm")
// the linked memory address in current address space

int main(int argc, char* argv[]) {
    bool rc = true;
    semaphores sems = {.writeSemaphore = NULL, .readSemaphore = NULL};
    sharedmem mem = {.sharedMemory = NULL, .fileDescriptor = 0};
    size_t buffersize;

    /* Parameters */
    rc = readParameters(argc, argv, &buffersize);
#ifdef DEBUG
    fprintf(stderr, "rc nach readParam: %i\n", rc);
#endif
    /* Semaphores */
    rc = rc && getSemaphores(buffersize, &sems);
#ifdef DEBUG
    fprintf(stderr, "rc nach getSems: %i\n", rc);
#endif
    /* Sharedmemory */
    rc = rc && getSharedMem(buffersize, &mem);
#ifdef DEBUG
    fprintf(stderr, "rc nach getShared: %i\n", rc);
#endif
    /* read from stdin and write to shared memory */
    rc = rc && transcribe(&sems, &mem, buffersize);
#ifdef DEBUG
    fprintf(stderr, "rc nach transcribe: %i\n", rc);
#endif

    if (!rc) {    //We only clean up if things went wrong. Otherwise its the receivers job.
        removeRessources(&sems, &mem);
        fprintf(stderr, "USAGE: %s [-m] length\n", argv[0]);
        return EXIT_FAILURE;
    }
    else
        return EXIT_SUCCESS;
}

static bool readParameters(const int argc, char* const argv[], size_t* bufferSize) {
    bool rc = true;
    int opt;    // option for getop
    int bOptionM = 0;   // Flag for the 'm' option
    char* stringInt = NULL;


    *bufferSize = 0;
    /* check if no paramters are given */
    if (argc < 2)
        rc = false;
    else{
        while (rc && (opt = getopt(argc, argv, "m:")) != -1) {
            switch (opt) {
                case 'm': {
                    if (bOptionM) { //we do not allow a second "-m"
                        rc = false;
                        break;
                    }
                    bOptionM = 1;
                    *bufferSize = strtol(optarg, &stringInt, 10);
                    if (strcmp(stringInt, "\0") != 0)   //non numeric leftovers in the option string is a no-no
                        rc = false;
                    break;
                }
                case '?':
                    rc = false;
                    break;
                default:
                    assert (0);   /* should never be reached */
                    break;
            }
        }
    }

    if ((argc != optind) || (errno == ERANGE) || (*bufferSize <= 0)) {
        rc = false;
    }

    return rc;
}

bool transcribe(semaphores* sems, sharedmem* mem, const size_t buffersize){
    bool rc = true;
    int readingInt;
    size_t sharedMemoryIndex = 0;

    do {
        readingInt = fgetc(stdin);

        // write index is the same as the read index. writer must wait
        int semaphoreWait = sem_wait(sems->writeSemaphore);
        if (semaphoreWait == ERROR)
            rc = false;

        /* write a char to shared memory */
        mem->sharedMemory[sharedMemoryIndex] = readingInt;

        int retval = sem_post(sems->readSemaphore);
        if (retval == ERROR)
            rc = false;

        // increment the counter
        sharedMemoryIndex = (sharedMemoryIndex + 1) % buffersize;
    } while (rc && readingInt != EOF);

    return rc;
}