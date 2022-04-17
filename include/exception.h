#ifndef EXCEPTION_H
#define EXCEPTION_H

#include "scheduler.h"
#include "pcb.h"
#include "asl.h"
#include "string.h"
#include "interrupts.h"
#include "umps3/umps/cp0.h"
#include "listx.h"
#include "utility.h"

void exceptionHandler();
void passUpOrDie(int exceptionIndex);
void syscallExceptionHandler();
void Create_Process();
void Terminate_Process();
void Passeren();
void Verhogen();
void Do_IO_Device();
void Get_CPU_Time();
void Wait_For_Clock();
void Get_SUPPORTA_Data();
void Get_Process_ID();
void Yield();
void *memcpy(void *dest, const void *src, size_t n);
int passeren(int *sem_key, pcb_t *pcb, pcb_t **unblocked_pcb);
int verhogen(int *sem_key, pcb_t *pcb, pcb_t **unblocked_pcb);

#endif