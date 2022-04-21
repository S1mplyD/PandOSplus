#ifndef EXCEPTION_H
#define EXCEPTION_H

#include "pandos_types.h"
#include "umps3/umps/cp0.h"
#include "umps3/umps/libumps.h"
#include "utility.h"
#include "asl.h"
#include "syscalls.h"
#include "interrupts.h"
#include "scheduler.h"

extern pcb_t *currentProcess;

void exceptionHandler();
void passUpOrDie(int index, state_t *exceptionState);
int passeren(int *semAddr, pcb_t * pcb, pcb_t **unblocked_pcb);
int verhogen(int *semAddr, pcb_t *pcb, pcb_t **unblocked_pcb);



#endif