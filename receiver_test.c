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
#include <sys/sysctl.h> // for SHMMAX
#include <linux/shm.h>  // for SHMMAX
#include <limits.h>     // for ULONG max
#include <assert.h>

#define NAMELLENGTH 14

typedef enum isError {ERROR = -1, SUCCESS} isError;


/* Global constant semaphore read */
static char semaphoreReadName[NAMELLENGTH];  // Semaphore read Name with own annuminas uid
static char semaphoreWriteName[NAMELLENGTH];  // Semaphore write Name with own annuminas uid
static char sharedMemoryName[NAMELLENGTH];     // Shared memory name

static void setRessourcesName(void);
static void createSemaphores(void);
static void createSharedMemory (size_t size);

static void removeRessources(size_t size);
static void BailOut(const char *message);

/* references to the semaphores and the shared memory */
static sem_t *readSemaphore = NULL;
static sem_t *writeSemaphore = NULL;
static char *sharedMemory = NULL;

/* Number of read processes */
static unsigned int r;

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

/* Report Error and free ressources
 * Since we are in the receiver process, we are responsable for removing all ressources, even in the error case
 */
void BailOut(const char *message)
{
  if (message != NULL)
    {
      fprintf(stderr, "%s: %s\n",szCommand, message);
      exit (EXIT_FAILURE);
    }

  /* TODO: set here a function call to remove all ressources */
}
int main (int argc, char **argv)
{
/* store the progam name */
  szCommand = argv[0];

  int opt;    // option for getop
  int bOptionM = 0;   // Flag for the 'm' option
  int bError = 0;     // Flag for Option Error
  size_t buffersize = 0;
  long bufferTemp = 0;
  char *stringInt = NULL;

  /* check if no paramters are given */
  if (argc < 2)
    {
      print_usage ();
    }
  // check operants with getopt(3)
  while ((opt = getopt (argc, argv, "m:")) != -1)
    {
      switch (opt)
        {
          case 'm':
            {
              if (bOptionM)
                {
                  bError = 1;
                  break;
                }
              bOptionM = 1;
              bufferTemp = strtol (optarg, &stringInt,10);
              break;
            }
          case '?':
            bError = 1;
          break;
          default:
            assert (0);   /* should never be reached */
          break;
        }
    }

  if (optopt == 'm')    /* parameter 'm' with no argument */
    {
      print_usage ();
    }
  printf ("argc: %d, optind: %d\n", argc, optind);
  if (bError)
    {
      print_usage ();
    }
  if (argc != optind)
    {
      print_usage ();
    }
  if ((errno == ERANGE) || (strcmp (stringInt, "\0") != 0) || (bufferTemp == 0)|| (bufferTemp >> 29))
    {
      print_usage ();
    }

  buffersize = (size_t)bufferTemp;



  /* Set the ressource names */
  setRessourcesName ();

  /* open the semaphores */
  createSemaphores ();

  // initialize the reading char for the shared memory
  char readingChar;

  /* open the shared memory */
  createSharedMemory (buffersize);
  /* Semaphore wait process */

  while (1)
    {
      // write index is the same as the read index. writer must wait
      int semaphoreWait = sem_wait (readSemaphore);
      if (semaphoreWait == ERROR)
        {
          fprintf (stderr, "Error in waiting semaphore, %s\n", strerror (errno));
          BailOut ("Could not wait for Semaphore");
        }
      /* read a char from shared memory */
      readingChar = sharedMemory[r];

      /* is end of file detected? */
      if (readingChar == EOF)
        {
          break;
        }

      // print out the char to stdout

      fputc (readingChar, stdout);
      int retval = sem_post (writeSemaphore);
      // increment the counter
      r = (r + 1) % buffersize;

      if (retval == ERROR)
        {
          fprintf (stderr, "Error in posting semaphore, %s\n", strerror (errno));
          BailOut ("Could not post for Semaphore");
        }
    }

  /* unmap the memory */
  removeRessources (buffersize);

  return EXIT_SUCCESS;
}

static void setRessourcesName(void)
{
  /* uid=2329 on annuminas, means values are sem1 = 2329000, sem2 = 2329001, shm = 2329000 */
  int uid = getuid();  // These functions are always successful. man7

  /* create read semaphore */
  snprintf(semaphoreReadName, 13, "/sem_%d", 1000 * uid + 0);
  snprintf(semaphoreWriteName, 13, "/sem_%d", 1000 * uid + 1);
  snprintf(sharedMemoryName, 13, "/shm_%d", 1000 * uid+0);
}

static void createSemaphores(void)
{
  if (readSemaphore != NULL)
    {
      return;
    }

  /* Open an existing read semaphore from file*/
  readSemaphore = sem_open(semaphoreReadName, O_RDONLY, 0);
  if (readSemaphore == SEM_FAILED)
    {
      /* Error message */
      fprintf(stderr, "Error in creating semaphore, %s\n", strerror(errno));
      BailOut("Could not create Semaphore");
    }

  if (writeSemaphore != NULL)
    {
      return;
    }

  /* open an existing write semaphore from file*/
  writeSemaphore = sem_open(semaphoreWriteName, O_RDONLY, 0);
  if (writeSemaphore == SEM_FAILED)
    {
      /* Error message */
      fprintf(stderr, "Error in creating semaphore, %s\n", strerror(errno));
      BailOut("Could not create Semaphore");
    }
}

/* Open an existing shared memory, it should exist, because the sender has to set up */
static void createSharedMemory (size_t size)
{

  if (sharedMemory != NULL)
    {
      return;
    }

  if (strcmp(sharedMemoryName, "\0") == 0)
    {
      setRessourcesName();
    }
  // initialize shared memory
 fileDescr_sm = shm_open(sharedMemoryName, O_RDONLY, 0);
  if (fileDescr_sm == ERROR)
    {
      fprintf(stderr, "Error in opening shared memory, %s\n", strerror(errno));
      BailOut("Could not create shared memory");
    }

  // fixing the shared memory to a define size (coming from the agrv)
/*  if (ftruncate(fileDescr_sm, buffersize * sizeof(char)) == ERROR)
    {
      fprintf(stderr, "Error in truncating shared memory, %s\n", strerror(errno));
      BailOut("Could not truncate shared memory");
    }*/

  sharedMemory = mmap(NULL, size * sizeof(char), PROT_READ, MAP_SHARED, fileDescr_sm, 0);
  if (sharedMemory == MAP_FAILED)
    {
      fprintf(stderr, "Error in mapping memory, %s\n", strerror(errno));
      BailOut("Could not map the memory");
    }

  return;
}

static void removeRessources(size_t size)
{
  /* close the read semaphore */
  int semaphore_read_close;
  semaphore_read_close = sem_close(readSemaphore);
  if (semaphore_read_close == -1)
    {
      fprintf(stderr, "Error in closing read semaphore, %s\n", strerror(errno));
      BailOut("Could not close read Semaphore\n");
    }

  /* close the write semaphore */
  int semaphore_write_close = sem_close(writeSemaphore);
  if (semaphore_write_close == -1)
    {
      fprintf(stderr, "Error in closing write semaphore, %s\n", strerror(errno));
      BailOut("Could not close write Semaphore\n");
      exit (EXIT_FAILURE);
    }

  /* unlink the read semaphore */
  int semaphore_read_unlink = sem_unlink(semaphoreReadName);
  if (semaphore_read_unlink == -1)
  {
    fprintf(stderr, "Error in unlinking read semaphore: %s\n", strerror(errno));
    BailOut("Could not unlink read Semaphore\n");
    exit(EXIT_FAILURE);
  }

  /* unlink the semaphore, if not done, error EEXIST */
  int semaphore_write_unlink = sem_unlink(semaphoreWriteName);
  if  (semaphore_write_unlink == -1)
    {
      fprintf(stderr, "Error in unlinking write semaphore , %s\n", strerror(errno));
      BailOut("Could not unlink write Semaphore\n");
    }

  /* unmap the memory */
  /* TODO: unmap fails every time, but the memory is deleted */
  int unmapReturn = munmap(sharedMemory, size);
  if (unmapReturn == ERROR)
    {
      fprintf(stderr, "Error in unmapping memory, %s\n", strerror(errno));
      BailOut("Could not unmap memory\n");
      //return ERROR;
    }

  /* close the memory file */
  int closeMemory = close(fileDescr_sm);
  if (closeMemory == ERROR)
    {
      fprintf(stderr, "Error in closing memory file, %s\n", strerror(errno));
      BailOut("Could not close memory file\n");
    }

  /* unlink the memory file from /dev/shm/ */
  if (shm_unlink(sharedMemoryName) == ERROR)
    {
      fprintf(stderr, "Error in unlinking memory file, %s\n", strerror(errno));
      BailOut("Could not unlink memory file\n");
    }

}