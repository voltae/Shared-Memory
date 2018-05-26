//
// Created by manuel on 20/05/18.
//

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
#include <assert.h>


#define NAMELLENGTH 14
#define ERROR (-1)

typedef struct semaphores{
    sem_t* readSemaphore;
    sem_t* writeSemaphore;
} semaphores;

typedef struct sharedmem {
    int fileDescriptor;
    int* sharedMemory;
} sharedmem;

semaphores getSemaphores(size_t size);
sharedmem getSharedMem(size_t size);
void removeRessources (size_t size);
#endif //SHARED_MEMORY_SHAREDMEMORY_H
