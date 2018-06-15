/*!
* @file common.c
* Operation systems shared memory sender/receiver functionality
* assignment 3
*
* @author Valentin Platzgummer <ic17b096@technikum-wien.at>
* @author Lara Kammerer <ic17b001@technikum-wien.at>
* @date 2018/06/15
* @version 1.6
* @brief common modul for allocation and removing the needed ressources
*/


// -------------------------------------------------------------- includes --
#include "sharedMemory.h"

// --------------------------------------------------------------- defines --

// -------------------------------------------------------------- typedefs --

// --------------------------------------------------------------- globals --

// ------------------------------------------------------------- functions --
static bool tryCreateSemaphores(char* semaphoreName, size_t size, sem_t** semaphore);


/*!
 * @brief creates the name for both semaphores and calls the allocating function
 * @param size type unsigned int, size of the requested shared memory
 * @param sems pointer to the semaphores struct
 * @return typ bool, true in case allocation-success, false on failure
 */
bool getSemaphores(size_t size, semaphores* sems) {
    bool ret = true;

    int uid = getuid();  // This function is always successful according to the manpage
    snprintf(sems->readSemaphoreName, NAMELLENGTH, "/sem_%d", 1000 * uid + 0);
    snprintf(sems->writeSemaphoreName, NAMELLENGTH, "/sem_%d", 1000 * uid + 1);

    ret = tryCreateSemaphores(sems->readSemaphoreName, 0, &sems->readSemaphore);

    ret = ret && tryCreateSemaphores(sems->writeSemaphoreName, size, &sems->writeSemaphore);

    return ret;
}

/*!
 * @brief creates the requested semaphores in case they do not exists, and opens them
 * in case they are already created
 * @param semaphoreName char, name of the semaphores either to create or open.
 * @param size type (size_t) unsigned integer, size of the buffer for the write semaphore
 * @param semaphore doublepointer to the semaphore
 * @return type bool, true in case semaphre exists, false if not.
 */
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
/*!
 * @brief function allocates the requested shared memory in case the shared memory does not exist
 * it opens the memory from file if there is already a memory allocated by an other process
 * @param size type (size_t) unsigned integer, size of the requested buffer
 * @param shared pointer to the sharedmemory struct
 * @return type bool, true if requested memory exists, false if not.
 */
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
/*!
 * @brief removes all allocated ressources, read semaphore, write semaphore
 * and the shared memory
 * @param sems pointer to the shared semaphore struct
 * @param shared pointer to the shared shared memory struct
 */
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
