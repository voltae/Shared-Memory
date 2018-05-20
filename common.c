//
// Created by manuel on 20/05/18.
//

#include "sharedMemory.h"

static sem_t* readSemaphore;
static sem_t* writeSemaphore;

//TODO: rewrite into one errorhandling path
semaphores getSemaphores(size_t size) {
    semaphores sems;
    if (readSemaphore != NULL)
        sems.readSemaphore = readSemaphore;
    else {
        readSemaphore = sem_open(semaphoreReadName, O_CREAT | O_EXCL, S_IRWXU, 0);
        if (readSemaphore == SEM_FAILED) {
            /* Error message */
            fprintf(stderr, "Sender: Error in creating read-semaphore, %s\n", strerror(errno));
            sems.readSemaphore = NULL;
            sems.writeSemaphore = NULL;
            return NULL;
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
            return NULL;
        }
    }

    sems.readSemaphore = readSemaphore;
    sems.writeSemaphore = writeSemaphore;
    return sems;
}