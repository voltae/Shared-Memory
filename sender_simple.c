/* Simple sender program with shared memory */

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>    /* Interface of shared memory */
#include <sys/stat.h>    /* From mode constants */
#include <fcntl.h>    /* For O_* constants */
#include <unistd.h>        /* ftruncare and close */

#include <semaphore.h>

#define SHM_NAME "/shm_shared"
#define SEM_READ "/sem_read"
#define SEM_WRITE "/sem_write"

#define LENGTH 16

int main (int argc, char **argv)
{
  /* counter */
  int w = 0;
  signed char c;

  /* open a file descriptor with the given name of the shm */
  int shmfd = shm_open (SHM_NAME, O_CREAT | O_EXCL | O_RDWR, S_IRWXU);
  if (shmfd == -1)
    {
      printf ("Error open shm\n");
    }

  /* Create the semaphores for reading and writing
   * reading begins by 0, writing begins by 16 */
  sem_t *const sem_read = sem_open (SEM_READ, O_CREAT | O_EXCL, S_IRWXU, 0);
  if (sem_read == SEM_FAILED)
    {
      fprintf (stderr, "Error in create read semaphore.\n");
    }
  sem_t *const sem_write = sem_open (SEM_WRITE, O_CREAT | O_EXCL, S_IRWXU, LENGTH);
  if (sem_write == SEM_FAILED)
  {
    fprintf (stderr, "Error in create write semaphore.\n");
  }


  /* truncating the memory to the needed size */
  if (ftruncate (shmfd, LENGTH * sizeof (char)) == -1)
    {
      printf ("error intruncate memory \n");
    }

  /* mapping the shm_memory into the process memory */
  char *memorypointer = mmap (NULL, LENGTH * sizeof (char), PROT_WRITE, MAP_SHARED, shmfd, 0);
  if (memorypointer == MAP_FAILED)
    {
      printf ("error mapping memory\n");
    }

  /* Emulate EOF in Shell with control-D */
  while ((c = fgetc (stdin)) != EOF)
    {
      sem_wait (sem_write);     // counts the writing places on down
      memorypointer[w] = c;
      sem_post (sem_read);      // counts the reading places on up
      w = (w + 1) % LENGTH;
    }

  /* Signalize the receiver that reading is not yet over EOF! has to be read */
  memorypointer[w] = EOF;
  sem_post (sem_read);

  fprintf (stdout, "Written EOF: %d to memory on position: %d\n",memorypointer[w], w);
  /* the receiver has to remove all resources */
  if (munmap( memorypointer, LENGTH * sizeof (char)) == -1)
  {
      printf("Error in unmapping memory\n");
  }
  /*
  if (close(shmfd) == -1)
  {
      printf("error in closing shmfd\n");
  }
  if (shm_unlink(SHM_NAME) == -1)
  {
      printf("error in unlinking shm\n");
  }
*/
  return 0;
}   	

