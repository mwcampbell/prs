#ifndef _DEBUG_H
#define _DEBUG_H_



#define DEBUG_FLAGS_AUTOMATION 0X01
#define DEBUG_FLAGS_SCHEDULER 0X02
#define DEBUG_FLAGS_DATABASE 0X04
#define DEBUG_FLAGS_TELNET 0X08
#define DEBUG_FLAGS_GENERAL 0X10
#define DEBUG_FLAGS_ALL 0X1F

void
debug_set_flags (int debug_flags);
void
debug_printf (int debug_flags,
	      const char *format,
	      ...);



#endif
