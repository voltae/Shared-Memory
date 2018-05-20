//
// Created by manuel on 20/05/18.
//

#include "sharedMemory.h"

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

    int uid = getuid();  // These functions are always successful. man7
    snprintf(semaphoreReadName, NAMELLENGTH, "/sem_%d", 1000 * uid + 0);
    snprintf(semaphoreWriteName, NAMELLENGTH, "/sem_%d", 1000 * uid + 1);

    readSemaphore = sem_open(semaphoreReadName, O_CREAT | O_EXCL, S_IRWXU, 0);
    if (readSemaphore == SEM_FAILED) {
        if (errno == EEXIST)   //This is ok, someone else created our semaphores for us. How nice!
            readSemaphore = sem_open(semaphoreReadName, O_RDWR, 0);

        if (readSemaphore == SEM_FAILED) {   //either from the first or from the second sem_open call.
            fprintf(stderr, "Sender: Error in creating read-semaphore, %s\n", strerror(errno));
            sems.readSemaphore = NULL;
            sems.writeSemaphore = NULL;
            return sems;
        }

    }

    writeSemaphore = sem_open(semaphoreWriteName, O_CREAT | O_EXCL, S_IRWXU, size);
    if (writeSemaphore == SEM_FAILED) {
        if(errno == EEXIST)
            writeSemaphore = sem_open(semaphoreWriteName, O_RDWR, 0);

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

//TODO: rewrite to single error path
sharedmem getSharedMem(size_t size, int flag) {
    sharedmem shared;

    int uid = getuid();  // These functions are always successful. man7
    snprintf(sharedMemoryName, NAMELLENGTH, "/shm_%d", 1000 * uid + 0);

    // initialize shared memory
    fileDescr = shm_open(sharedMemoryName, O_CREAT | O_EXCL | flag, S_IRWXU);
    if (fileDescr == ERROR) {
        if(errno == EEXIST)
            fileDescr = shm_open (sharedMemoryName, flag, 0);

        if(fileDescr == ERROR){
            fprintf(stderr, "Sender: Error in opening shared memory, %s\n", strerror(errno));
            shared.sharedMemory = NULL;
            shared.fileDescriptor = 0;
            return shared;
        }
    } else {
        // fixing the shared memory to a define size (coming from the agrv)
        int returrnVal = ftruncate(fileDescr, size * sizeof(short));
        if (returrnVal == ERROR) {
            fprintf(stderr, "Error in truncating shared memory, %s\n", strerror(errno));
            shared.sharedMemory = NULL;
            shared.fileDescriptor = 0;
            return shared;
        }
    }

    sharedMemory = mmap(NULL, size * sizeof(short), PROT_WRITE, MAP_SHARED, fileDescr, 0);
    if (sharedMemory == MAP_FAILED) {
        fprintf(stderr, "Error in mapping memory, %s\n", strerror(errno));
        shared.sharedMemory = NULL;
        shared.fileDescriptor = 0;
        return shared;
    }

    shared.sharedMemory = sharedMemory;
    shared.fileDescriptor = fileDescr;
    return shared;
}

void removeRessources(size_t size) {
    /* Check if the read semaphore pointer exists */
    if (readSemaphore != NULL) {
        /* close the read semaphore */
        int semaphore_read_close;
        semaphore_read_close = sem_close(readSemaphore);
        if (semaphore_read_close == -1) {
            fprintf(stderr, "Error in closing read semaphore, %s\n", strerror(errno));
        }


        /* unlink the read semaphore */
        int semaphore_read_unlink = sem_unlink(semaphoreReadName);
        if (semaphore_read_unlink == -1) {
            fprintf(stderr, "Error in unlinking read semaphore: %s\n", strerror(errno));
        }
        readSemaphore = NULL;
    }

    /* check if write semaphore pointer exists */
    if (writeSemaphore != NULL) {
        /* close the write semaphore */
        int semaphore_write_close = sem_close(writeSemaphore);
        if (semaphore_write_close == -1) {
            fprintf(stderr, "Error in closing write semaphore, %s\n", strerror(errno));
        }


        /* unlink the semaphore, if not done, error EEXIST */
        int semaphore_write_unlink = sem_unlink(semaphoreWriteName);
        if (semaphore_write_unlink == -1) {
            fprintf(stderr, "Error in unlinking write semaphore , %s\n", strerror(errno));
        }
        writeSemaphore = NULL;
    }

    if (sharedMemory != NULL) {
        /* unmap the memory */
        /* TODO: unmap fails every time, but the memory is deleted */
        int unmapReturn = munmap(sharedMemory, size);
        if (unmapReturn == ERROR) {
            fprintf(stderr, "Error in unmapping memory, %s\n", strerror(errno));
        }

        /* close the memory file */
        int closeMemory = close(fileDescr);
        if (closeMemory == ERROR) {
            fprintf(stderr, "Error in closing memory file, %s\n", strerror(errno));
        }

        /* unlink the memory file from /dev/shm/ */
        if (shm_unlink(sharedMemoryName) == ERROR) {
            fprintf(stderr, "Error in unlinking memory file, %s\n", strerror(errno));
        }

        sharedMemory = NULL;
    }
}
