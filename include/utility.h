#ifndef UTILITIY_H
#define UTILITIY_H

// Una libreria di funzioni ausiliarie

#include "pcb.h"
#include "asl.h"
#include <stddef.h>
#include "umps3/umps/libumps.h"
#include "scheduler.h"
#include "umps3/umps/arch.h"

extern unsigned int excTimeStart;
extern unsigned int cPStartT;
extern unsigned int ITtimeS;
extern unsigned int PLTTL;
extern unsigned int logT;

void  regExcTime(pcb_t *regTime, pcb_t *timeSlice);
void regToCurrentProcess(pcb_t *regTime, pcb_t *timeSlice, state_t *exceptionState);
void killProcess(pcb_t *p);
void setTimeNoSchedule(pcb_t *target);
void setTimeAndSchedule(pcb_t *target);
void setPcbToProperQueue(pcb_t *pcb);
unsigned int  getExcTime();
void resumeIfTimeLeft(pcb_t *timeSlice, unsigned int excTime);
pcb_t * findPcb(struct list_head *list, int pid);
pcb_t *getPcb(int pid);
int *getDeviceSemAddr(int line, int device);
int *findDeviceSemKey(memaddr command_addr);
int *getTerminalSemAddr(int device, int mode);
void memcpy(void* to, void* from, size_tt n);
#endif