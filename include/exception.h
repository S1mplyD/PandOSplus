#ifndef EXCEPTION_H
#define EXCEPTION_H

#include "scheduler.h"
#include "pcb.h"
#include "asl.h"

void exceptionHandler();
void passUpOrDie(int exceptionIndex, state_t *exceptionState);
void syscallExceptionHandler(state_t *excstate);
void Create_Process();
void Passeren(int *semaddr);
void Verhogen(int *semaddr);
void Get_CPU_Time();
void Wait_For_Clock();
void Get_SUPPORTA_Data();
void Get_Process_ID(int parent);
void Yield();

#endif