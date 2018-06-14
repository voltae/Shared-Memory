//
// Created by marcaurel on 11.05.18.
//

#include "sharedMemory.h"

static bool readParameters(const int argc, char* const argv[], size_t* bufferSize);
static bool transcribe(semaphores* sems, sharedmem* mem, const size_t buffersize);

int main(int argc, char* argv[]) {
    bool rc = true;
    semaphores sems = {.writeSemaphore = NULL, .readSemaphore = NULL};
    sharedmem mem = {.sharedMemory = NULL, .fileDescriptor = 0};
    size_t buffersize;

    /* Parameters */
    rc = readParameters(argc, argv, &buffersize);

    /* Semaphores */
    rc = rc && getSemaphores(buffersize, &sems);

    /* Sharedmemory */
    rc = rc && getSharedMem(buffersize, &mem);

    /* read from stdin and write to shared memory */
    rc = rc && transcribe(&sems, &mem, buffersize);

    if (!rc) {    //We only clean up if things went wrong. Otherwise its the receivers job.
        removeRessources(&sems, &mem);
        fprintf(stderr, "USAGE: %s [-m] length\n", argv[0]);
        return EXIT_FAILURE;
    } else
        return EXIT_SUCCESS;
}

static bool readParameters(const int argc, char* const argv[], size_t* bufferSize) {
    bool rc = true;
    int option;
    bool mFound = false;
    char* stringInt = NULL;


    *bufferSize = 0;
    /* check if no paramters are given */
    if (argc < 2)
        rc = false;
    else {
        while (rc && (option = getopt(argc, argv, "m:")) != -1) {
            switch (option) {
                case 'm': {
                    if (!mFound) {
                        mFound = true;
                        *bufferSize = strtol(optarg, &stringInt, 10);
                        //non numeric leftovers in the option string is a no-no and giving us a buffer size of 0 or less is just mean
                        //We don't check for ERANGE and LONG_MIN because we already test for size < 0. LONG_MIN should satisfy that. Rough estimate.
                        if ((strcmp(stringInt, "\0") != 0) || (*bufferSize <= 0) ||
                            (*bufferSize == LONG_MAX && errno == ERANGE))
                            rc = false;
                    } else //we do not allow a second "-m"
                        rc = false;

                    break;
                }
                case '?':   //other options than m are a no-no
                    rc = false;
                    break;
                default:
                    assert (0);   /* should never be reached */
                    break;
            }
        }
    }

    //don't want any other junk in my arguments.
    if (argc != optind) {
        rc = false;
    }

    return rc;
}

bool transcribe(semaphores* sems, sharedmem* mem, const size_t buffersize) {
    bool rc = true;
    int readingInt;
    size_t sharedMemoryIndex = 0;

    do {
        readingInt = fgetc(stdin);

        if (sem_wait(sems->writeSemaphore) == ERROR)
            rc = false;

        mem->sharedMemory[sharedMemoryIndex] = readingInt;

        if (sem_post(sems->readSemaphore) == ERROR)
            rc = false;

        sharedMemoryIndex = (sharedMemoryIndex + 1) % buffersize;
    } while (rc && readingInt != EOF);

    return rc;
}