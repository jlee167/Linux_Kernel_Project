#ifndef _IDT_ASM_H
#define _IDT_ASM_H

#ifndef ASM

#include "keyboard.h"
#include "rtc.h"
#include "schedule.h"

/*keyboard wrapper for assembly linkage*/
void keyboard_wrapper(void);
/*rtc wrapper for assembly linkage*/
void rtc_wrapper(void);

void pit_wrapper(void);

#endif

#endif

