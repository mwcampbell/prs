#include <pthread.h>
#include "global_data.h"



/*
 *
 * Mutex to make everything thread safe
 *
 */



static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;



/*
 *
 * Global data
 *
 */


static int global_flags = 0;
static int soundcard_fd = -1;
static int soundcard_duplex = 0;



void
global_data_set_flag (prs_flag flag)
{
  pthread_mutex_lock (&mut);
  global_flags = (global_flags | flag);
  pthread_mutex_unlock (&mut);
}



void
global_data_clear_flag (prs_flag flag)
{
  pthread_mutex_lock (&mut);
  if (global_flags & flag)
    global_flags = (global_flags ^ flag);
  pthread_mutex_unlock (&mut);
}



int
global_data_is_flag_set (prs_flag flag)
{
  int rv;

  pthread_mutex_lock (&mut);
  rv = global_flags;
  pthread_mutex_unlock (&mut);
  return rv ?1 :0;
}



int
global_data_get_soundcard_fd (void)
{
  int rv;

  pthread_mutex_lock (&mut);
  rv = soundcard_fd;
  pthread_mutex_unlock (&mut);
  return rv;
}



void
global_data_set_soundcard_fd (int fd)
{
  pthread_mutex_lock (&mut);
  soundcard_fd = fd;
  pthread_mutex_unlock (&mut);
}



int
global_data_get_soundcard_duplex (void)
{
  int rv;

  pthread_mutex_lock (&mut);
  rv = soundcard_duplex;
  pthread_mutex_unlock (&mut);
  return rv;
}



void
global_data_set_soundcard_duplex (int duplex)
{
  pthread_mutex_lock (&mut);
  soundcard_duplex = duplex;
  pthread_mutex_unlock (&mut);
}
