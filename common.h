//
// Created by marcaurel on 12.05.18.
//

#ifndef SHARED_MEMORY_COMMON_H
#define SHARED_MEMORY_COMMON_H

#include <stdio.h>
#include <stdlib.h>  // for EXIT_SUCCESS etc

#include <semaphore.h>  // for semaphores
#include <fcntl.h>      // For O_* constants
#include <sys/stat.h>   // For mode constants
#include <sys/mman.h>   // for shared memory
#include <unistd.h>     // for ftruncate() truncation shared memory

#include <errno.h>      // for errno global constant
#include <string.h>     // for strerror


#define NAMELLENGTH 14
typedef enum isError {ERROR = -1, SUCCESS} isError;


extern char semaphoreReadName[NAMELLENGTH];  // Semaphore read Name with own annuminas uid
extern char semaphoreWriteName[NAMELLENGTH];  // Semaphore write Name with own annuminas uid
extern char sharedMemoryName[NAMELLENGTH];     // file name of the storing memory


extern sem_t *createSemaphore (size_t buffersize, char type);
extern char *createSharedMemory (size_t buffersize);
extern isError closeSharedMemory (char *sharedMemory, size_t buffersize);
extern isError closeSemaphores (sem_t *semaphore, char type);
void BailOut(const char *message);


#endif //SHARED_MEMORY_COMMON_RESOURCES_H
