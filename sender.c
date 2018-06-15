
/**
 * @file sender.c
 *
 * Betriebssysteme - Beispiel 3
 * sender - records input form stdin an inputs it into a ringbuffer in shared memory
 *
 * @author Manuel Seifner	 <ic17b022@technikum-wien.at>
 * @author Oliver Safar		 <ic17b077@technikum-wien.at>
 * @date 2018/06/15
 *
 * @version 1.0
*/

// -------------------------------------------------------------- includes --

#include "sharedMemory.h"

// --------------------------------------------------------------- defines --

// -------------------------------------------------------------- typedefs --

// --------------------------------------------------------------- globals --

// ------------------------------------------------------------- functions --

static bool readParameters(const int argc, char* const argv[], size_t* bufferSize);
static bool transcribe(semaphores* sems, sharedmem* mem, const size_t buffersize);

/**
 * \brief The sender() process reads input and saves it into a ring buffer in shared memory
 *
 * \param   argc       the number of arguments
 * \param   argv[]     the arguments itselves (including the program name in argv[0])
 *
 * \return  EXIT_SUCCESS when program finishes without error
 * \return  EXIT_FAILURE if an error occurs
 */
int main(int argc, char* argv[]) {
    bool rc = true;
    semaphores sems = {.writeSemaphore = NULL, .readSemaphore = NULL};
    sharedmem mem = {.sharedMemory = NULL, .fileDescriptor = 0};
    size_t buffersize;

    /* Parameters */
    rc = readParameters(argc, argv, &buffersize);

    /* Semaphores */
    rc = rc && getSemaphores(buffersize, &sems);

    /* Sharedmemory */
    rc = rc && getSharedMem(buffersize, &mem);

    /* read from stdin and write to shared memory */
    rc = rc && transcribe(&sems, &mem, buffersize);

    if (!rc) {    //We only clean up if things went wrong. Otherwise its the receivers job.
        removeRessources(&sems, &mem);
        fprintf(stderr, "USAGE: %s [-m] length\n", argv[0]);
        return EXIT_FAILURE;
    } else
        return EXIT_SUCCESS;
}

/**
 * \brief This function checks the input arguments.
 *
 * \param   argc       the number of arguments
 * \param   argv[]     the arguments itselves (including the program name in argv[0])
 * \param   bufferSize pointer of typ size_t storing the size of the requested ringbuffer
 *
 * \retval  rc boolean starting out true, is set to false when a check fails
 * \return  true if parameters are good
 * \return  false if wrong parameters were entered
 */
static bool readParameters(const int argc, char* const argv[], size_t* bufferSize) {
    bool rc = true;
    int option;
    bool mFound = false;
    char* stringInt = NULL;


    *bufferSize = 0;
    /* check if no paramters are given */
    if (argc < 2)
        rc = false;
    else {
        while (rc && (option = getopt(argc, argv, "m:")) != -1) {
            switch (option) {
                case 'm': {
                    if (!mFound) {
                        mFound = true;
                        *bufferSize = strtol(optarg, &stringInt, 10);
                        //non numeric leftovers in the option string is a no-no and giving us a buffer size of 0 or less is just mean
                        //We don't check for ERANGE and LONG_MIN because we already test for size < 0. LONG_MIN should satisfy that. Rough estimate.
                        if ((strcmp(stringInt, "\0") != 0) || (*bufferSize <= 0) ||
                            (*bufferSize == LONG_MAX && errno == ERANGE))
                            rc = false;
                    } else //we do not allow a second "-m"
                        rc = false;

                    break;
                }
                case '?':   //other options than m are a no-no
                    rc = false;
                    break;
                default:
                    assert (0);   /* should never be reached */
                    break;
            }
        }
    }

    //don't want any other junk in my arguments.
    if (argc != optind) {
        rc = false;
    }

    return rc;
}

/**
 * \brief This function is responsible for writing to the ring buffer it keeps writing until the buffer is full or EOF is encountered.
 * \brief If the buffer is full it waits until the reciever starts reading on the other end.
 *
 * \param   *sems       pointer to the semaphoes to keep track of how much space is used in the ring buffer
 * \param   *mem        pointer to the shared memory that contains the ring buffer
 * \param   bufferSize  size of the requested ringbuffer
 *
 * \retval  rc boolean starting out true, is set to false if an error occurs during the transcribe process
 * \return  true if EOF was reached without an error
 * \return  false if an error occured
 */
bool transcribe(semaphores* sems, sharedmem* mem, const size_t buffersize) {
    bool rc = true;
    int semWaitRC = 0;
    int readingInt;
    size_t sharedMemoryIndex = 0;


    do {
        readingInt = fgetc(stdin);

        do {
            semWaitRC = sem_wait(sems->writeSemaphore);
        } while (semWaitRC == ERROR && errno == EINTR);

        //last sem_wait call failed and errno is not EINTR
        if (semWaitRC == ERROR)
            rc = false;

        mem->sharedMemory[sharedMemoryIndex] = readingInt;

        if (sem_post(sems->readSemaphore) == ERROR)
            rc = false;

        sharedMemoryIndex = (sharedMemoryIndex + 1) % buffersize;
    } while (rc && readingInt != EOF);

    return rc;
}
// =================================================================== eof ==

// Local Variables:
// mode: c
// c-mode: k&r
// c-basic-offset: 8
// indent-tabs-mode: t
// End:
