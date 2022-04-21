#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "pandos_types.h"
#include "utility.h"
#include "exception.h"
#include "scheduler.h"
#include "umps3/umps/libumps.h"
#include "umps3/umps/cp0.h"

extern pcb_t *currentProcess;
extern int semDevice[49];
extern int softBlockCounter;
extern int processCount;

void syscallExceptionHandler(state_t *exceptionState);
void Create_Process(state_t *exceptionState);
void Terminate_Process(state_t *exceptionState);
void Passeren(state_t *exceptionState);
void Verhogen(state_t *exceptionState);
void Do_IO_Device(state_t *exceptionState);
void Get_CPU_Time(state_t *exceptionState);
void Wait_For_Clock(state_t *exceptionState);
void Get_SUPPORT_Data(state_t *exceptionState);
void Get_Process_ID(state_t *exceptionState);
void Yield(state_t *exceptionState);

#endif