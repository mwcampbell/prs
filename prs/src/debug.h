#ifndef _DEBUG_H
#define _DEBUG_H_



#define DEBUG_FLAGS_AUTOMATION 0x01
#define DEBUG_FLAGS_SCHEDULER 0x02
#define DEBUG_FLAGS_DATABASE 0x04
#define DEBUG_FLAGS_TELNET 0x08
#define DEBUG_FLAGS_GENERAL 0x10
#define DEBUG_FLAGS_MIXER 0x20
#define DEBUG_FLAGS_FILE_INFO 0x40
#define DEBUG_FLAGS_CODEC 0x80
#define DEBUG_FLAGS_ALL 0xff

void
debug_set_flags (int debug_flags);
void
debug_printf (int debug_flags,
	      const char *format,
	      ...);



#endif
