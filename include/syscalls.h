#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <pandos_types.h>

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