#ifndef UTILITIY_H
#define UTILITIY_H

// Una libreria di funzioni ausiliarie

#include "pcb.h"
#include "asl.h"
#include <stddef.h>
#include "umps3/umps/libumps.h"
#include "scheduler.h"
#include "umps3/umps/arch.h"

extern unsigned int ITtimeS;
extern unsigned int cPStartT;
extern unsigned int excTimeStart;
extern unsigned int PLTTL;
extern unsigned int logT;

// Rendi un processo come Bruce Wayne
void killProcess(pcb_t *p);
pcb_t *getPcb(int pid);
pcb_t *findPcb(struct list_head *queue, int pid);
void setPcbToProperQueue(pcb_t *pcb);
void *memcpy(void *dest, const void *src, size_t n);
void regExcTime(pcb_t *regTime, pcb_t *timeSlice);
void regToCurrentProcess(pcb_t *regTime, pcb_t *timeSlice, state_t *exceptionState);
void setTimeNoSchedule(pcb_t *target);
void setTimeAndSchedule(pcb_t *target);
unsigned int getExcTime();
void resumeIfTimeLeft(pcb_t *timeSlice, unsigned int exception_time);
int *getDeviceSemAddr(int line, int device);
int *findDeviceSemKey(memaddr command_addr);
int *getTerminalSemAddr(int device, int mode);

#endif