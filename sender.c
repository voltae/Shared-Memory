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
#include <assert.h>

#define NAMELLENGTH 14

typedef enum isError {
    ERROR = -1, SUCCESS
} isError;

/* Global constant semaphore read */
static char semaphoreReadName[NAMELLENGTH];  // Semaphore read Name with own annuminas uid
static char semaphoreWriteName[NAMELLENGTH];  // Semaphore write Name with own annuminas uid
static char sharedMemoryName[NAMELLENGTH];     // Shared memory name

static void setRessourcesName(void);

static void createSemaphores(size_t size,  const char* programName);
static void createSharedMemory(size_t size, const char* programName);

//static void removeRessources (void);
static void bailOut(const char* porgramName, const char* message);

static sem_t* readSemaphore = NULL;
static sem_t* writeSemaphore = NULL;
static char* sharedMemory = NULL;

/* Number of write processes */
static unsigned int w;
/* file descriptor for the shared memory */
static int fileDescr_sm;

/* Global constant for shared memory */
// file descriptor for the shared memory (is stored on disk "/dev/shm")
// the linked memory address in current address space

/* Report Error and free ressources */
void print_usage(const char* porgramName) {
    fprintf(stderr, "USAGE: %s [-m] length\n", porgramName);
    exit(EXIT_FAILURE);
}

/* Report Error and allocate ressources
 * Since we are in the sender process, we are responsable for allocating all ressources,
 */
void bailOut(const char* porgramName, const char* message) {
    if (message != NULL) {
        fprintf(stderr, "%s: %s\n", porgramName, message);
    }

}

int main(int argc, char** argv) {
    int opt;    // option for getop
    int bOptionM = 0;   // Flag for the 'm' option
    int bError = 0;     // Flag for Option Error
    size_t buffersize = 0;
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
    printf("argc: %d, optind: %d\n", argc, optind);
    if (bError) {
        print_usage(argv[0]);
    }
    if (argc != optind) {
        print_usage(argv[0]);
    }
    if ((errno == ERANGE) || (strcmp(stringInt, "\0") != 0) || (bufferTemp == 0) || (bufferTemp >> 29)) {
        print_usage(argv[0]);
    }

    buffersize = (size_t) bufferTemp;

    /* Set the ressource names */
    setRessourcesName();
    /* open the semaphores */
    createSemaphores(buffersize, NULL);
    /* Create shared Memory */
    createSharedMemory(buffersize, NULL);
    // initialize the reading char for the shared memory
    char readingChar;

    while ((readingChar = fgetc(stdin)) != EOF) {
        // write index is the same as the read index. writer must wait
        int semaphoreWait = sem_wait(writeSemaphore);
        if (semaphoreWait == ERROR) {
            fprintf(stderr, "Error in waiting semaphore, %s\n", strerror(errno));
            bailOut(argv[0], "Could not wait for Semaphore");
        }
        /* read a char from shared memory */
        sharedMemory[w] = readingChar;

        int retval = sem_post(readSemaphore);
        if (retval == ERROR) {
            fprintf(stderr, "Error in posting semaphore, %s\n", strerror(errno));
            bailOut(argv[0], "Could not post for Semaphore");
        }
        // increment the counter
        w = (w + 1) % buffersize;
    }

    /* Signalize the receiver that reading is not yet over EOF! has to be read */
    sharedMemory[w] = EOF;
    sem_post(readSemaphore);

    return EXIT_SUCCESS;
}

static void setRessourcesName(void) {

    int uid = getuid();  // These functions are always successful. man7

    /* create read semaphore */
    snprintf(semaphoreReadName, 13, "/sem_%d", 1000 * uid + 0);
    snprintf(semaphoreWriteName, 13, "/sem_%d", 1000 * uid + 1);
    snprintf(sharedMemoryName, 13, "/shm_%d", 1000 * uid + 0);
}

static void createSemaphores(size_t size, const char* programName) {
    if (readSemaphore != NULL) {
        return;
    }

    /* Open a new read semaphore */
    readSemaphore = sem_open(semaphoreReadName, O_CREAT | O_EXCL, S_IRWXU, 0);
    if (readSemaphore == SEM_FAILED) {
        /* Error message */
        fprintf(stderr, "Error in creating semaphore, %s\n", strerror(errno));
        bailOut(programName, "Could not create Semaphore");
    }

    if (writeSemaphore != NULL) {
        return;
    }

    /* create a new write semaphore */
    writeSemaphore = sem_open(semaphoreWriteName, O_CREAT | O_EXCL, S_IRWXU, size);
    if (writeSemaphore == SEM_FAILED) {
        /* Error message */
        fprintf(stderr, "Error in creating semaphore, %s\n", strerror(errno));
        bailOut(programName, "Could not create Semaphore");
    }
}

/* Create a new shared memory, it should exist */
static void createSharedMemory(size_t size, const char* programName) {
    if (sharedMemory != NULL) {
        return;
    }

    if (strcmp(sharedMemoryName, "\0") == 0) {
        setRessourcesName();
    }

    // initialize shared memory
    fileDescr_sm = shm_open(sharedMemoryName, O_CREAT | O_EXCL | O_RDWR, S_IRWXU);
    if (fileDescr_sm == ERROR) {
        fprintf(stderr, "Error in opening shared memory, %s\n", strerror(errno));
        bailOut(programName, "Could not create shared memory");
    }

    // fixing the shared memory to a define size (coming from the agrv)
    int returrnVal = ftruncate(fileDescr_sm, size * sizeof(char));
    if (returrnVal == ERROR) {
        fprintf(stderr, "Error in truncating shared memory, %s\n", strerror(errno));
        bailOut(programName, "Could not truncate shared memory");
    }

    sharedMemory = mmap(NULL, size * sizeof(char), PROT_WRITE, MAP_SHARED, fileDescr_sm, 0);
    if (sharedMemory == MAP_FAILED) {
        fprintf(stderr, "Error in mapping memory, %s\n", strerror(errno));
        bailOut(programName, "Could not map the memory");
    }
}