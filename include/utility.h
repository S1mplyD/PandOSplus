#ifndef UTILITIY_H
#define UTILITIY_H

// Una libreria di funzioni ausiliarie
#include "pandos_types.h"
#include "pcb.h"
#include "scheduler.h"
#include "umps3/umps/libumps.h"
#include "umps3/umps/arch.h"

typedef __SIZE_TYPE__ 	size_t;

extern unsigned int excTimeStart;
extern unsigned int cPStartT;
extern unsigned int ITtimeS;
extern unsigned int PLTTL;
extern unsigned int logT;
extern struct list_head semd_h;
extern pcb_t *currentProcess;
extern int semDevice[49];
extern int softBlockCounter;
extern struct list_head LO_readyQueue;
extern struct list_head HI_readyQueue;
extern int processCount;

void regExcTime(pcb_t *regTime, pcb_t *timeSlice);
void regToCurrentProcess(pcb_t *regTime, pcb_t *timeSlice, state_t *exceptionState);
void killProcess(pcb_t *p);
void setPtimeToExcTime(pcb_t *target);
void setTimeAndSchedule(pcb_t *target);
void setPcbToProperQueue(pcb_t *pcb);
unsigned int getExcTime();
void resumeIfTimeLeft(pcb_t *timeSlice, unsigned int excTime);
pcb_t *findPcb(struct list_head *list, int pid);
pcb_t *getPcb(int pid);
int *getDeviceSemAddr(int line, int device);
int *findDeviceSemKey(memaddr command_addr);
int *getTerminalSemAddr(int device, int mode);
void memcpy(void *dest, void *src, size_t n);
#endif