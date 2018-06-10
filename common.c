//
// Created by manuel on 20/05/18.
//

#include "sharedMemory.h"

static bool tryCreateSemaphores(char* semaphoreName, size_t size, sem_t** semaphore);

semaphores getSemaphores(size_t size) {
    semaphores sems;
    bool error = false;

    int uid = getuid();  // This function is always successful according to the manpage
    snprintf(sems.readSemaphoreName, NAMELLENGTH, "/sem_%d", 1000 * uid + 0);
    snprintf(sems.writeSemaphoreName, NAMELLENGTH, "/sem_%d", 1000 * uid + 1);

    if (!tryCreateSemaphores(sems.readSemaphoreName, 0, &sems.readSemaphore))
        error = true;

    if (!error) {
        if (!tryCreateSemaphores(sems.writeSemaphoreName, size, &sems.writeSemaphore))
            error = true;
    }

    if (error) {
        sems.readSemaphore = NULL;
        sems.writeSemaphore = NULL;
    }

    return sems;
}

bool tryCreateSemaphores(char* semaphoreName, size_t size, sem_t** semaphore) {
    bool ret = true;

    *semaphore = sem_open(semaphoreName, O_CREAT | O_EXCL, S_IRWXU, size);
    if (*semaphore == SEM_FAILED) {
        if (errno == EEXIST)   //This is ok, someone else created our semaphore for us. How nice!
            *semaphore = sem_open(semaphoreName, O_RDWR, 0);

        if (*semaphore == SEM_FAILED) { //either from the first or from the second sem_open call.
            fprintf(stderr, "Sender: Error creating %s, %s\n", semaphoreName, strerror(errno));
            ret = false;
        }
    }

    return ret;
}

//TODO: rewrite to single error path
sharedmem getSharedMem(size_t size) {
    sharedmem shared;
    int protection = PROT_WRITE;

    shared.size = size;

    int uid = getuid();  // These functions are always successful. man7
    snprintf(shared.sharedMemoryName, NAMELLENGTH, "/shm_%d", 1000 * uid + 0);

    // initialize shared memory
    shared.fileDescriptor = shm_open(shared.sharedMemoryName, O_CREAT | O_EXCL | O_RDWR, S_IRWXU);
    if (shared.fileDescriptor == ERROR) {
        if (errno == EEXIST)
            shared.fileDescriptor = shm_open(shared.sharedMemoryName, O_RDWR, 0);

        if (shared.fileDescriptor == ERROR) {
            fprintf(stderr, "Error in opening shared memory, %s\n", strerror(errno));
            shared.sharedMemory = NULL;
            shared.fileDescriptor = 0;
            return shared;
        }
    } else {
        // fixing the shared memory to a define size (coming from the agrv)
        int returrnVal = ftruncate(shared.fileDescriptor, size * sizeof(int));
        if (returrnVal == ERROR) {
            fprintf(stderr, "Error in truncating shared memory, %s\n", strerror(errno));
            shared.sharedMemory = NULL;
            shared.fileDescriptor = 0;
            return shared;
        }
    }

    shared.sharedMemory = mmap(NULL, size * sizeof(int), protection, MAP_SHARED, shared.fileDescriptor, 0);
    if (shared.sharedMemory == MAP_FAILED) {
        fprintf(stderr, "Error in mapping memory, %s\n", strerror(errno));
        shared.sharedMemory = NULL;
        shared.fileDescriptor = 0;
        return shared;
    }

    return shared;
}

void removeRessources(semaphores* sems, sharedmem* shared) {
    if (sems != NULL) {
        if (sems->readSemaphore != NULL) {
            /* close the read semaphore */
            int semaphore_read_close;
            semaphore_read_close = sem_close(sems->readSemaphore);
            if (semaphore_read_close == -1) {
                fprintf(stderr, "Error in closing read semaphore, %s\n", strerror(errno));
            }


            /* unlink the read semaphore */
            int semaphore_read_unlink = sem_unlink(sems->readSemaphoreName);
            if (semaphore_read_unlink == -1) {
                fprintf(stderr, "Error in unlinking read semaphore: %s\n", strerror(errno));
            }
            sems->readSemaphore = NULL;
        }

        if (sems->writeSemaphore != NULL) {
            /* close the write semaphore */
            int semaphore_write_close = sem_close(sems->writeSemaphore);
            if (semaphore_write_close == -1) {
                fprintf(stderr, "Error in closing write semaphore, %s\n", strerror(errno));
            }


            /* unlink the semaphore, if not done, error EEXIST */
            int semaphore_write_unlink = sem_unlink(sems->writeSemaphoreName);
            if (semaphore_write_unlink == -1) {
                fprintf(stderr, "Error in unlinking write semaphore , %s\n", strerror(errno));
            }
            sems->writeSemaphore = NULL;
        }
    }

    if (shared != NULL) {
        if (shared->sharedMemory != NULL) {
            /* unmap the memory */
            /* TODO: unmap fails every time, but the memory is deleted */
            int unmapReturn = munmap(shared->sharedMemory, shared->size);
            if (unmapReturn == ERROR) {
                fprintf(stderr, "Error in unmapping memory, %s\n", strerror(errno));
            }

            /* close the memory file */
            int closeMemory = close(shared->fileDescriptor);
            if (closeMemory == ERROR) {
                fprintf(stderr, "Error in closing memory file, %s\n", strerror(errno));
            }

            /* unlink the memory file from /dev/shm/ */
            if (shm_unlink(shared->sharedMemoryName) == ERROR) {
                fprintf(stderr, "Error in unlinking memory file, %s\n", strerror(errno));
            }

            shared->sharedMemory = NULL;
        }
    }
}
