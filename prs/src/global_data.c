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
