#ifndef _GLOBAL_DATA_H_
#define _GLOBAL_DATA_H



/*
 *
 * PRS flags
 *
 */


typedef enum {
  PRS_FLAG_STREAM_RESET_REQUEST,
  PRS_FLAG_REQUEST_RECEIVED
} prs_flag;


  
void
global_data_set_flag (prs_flag flag);
void
global_data_clear_flag (prs_flag flag);
int
global_data_is_flag_set (prs_flag flag);




#endif
