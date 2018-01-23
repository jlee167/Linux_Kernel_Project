#ifndef _SYSCALLS_ASM_H
#define _SYSCALLS_ASM_H

#ifndef ASM

#include "syscalls.h"
#include "x86_desc.h"

/*keyboard wrapper for assembly linkage*/
void context_switch(void);

#endif

#endif
