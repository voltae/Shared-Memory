//
// @file datah.c
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

// global for child process to access it in both functions //wo gehört das hin?------------------------------------------------------------------------------------------------------
// global for the pointer to the pipe //und das?------------------------------------------------------------------------------------------------------------------------------------
// -------------------------------------------------------------- includes --
#include "datah.h"

#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <err.h>

// --------------------------------------------------------------- defines --
#define LEN 24
#define NAME(offset) (1000ull * getuid() + offset)
#define PERMS 0600

// -------------------------------------------------------------- typedefs --
typedef struct
{
  char name[LEN];
  char wName[LEN];
  char rNAme[LEN];
  sem_t *wId;
  sem_t *rId;
  long size;
  int fd;
  int *buffer;
} shared_t;

// --------------------------------------------------------------- globals --
// global pointer for programName
static const char *programName = NULL;

// ------------------------------------------------------------- functions --
static int sharedInit(long size, shared_t *sharedData);
static void sharedCleanup(shared_t *sharedData);
static void sharedClose(shared_t *sharedData);
static void sharedRemove(shared_t *sharedData);

/**
 *
 * \brief parses shm-size from arguments
 *
 * @param argc the number of arguments
 * @param argv the arguments
 *
 * @returns the shared memory size or -1 in case of error
 *
 */
long sharedSize(int argc, char *argv[])
{
	//check in der fkt zum übernehmen der size..find ich nett..übersichtlich-----------------------------------------------------------------------------------------------------
  int opt;
  long size = -1;
  char *notconv = "";
  programName = argv[0];

  if (argc < 2)
  {
    fprintf(stderr, "Usage: %s: -m <buffer_size>\nArguments missing!\n", programName);
    return -1;
  }

  while ((opt = getopt(argc, argv, "m:")) != -1) //zuweisung und Bedingung in einem----------------------------------------------------------------------------------------------
  {
    if (opt == 'm')
    {
      errno = 0;
      size = strtol(optarg, &notconv, 10); //wofür das notconv? reicht nicht NULL?-------------------------------------------------------------------------------------------------
      if (errno != 0 || *notconv != '\0' || size < 1) //hmm also notconv wird von strtol mögleicherweise bearbeitet?---------------------------------------------------------------
										//sie checken nicht welcher error es ist..ist aber womöglich auch egal?--------------------------------------------------------------------
      {
        fprintf(stderr, "Usage: %s: -m <buffer_size>\nInvalid argument!\n", programName);
        return -1;
      }
    }
    else
    {
      return -1;
    }
  }

  if (optind < argc)
  {
    fprintf(stderr, "Usage: %s: -m <buffer_size>\nInvalid argument!\n", programName);
    return -1;
  }

  return size;
}

/**
 *
 * \brief writes from stream to ring-buffer
 *
 * @param size the size of the shared memory
 * @param stream the stream to take as input
 *
 * @returns 0, -1 in case of error
 *
 */

// gut ab hier musst du dir das mal genau anschaun..keine ahnung was die da genau machen bzw dauerts jtz zu lang mich da reinzulesen was sie wieso machen+++++++++++++++++++++++++++++++++++++
int sharedSend(long size, FILE *stream)
{
  shared_t sharedData;
  int input = EOF;
  int position = 0;

  if (sharedInit(size, &sharedData) == -1)
  {
    sharedCleanup(&sharedData);
    return -1;
  }

  do
  {
    if (sem_wait(sharedData.wId) == -1)
    {
      if (errno == EINTR)
      {
        continue;
      }
      else
      {
        fprintf(stderr, "%s: Error with sem_wait\n", programName);
        sharedCleanup(&sharedData);
        return -1;
      }
    }

    // stop if sharedCleanup is triggered
    if (sharedData.buffer[sharedData.size] == EOF)
    {
      fprintf(stderr, "%s: Empfaenger not runninge\n", programName);
      sharedClose(&sharedData);
      return -1;
    }

    input = fgetc(stream);

    if (input == EOF && ferror(stream) != 0)
    {
      fprintf(stderr, "%s: fgetc\n", programName);
      sharedCleanup(&sharedData);
      return -1;
    }

    sharedData.buffer[position] = input;
    position++;

    // stay within buffer
    if (position == size)
    {
      position = 0;
    }

    if (sem_post(sharedData.rId) == -1)
    {
      fprintf(stderr, "%s: sem_post\n", programName);
      sharedCleanup(&sharedData);
      return -1;
    }
  } while (input != EOF);

  sharedClose(&sharedData);

  return 0;
}

/**
 *
 * \brief prints data from ring-buffer
 *
 * @param size the size of the shared memory
 * @param stream the stream to take as input
 *
 * @returns 0, -1 in case of error
 *
 */
int sharedReceive(long size, FILE *stream)
{
  shared_t sharedData;
  int output = EOF;
  int position = 0;

  if (sharedInit(size, &sharedData) == -1)
  {
    sharedCleanup(&sharedData);
    return -1;
  }

  do
  {
    if (sem_wait(sharedData.rId) == -1)
    {
      // try again
      if (errno == EINTR)
      {
        continue;
      }
      else
      {
        fprintf(stderr, "%s: sem_wait\n", programName);
        sharedCleanup(&sharedData);
        return -1;
      }
    }

    // stop if sharedCleanup is triggered
    if (sharedData.buffer[sharedData.size] == EOF)
    {
      fprintf(stderr, "%s: Sender not running\n", programName);
      sharedClose(&sharedData);
      return -1;
    }

    output = sharedData.buffer[position];
    position++;

    if (output != EOF)
    {
      if (fputc(output, stream) == EOF)
      {
        fprintf(stderr, "%s: fputc\n", programName);
        sharedCleanup(&sharedData);
        return -1;
      }
    }

    // stay within buffer
    if (position == size)
    {
      position = 0;
    }

    if (sem_post(sharedData.wId) == -1)
    {
      fprintf(stderr, "%s: sem_post\n", programName);
      sharedCleanup(&sharedData);
      return -1;
    }
  } while (output != EOF);

  if (fflush(stream) == EOF)
  {
    fprintf(stderr, "%s: fflush\n", programName);
    return -1;
  }

  sharedCleanup(&sharedData);

  return 0;
}

/**
 *
 * \brief creates/opens semaphores and shared memory
 *
 * @param size the size of the shared memory
 * @param data struct to manage data
 *
 * @returns 0, -1 in case of error
 *
 */
static int sharedInit(long size, shared_t *sharedData)
{
  sharedData->size = size;
  sharedData->fd = -1;
  sharedData->buffer = MAP_FAILED;
  sharedData->wId = SEM_FAILED;
  sharedData->rId = SEM_FAILED;

  if (sprintf(sharedData->name, "%c%llu", '/', NAME(0)) < 0)
  {
    fprintf(stderr, "%s: sprintf\n", programName);
    return -1;
  }
  if (sprintf(sharedData->wName, "%c%llu", '/', NAME(1)) < 0)
  {
    fprintf(stderr, "%s: sprintf\n", programName);
    return -1;
  }
  if (sprintf(sharedData->rNAme, "%c%llu", '/', NAME(2)) < 0)
  {
    fprintf(stderr, "%s: sprintf\n", programName);
    return -1;
  }

  if ((sharedData->fd = shm_open(sharedData->name, O_RDWR | O_CREAT, PERMS)) == -1)
  {
    fprintf(stderr, "%s: shm_open\n", programName);
    return -1;
  }

  if (ftruncate(sharedData->fd, size * sizeof(int)) == -1)
  {
    fprintf(stderr, "Usage: %s: -m <buffer_size>\nMaximum possible size for shared memory is exceeded!\n", programName);
    return -1;
  }

  if ((sharedData->buffer = mmap(NULL, (size + 1) * sizeof(*sharedData->buffer), PROT_READ | PROT_WRITE, MAP_SHARED, sharedData->fd, 0)) == MAP_FAILED)
  {
    fprintf(stderr, "%s: mmap\n", programName);
    return -1;
  }

  if ((sharedData->wId = sem_open(sharedData->wName, O_CREAT, PERMS, size)) == SEM_FAILED)
  {
    fprintf(stderr, "%s: sem_open\n", programName);
    return -1;
  }

  if ((sharedData->rId = sem_open(sharedData->rNAme, O_CREAT, PERMS, 0)) == SEM_FAILED)
  {
    fprintf(stderr, "%s: sem_open\n", programName);
    return -1;
  }

  return 0;
}

/**
 * @brief performs a full cleanup
 *
 * @param sharedData struct to manage data
 */
static void sharedCleanup(shared_t *sharedData)
{

  if (sharedData->buffer != MAP_FAILED)
  {
    sharedData->buffer[sharedData->size] = EOF;
  }

  if (sharedData->wId != SEM_FAILED)
  {
    if (sem_post(sharedData->wId) == -1)
    {
      fprintf(stderr, "%s: sem_post\n", programName);
    }
  }
  if (sharedData->rId != SEM_FAILED)
  {
    if (sem_post(sharedData->rId) == -1)
    {
      fprintf(stderr, "%s: sem_post\n", programName);
    }
  }

  sharedClose(sharedData);
  sharedRemove(sharedData);
}

/**
 * @brief closes the semaphores and shared memory
 *
 * @param sharedData struct to manage data
 */
static void sharedClose(shared_t *sharedData)
{
  if (sharedData->wId != SEM_FAILED)
  {
    if (sem_close(sharedData->wId) == -1)
    {
      fprintf(stderr, "%s: sem_close\n", programName);
    }
  }
  if (sharedData->rId != SEM_FAILED)
  {
    if (sem_close(sharedData->rId) == -1)
    {
      fprintf(stderr, "%s: sem_close\n", programName);
    }
  }
  if (sharedData->buffer != MAP_FAILED)
  {
    if (munmap(sharedData->buffer, (size_t)sharedData->size) == -1)
    {
      fprintf(stderr, "%s: munmap\n", programName);
    }
  }
}

/**
 * @brief removes the semaphores and shared memory
 *
 * @param sharedData struct to manage data
 */
static void sharedRemove(shared_t *sharedData)
{
  if (sharedData->wId != SEM_FAILED)
  {
    if (sem_unlink(sharedData->wName) == -1)
    {
      fprintf(stderr, "%s: sem_unlink\n", programName);
    }
  }
  if (sharedData->rId != SEM_FAILED)
  {
    if (sem_unlink(sharedData->rNAme) == -1)
    {
      fprintf(stderr, "%s: sem_unlink\n", programName);
    }
  }
  if (sharedData->fd != -1)
  {
    if (shm_unlink(sharedData->name) == -1)
    {
      fprintf(stderr, "%s: sem_unlink\n", programName);
    }
  }
}