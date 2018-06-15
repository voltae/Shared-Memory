/*!
* @file receiver.c
* Operation systems shared memory sender/receiver functionality
* assignment 3
*
* @author Valentin Platzgummer <ic17b096@technikum-wien.at>
* @author Lara Kammerer <ic17b001@technikum-wien.at>
* @date 2018/06/15
* @version 1.6
* @brief The receiver() process reads characters from a buffer, implemented as ringbuffer with user defined size
* and print them to standard out.
* @param type integer of the requested size of memory. It will fail, if the user exceeds the shared memory limit
* of the given machine, error will be printed.
* @param command -m informs the receiver that the following number is the size of the requested ringbuffer
* @retval integer
* @return 0 in case the operation worked as expected, 1 with an usage message in all other cases.
*/

// -------------------------------------------------------------- includes --
#include "sharedMemory.h"

// --------------------------------------------------------------- defines --

// -------------------------------------------------------------- typedefs --

// --------------------------------------------------------------- globals --

// ------------------------------------------------------------- functions --
static void BailOut(const char* progname, semaphores* sems, sharedmem* shared);
static bool checkCommand(int argc, char** argv, size_t *buffersize);

/*!
 * @brief Report Error and free resources
 * Since we are in the receiver process, we are responsable for removing all resources, even in the error case
 * @param progname application name ie argv[0]
 * @param sems local pointer to the allocated semaphores
 * @param shared local pointer to the allocated shared memory
 */
static void BailOut(const char* progname, semaphores* sems, sharedmem* shared) {
    {
        removeRessources(sems, shared);
        fprintf(stderr, "USAGE: %s [-m] length\n", progname);
        exit(EXIT_FAILURE);
    }
}

/*!
 * @brief function checks the incoming commands from command line call against their validity.
 * @param argc number of incoming parameters from command line call
 * @param argv feeded in parameters from command line call
 * @param buffersize pointer of typ size_t (unsigned integer) stores the size of the requested ringbuffer
 * @retval boolean
 * @return true in case check is ok
 * @return false if check failed
 */
static bool checkCommand(int argc, char** argv, size_t *buffersize) {
    int opt;    // option for getop
    int bOptionM = 0;   // Flag for the 'm' option
    int bError = 0;     // Flag for Option Error
    long bufferTemp = 0;
    char* stringInt = NULL;

    if (argc < 2) {
        return false;
    }
    // check operands with getopt(3)
    while ((opt = getopt(argc, argv, "m:")) != -1) {
        switch (opt) {
            case 'm':
                if (bOptionM) {
                    bError = 1;
                    break;
                }
                bOptionM = 1;
                bufferTemp = strtol(optarg, &stringInt, 10);
                break;
            case '?':
                bError = 1;
                break;
            default:
                assert (0);   /* should never be reached */
                break;
        }
    }

    if (optopt == 'm')  {  /* parameter 'm' with no argument */
        return false;
    }
    if (bError) {
        return false;
    }
    if (argc != optind) {
        return false;
    }
    if ((errno == ERANGE) || (strcmp(stringInt, "\0") != 0) || (bufferTemp < 1)) {
        return false;
    }
    *buffersize = (size_t)bufferTemp;
    return true;
}
/*!
 * @brief the reader waits until the writer sends characters to the shared memory. The reader then reads as long
 * as there are some chars to read.
 * @param argc feeded in number of arguments from command line calls
 * @param argv feeded in arguments from command line calls
 * @retval integer
 * @return 0 in case function worked as expected
 */
int main(int argc, char** argv) {
    semaphores sems;
    sharedmem mem;
    bool isNotEOF = true;
    bool getRessources = true;

    /* buffer size allocated, stackvariable */
    size_t buffersize = 0;

    /* Number of read processes, stack variable */
    size_t readingIndex = 0;
    // initialize the reading int for the shared memory
    int readingInt = 0;

    /* check if no parameters are given, in this case print error message and leave program */
    getRessources = getRessources && checkCommand(argc, argv, &buffersize);

    /* trying to open the semaphore */
    getRessources = getRessources && getSemaphores(buffersize, &sems);

    /* trying the shared memory */
    getRessources = getRessources && getSharedMem(buffersize, &mem);

    /* if either semaphores or memory went wrong, output the error. */
    if (getRessources == false)
        BailOut(argv[0], &sems, &mem);


    while (isNotEOF) {
        // write index is the same as the read index. writer must wait
        int semaphoreWait = sem_wait(sems.readSemaphore);
        if (semaphoreWait == ERROR) {
            BailOut(argv[0], &sems, &mem);
        }
        /* read the next character from buffer for the next iteration */
        readingInt = mem.sharedMemory[readingIndex];
        // print out the char to stdout
        if(readingInt != EOF) {
            fputc(readingInt, stdout);
            isNotEOF = true; /* is not necessary, defensive style */
        } else {
            isNotEOF = false;
        }
        /* increment the buffer counter */
        readingIndex = (readingIndex + 1) % buffersize;

        int retval = sem_post(sems.writeSemaphore);
        // increment the counter
        if (retval == ERROR) {
            BailOut(argv[0], &sems, &mem);
        }
    }

    /* remove all resources after the job is done */
    removeRessources(&sems, &mem);

    return EXIT_SUCCESS;
}
// =================================================================== eof ==

// Local Variables:
// mode: c
// c-mode: k&r
// c-basic-offset: 8
// indent-tabs-mode: t
// End: