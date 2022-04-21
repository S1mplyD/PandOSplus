#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "pcb.h"
#include "umps3/umps/libumps.h"
#include "utility.h"

extern pcb_t *currentProcess;
extern int softBlockCounter;
extern struct list_head LO_readyQueue;
extern struct list_head HI_readyQueue;
extern int processCount;

void scheduler();

#endif