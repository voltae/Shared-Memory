//
// @file sender.c
// Betriebssysteme senderReceiver.
// Beispiel 3
//
// @author Dominic Mages <ic17b014@technikum-wien.at>
// @author Ralph Hödl <ic17b003@technikum-wien.at>
// @author David North <ic17b086@technikum-wien.at>
// @date 2018/06/14
//
// @version 001
//
//

#include "datah.h"

/**
 *
 * \brief reads from stdin into shared memory
 *
 * @param argc the number of arguments
 * @param argv the arguments
 *
 * @returns EXIT_SUCCESS, EXIT_FAILURE
 *
 */
int main(int argc, char *argv[])
{
    long shmSize;

    if ((shmSize = sharedSize(argc, argv)) == -1)
    {
        fprintf(stderr, "Usage: %s -m <buffer_size>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (sharedSend(shmSize, stdin) == -1)
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}