/*!
* @file sharedMemory.h
* Operation systems shared memory sender/receiver functionality
* assignment 3
*
* @author Valentin Platzgummer <ic17b096@technikum-wien.at>
* @author Lara Kammerer <ic17b001@technikum-wien.at>
* @date 2018/06/15
* @version 1.6
* @brief Header declaration for all common used ressources
*/
#ifndef SHARED_MEMORY_SHAREDMEMORY_H
#define SHARED_MEMORY_SHAREDMEMORY_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>  // for EXIT_SUCCESS etc
#include <semaphore.h>  // for semaphores
#include <fcntl.h>      // For O_* constants
#include <sys/stat.h>   // For mode constants
#include <sys/mman.h>   // for shared memory
#include <unistd.h>     // for ftruncate() truncation shared memory
#include <errno.h>      // for errno global constant
#include <string.h>     // for strerror
#include <assert.h>     // assert
#include <stdbool.h>    // for use of bool
#include <values.h>     // for LONG_MAX


#define NAMELLENGTH 14
#define ERROR (-1)

/*!
 * @struct semaphores stores the references of the semaphores
 */
typedef struct semaphores{
    sem_t* readSemaphore;           /*! pointer to the read semaphore */
    sem_t* writeSemaphore;          /*! pointer to the write semaphore */
    char readSemaphoreName[NAMELLENGTH];    /*! char stores the name of the read semaphore */
    char writeSemaphoreName[NAMELLENGTH];   /*! char stores the name of the write semaphore */
} semaphores;
/*!
 * @struct sharedmem stores the reference to the shared memory
 */
typedef struct sharedmem {
    int fileDescriptor;     /*! file descriptor of the shared memory */
    int* sharedMemory;      /*! int pointer to the actual shared memory */
    char sharedMemoryName[NAMELLENGTH];     /*! char stores the name of the shared memory */
    size_t size;            /*! unsigned int stores the size of the shared memory */
} sharedmem;

/*!
 * @brief creates the name for both semaphores and calls the allocating function
 * @param size type unsigned int, size of the requested shared memory
 * @param sems pointer to the semaphores struct
 * @return typ bool, true in case allocation-success, false on failure
 */
bool getSemaphores(size_t size, semaphores* sems);
/*!
 * @brief function allocates the requested shared memory in case the shared memory does not exist
 * it opens the memory from file if there is already a memory allocated by an other process
 * @param size type (size_t) unsigned integer, size of the requested buffer
 * @param shared pointer to the sharedmemory struct
 * @return type bool, true if requested memory exists, false if not.
 */
bool getSharedMem(size_t size, sharedmem* shared);
/*!
 * @brief removes all allocated ressources, read semaphore, write semaphore
 * and the shared memory
 * @param sems pointer to the shared semaphore struct
 * @param shared pointer to the shared shared memory struct
 */
void removeRessources(semaphores* sems, sharedmem* shared);
#endif //SHARED_MEMORY_SHAREDMEMORY_H
