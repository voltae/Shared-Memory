//
// Created by marcaurel on 11.05.18.
//

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

typedef enum isError { ERROR = -1, SUCCESS } isError;

/* Global constant semaphore read */
static char semaphoreReadName[NAMELLENGTH];  // Semaphore read Name with own annuminas uid
static char semaphoreWriteName[NAMELLENGTH];  // Semaphore write Name with own annuminas uid
static char sharedMemoryName[NAMELLENGTH];     // Shared memory name

static void setRessourcesName (void);
static void createSemaphores (size_t size);
static void createSharedMemory (size_t size);

//static void removeRessources (void);
static void BailOut (const char *message);

static sem_t *readSemaphore = NULL;
static sem_t *writeSemaphore = NULL;
static char *sharedMemory = NULL;

/* Number of write processes */
static unsigned int w;
/* file descriptor for the shared memory */
static int fileDescr_sm;

static char *szCommand = "<not yet set>";

/* Global constant for shared memory */
// file descriptor for the shared memory (is stored on disk "/dev/shm")
// the linked memory address in current address space

/* Report Error and free ressources */
void print_usage (void)
{
  fprintf (stderr, "USAGE: %s [-m] length\n", szCommand);
  exit (EXIT_FAILURE);
}

/* Report Error and allocate ressources
 * Since we are in the sender process, we are responsable for allocating all ressources,
 */
void BailOut (const char *message)
{
  if (message != NULL)
    {
      fprintf(stderr, "%s: %s\n",szCommand, message);
    }

}
int main (int argc, char **argv)
{
  /* store the progam name */
  szCommand = argv[0];

  size_t buffersize = 0; // buffersize of memory
  int opt;    // option for getop
  long bufferTemp;
  char *stringInt = NULL;
  /* check if arguments are more 1 */
  if (argc < 2)
    {
      print_usage ();
    }
  // check operants with getopt(3)
  while ((opt = getopt (argc, argv, "m:")) != -1)
    {
      switch (opt)
        {
          case 1:
            print_usage ();
          case 'm':
            {
              bufferTemp = strtol (optarg, &stringInt, 10);
              if ((bufferTemp == ERROR) || stringInt != NULL)
                {
                  /* error handing */
                  print_usage ();
                }
              buffersize = (size_t) bufferTemp;
              break;
            }
          default:
            print_usage ();
          break;
        }
    }

  /* Set the ressource names */
  setRessourcesName ();
  /* open the semaphores */
  createSemaphores (buffersize);
  /* Create shared Memory */
  createSharedMemory (buffersize);
  // initialize the reading char for the shared memory
  char readingChar;

  while ((readingChar = fgetc (stdin)) != EOF)
    {
      // write index is the same as the read index. writer must wait
      int semaphoreWait = sem_wait (writeSemaphore);
      if (semaphoreWait == ERROR)
        {
          fprintf (stderr, "Error in waiting semaphore, %s\n", strerror (errno));
          BailOut ("Could not wait for Semaphore");
        }
      /* read a char from shared memory */
      sharedMemory[w] = readingChar;

      int retval = sem_post (readSemaphore);
      if (retval == ERROR)
        {
          fprintf (stderr, "Error in posting semaphore, %s\n", strerror (errno));
          BailOut ("Could not post for Semaphore");
        }
      // increment the counter
      w = (w + 1) % buffersize;
    }

  /* Signalize the receiver that reading is not yet over EOF! has to be read */
  sharedMemory[w] = EOF;
  sem_post (readSemaphore);

  return EXIT_SUCCESS;
}

static void setRessourcesName (void)
{

  int uid = getuid ();  // These functions are always successful. man7

  /* create read semaphore */
  snprintf (semaphoreReadName, 13, "/sem_%d", 1000 * uid + 0);
  snprintf (semaphoreWriteName, 13, "/sem_%d", 1000 * uid + 1);
  snprintf (sharedMemoryName, 13, "/shm_%d", 1000 * uid + 0);
}

static void createSemaphores (size_t size)
{
  if (readSemaphore != NULL)
    {
      return;
    }

  /* Open a new read semaphore */
  readSemaphore = sem_open (semaphoreReadName, O_CREAT | O_EXCL, S_IRWXU, 0);
  if (readSemaphore == SEM_FAILED)
    {
      /* Error message */
      fprintf (stderr, "Error in creating semaphore, %s\n", strerror (errno));
      BailOut ("Could not create Semaphore");
    }

  if (writeSemaphore != NULL)
    {
      return;
    }

  /* create a new write semaphore */
  writeSemaphore = sem_open (semaphoreWriteName, O_CREAT | O_EXCL, S_IRWXU, size);
  if (writeSemaphore == SEM_FAILED)
    {
      /* Error message */
      fprintf (stderr, "Error in creating semaphore, %s\n", strerror (errno));
      BailOut ("Could not create Semaphore");
    }
}

/* Create a new shared memory, it should exist */
static void createSharedMemory (size_t size)
{
  if (sharedMemory != NULL)
    {
      return;
    }

  if (strcmp (sharedMemoryName, "\0") == 0)
    {
      setRessourcesName ();
    }

  // initialize shared memory
  fileDescr_sm = shm_open (sharedMemoryName, O_CREAT | O_EXCL | O_RDWR, S_IRWXU);
  if (fileDescr_sm == ERROR)
    {
      fprintf (stderr, "Error in opening shared memory, %s\n", strerror (errno));
      BailOut ("Could not create shared memory");
    }

  // fixing the shared memory to a define size (coming from the agrv)
  int returrnVal = ftruncate (fileDescr_sm, size * sizeof (char));
  if (returrnVal == ERROR)
    {
      fprintf (stderr, "Error in truncating shared memory, %s\n", strerror (errno));
      BailOut ("Could not truncate shared memory");
    }

  sharedMemory = mmap(NULL, size * sizeof (char), PROT_WRITE, MAP_SHARED, fileDescr_sm, 0);
  if (sharedMemory == MAP_FAILED)
    {
      fprintf (stderr, "Error in mapping memory, %s\n", strerror (errno));
      BailOut ("Could not map the memory");
    }
}

/*
static void removeRessources (void)
{
  int returnValue = sem_unlink (semaphoreReadName) == -1;

  if (returnValue == -1)
    {
      fprintf (stderr, "Error in unlinking read semaphore, %s\n", strerror (errno));
      BailOut ("Could not unlink read Semaphore\n");
      exit (EXIT_FAILURE);
    }

  int semaphore_close = sem_close (writeSemaphore);
  if (semaphore_close == -1)
    {
      fprintf (stderr, "Error in closing write semaphore, %s\n", strerror (errno));
      BailOut ("Could not close write Semaphore\n");
      exit (EXIT_FAILURE);
    }
  semaphore_close = 0;
  */
/* unlink the semaphore, if not done, error EEXIST *//*

  semaphore_close = sem_unlink (semaphoreWriteName);

  if (semaphore_close == -1)
    {
      fprintf (stderr, "Error in unlinking write semaphore, %s\n", strerror (errno));
      BailOut ("Could not unlink write Semaphore\n");
      returnValue = ERROR;
    }

  return;
}
 */