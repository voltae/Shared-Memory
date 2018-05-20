//
// Created by manuel on 20/05/18.
//

#include "sharedMemory.h"

static void setRessourcesNames(void);

/* Global constant semaphore read */
static char semaphoreReadName[NAMELLENGTH];  // Semaphore read Name with own annuminas uid
static char semaphoreWriteName[NAMELLENGTH];  // Semaphore write Name with own annuminas uid
static char sharedMemoryName[NAMELLENGTH];     // Shared memory name

static sem_t* readSemaphore;
static sem_t* writeSemaphore;

static short int* sharedMemory;
static int fileDescr;

//TODO: rewrite into one errorhandling path
semaphores getSemaphores(size_t size) {
    semaphores sems;

    if (strcmp(semaphoreReadName, "\0") == 0)
        setRessourcesNames();

    if (readSemaphore != NULL)
        sems.readSemaphore = readSemaphore;
    else {
        readSemaphore = sem_open(semaphoreReadName, O_CREAT | O_EXCL, S_IRWXU, 0);
        if (readSemaphore == SEM_FAILED) {
            /* Error message */
            fprintf(stderr, "Sender: Error in creating read-semaphore, %s\n", strerror(errno));
            sems.readSemaphore = NULL;
            sems.writeSemaphore = NULL;
            return sems;
        }
    }

    if (writeSemaphore != NULL)
        sems.writeSemaphore = writeSemaphore;
    else {
        writeSemaphore = sem_open(semaphoreWriteName, O_CREAT | O_EXCL, S_IRWXU, size);
        if (writeSemaphore == SEM_FAILED) {
            /* Error message */
            fprintf(stderr, "Sender: Error in creating write-semaphore, %s\n", strerror(errno));
            sems.readSemaphore = NULL;
            sems.writeSemaphore = NULL;
            return sems;
        }
    }

    sems.readSemaphore = readSemaphore;
    sems.writeSemaphore = writeSemaphore;
    return sems;
}

sharedmem getSharedMem(size_t size) {
    sharedmem shared;

    if (sharedMemory != NULL) {
        shared.sharedMemory = sharedMemory;
        shared.fileDescriptor = fileDescr;
    } else {
        if (strcmp(sharedMemoryName, "\0") == 0) {
            setRessourcesNames();
        }

        // initialize shared memory
        fileDescr = shm_open(sharedMemoryName, O_CREAT | O_EXCL | O_RDWR, S_IRWXU);
        if (fileDescr == ERROR) {
            fprintf(stderr, "Sender: Error in opening shared memory, %s\n", strerror(errno));
            shared.sharedMemory = NULL;
            shared.fileDescriptor = 0;
            return shared;
        }

        // fixing the shared memory to a define size (coming from the agrv)
        int returrnVal = ftruncate(fileDescr, size * sizeof(short));
        if (returrnVal == ERROR) {
            fprintf(stderr, "Error in truncating shared memory, %s\n", strerror(errno));
            shared.sharedMemory = NULL;
            shared.fileDescriptor = 0;
            return shared;
        }

        sharedMemory = mmap(NULL, size * sizeof(short), PROT_WRITE, MAP_SHARED, fileDescr, 0);
        if (sharedMemory == MAP_FAILED) {
            fprintf(stderr, "Error in mapping memory, %s\n", strerror(errno));
            shared.sharedMemory = NULL;
            shared.fileDescriptor = 0;
            return shared;
        }
    }

    return shared;
}

static void setRessourcesNames(void) {

    int uid = getuid();  // These functions are always successful. man7

    /* create read semaphore */
    snprintf(semaphoreReadName, NAMELLENGTH, "/sem_%d", 1000 * uid + 0);
    snprintf(semaphoreWriteName, NAMELLENGTH, "/sem_%d", 1000 * uid + 1);
    snprintf(sharedMemoryName, NAMELLENGTH, "/shm_%d", 1000 * uid + 0);
}