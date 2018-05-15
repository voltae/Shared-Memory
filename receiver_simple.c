/* Simple receiver program with shared memory */

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
  int i = 0;
  char c;

  /* open a file descriptor with the given name of the shm */
  int shmfd = shm_open (SHM_NAME, O_RDONLY, 0);
  if (shmfd == -1)
    {
      printf ("Error open shm\n");
    }

  /* open the existing semaphores */
  sem_t *const sem_read = sem_open (SEM_READ, 0);
  if (sem_read == SEM_FAILED)
    {
      fprintf (stderr, "Error in open read semaphore\n");
    }
  sem_t *const sem_write = sem_open (SEM_WRITE, 0);
  if (sem_write == SEM_FAILED)
    {
      fprintf (stderr, "Error in open write semaphore.\n");
    }

  /* mapping the shm_memory into the process memory */
  char *memorypointer = mmap (NULL, LENGTH * sizeof (char), PROT_READ, MAP_SHARED, shmfd, 0);
  if (memorypointer == MAP_FAILED)
    {
      printf ("error mapping memory\n");
    }

  while (1)
    {
      sem_wait (sem_read);    // counts the reading places one down
      c = memorypointer[i++];
      if (c != EOF)
        {
          fputc (c, stdout);
          sem_post (sem_write);   // counts the writing places one up
        }
      else
        break;

    }

  /* unmap the shared memory */
  if (munmap (memorypointer, LENGTH * sizeof (char)) == -1)
    {
      printf ("Error in unmapping memory\n");
    }

  /* close the filepointer from shared memeory */
  if (close (shmfd) == -1)
    {
      printf ("error in closing shmfd\n");
    }
  /* unlink (delete) the shared memeory from file system */
  if (shm_unlink (SHM_NAME) == -1)
    {
      printf ("error in unlinking shm\n");
    }

  /* close and unlink semaphores */
  if (sem_close (sem_read) == -1)
    {
      fprintf (stderr, "Error in closing sem_read\n");
    }
  if (sem_close (sem_write) == -1)
    {
      fprintf (stderr, "Error in closing sem_write\n");
    }
  if (sem_unlink (SEM_READ) == -1)
    {
      fprintf (stderr, "Error in unlinking sem_read\n");
    }
  if (sem_unlink (SEM_WRITE) == -1)
    {
      fprintf (stderr, "Error in unlinking sem_write\n");
    }
  return 0;
}

