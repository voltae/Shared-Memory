//
// Created by manuel on 20/05/18.
//

#include "sharedMemory.h"

static bool tryCreateSemaphores(char* semaphoreName, size_t size, sem_t** semaphore);

bool getSemaphores(size_t size, semaphores* sems) {
    bool ret = true;

    int uid = getuid();  // This function is always successful according to the manpage
    snprintf(sems->readSemaphoreName, NAMELLENGTH, "/sem_%d", 1000 * uid + 0);
    snprintf(sems->writeSemaphoreName, NAMELLENGTH, "/sem_%d", 1000 * uid + 1);

    ret = tryCreateSemaphores(sems->readSemaphoreName, 0, &sems->readSemaphore);

    ret = ret && tryCreateSemaphores(sems->writeSemaphoreName, size, &sems->writeSemaphore);

    return ret;
}

bool tryCreateSemaphores(char* semaphoreName, size_t size, sem_t** semaphore) {
    bool ret = true;

    *semaphore = sem_open(semaphoreName, O_CREAT | O_EXCL, S_IRWXU, size);
    if (*semaphore == SEM_FAILED) {
        if (errno == EEXIST)   //This is ok, someone else created our semaphore for us. How nice!
            *semaphore = sem_open(semaphoreName, O_RDWR, 0);

        if (*semaphore == SEM_FAILED) { //either from the first or from the second sem_open call.
            ret = false;
        }
    }

    return ret;
}

bool getSharedMem(size_t size, sharedmem* shared) {
    bool ret = true;
    int protection = PROT_WRITE;

    shared->size = size;

    int uid = getuid();  // This function is always successful according to the manpage
    snprintf(shared->sharedMemoryName, NAMELLENGTH, "/shm_%d", 1000 * uid + 0);

    shared->fileDescriptor = shm_open(shared->sharedMemoryName, O_CREAT | O_EXCL | O_RDWR, S_IRWXU);
    if (shared->fileDescriptor == ERROR) {
        if (errno == EEXIST)    //This is fine, someone else created our shared memory for us. How nice!
            shared->fileDescriptor = shm_open(shared->sharedMemoryName, O_RDWR, 0);

        if (shared->fileDescriptor == ERROR) { //either from the first or from the second shm_open call.
            ret = false;
        }
    } else {
        if (ftruncate(shared->fileDescriptor, size * sizeof(int)) == ERROR)
            ret = false;
    }

    //if we managed to create a file the shm needs to be mapped even if we ran into problems truncating it or we won't have a pointer to free later.
    if (shared->fileDescriptor != ERROR) {
        shared->sharedMemory = mmap(NULL, size * sizeof(int), protection, MAP_SHARED, shared->fileDescriptor, 0);
        if (shared->sharedMemory == MAP_FAILED)
            ret = false;
    }

    return ret;
}

void removeRessources(semaphores* sems, sharedmem* shared) {
    if (sems != NULL) {
        if (sems->readSemaphore != NULL) {
            /* close the read semaphore */
            if (sem_close(sems->readSemaphore) == 0) {
                //if either of those fails... well that's really too bad.
                sem_unlink(sems->readSemaphoreName);
                sems->readSemaphore = NULL;
            }
        }

        if (sems->writeSemaphore != NULL) {
            /* close the write semaphore */
            if (sem_close(sems->writeSemaphore) == 0) {
                //if either of those fails... well that's really too bad.
                sem_unlink(sems->writeSemaphoreName);
                sems->writeSemaphore = NULL;
            }
        }
    }

    if (shared != NULL) {
        if (shared->fileDescriptor != 0) {
            do {
                if (!close(shared->fileDescriptor))
                    shared->fileDescriptor = 0;
                /* if the disk has trouble closing our file I assume nobody else will manage to read it either and if we
                 * managed to create a bogus filedescriptor that would be quite impressive! */
            } while (shared->fileDescriptor != 0 && errno == EINTR);
        }


        if (shared->sharedMemory != NULL) {
            //if either of those fails. Well. I'd be really sad.
            munmap(shared->sharedMemory, shared->size);
            shm_unlink(shared->sharedMemoryName);
            shared->sharedMemory = NULL;
        }
    }
}
