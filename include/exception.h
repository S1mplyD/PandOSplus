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

int *_findDeviceSemKey(int line, int device);
int *findDeviceSemKey(memaddr command_addr);
int *_findTerminalSemKey(int device, int mode);
void bill_exception_time_to_process(pcb_t *pcb, unsigned int exception_time);
void restart_timeslice(unsigned int exception_time);
void exception_end_bill_before_continue(pcb_t *time_bill_pcb, pcb_t *time_slice_pcb);
void exception_end_bill_and_continue(pcb_t *time_bill_pcb, pcb_t *time_slice_pcb, state_t *saved_state);
void exception_end_bill_before_schedule(pcb_t *time_bill_target);
void exception_end_bill_and_schedule(pcb_t *time_bill_target);

#endif