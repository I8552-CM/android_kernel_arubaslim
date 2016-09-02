/****************************************************************************

**

** COPYRIGHT(C) : Samsung Electronics Co.Ltd, 2006-2012 ALL RIGHTS RESERVED

**

** AUTHOR       :  <@samsung.com>			@LDK@

**                                                                      @LDK@

****************************************************************************/

#ifndef __SEC_DUMP_H__
#define __SEC_DUMP_H__


#ifdef CONFIG_EVENT_LOGGING
typedef struct event_header {
	struct timeval timeVal;
	__u16 class;
	__u16 repeat_count;
	__s32 payload_length;
} EVENT_HEADER;
#endif

struct _mem_param {
	unsigned short addr;
	unsigned long data;
	int dir;
};

extern int charging_boot;
extern int dump_enable_flag;


/* TODO: add more definitions */

#endif	/* __SEC_DUMP_H__ */

