#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <umps3/umps/cp0.h>
#include <umps3/umps/libumps.h>
#include <umps3/umps/arch.h>
#include <exception.h>
#include <utility.h>
#include <asl.h>
#include <pcb.h>
#include <syscalls.h>
#include <interrupts.h>
#include <scheduler.h>

void exceptionHandler();
void passUpOrDie(int exceptionIndex, state_t *exceptionState);

void *memcpy(void *dest, const void *src, size_t n);
int passeren(int *semAddr, pcb_t *pcb, pcb_t **unblock);
int verhogen(int *semAddr, pcb_t *pcb, pcb_t **unblock);

#endif