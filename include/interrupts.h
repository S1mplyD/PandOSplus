#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "pandos_types.h"
#include "umps3/umps/libumps.h"
#include "umps3/umps/arch.h"
#include "umps3/umps/cp0.h"
#include "exception.h"
#include "scheduler.h"
#include "utility.h"

extern pcb_t *currentProcess;
extern int semDevice[49];
extern int softBlockCounter;

void interruptHandler(state_t *exceptionState);
void interrupt(int lineNumber, state_t *exceptionState);

#endif