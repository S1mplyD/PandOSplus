#ifndef UTILITIES_H
#define UTILITIES_H

// Una libreria di funzioni ausiliarie

#include "pcb.h"
#include "asl.h"

extern pcb_t *currentProcess;
extern int semDevice[49];
extern int softBlockCounter;
extern int processCount;
extern struct list_head LO_readyQueue;
extern struct list_head HI_readyQueue;

//Rendi un processo come Bruce Wayne
void killProcess(pcb_t *p);

//Ritorna il processo che ha id == pid
pcb_t *getPcb(int pid);

pcb_t *findPcb(struct list_head *queue, int pid);

//Mette il pcb nell'apposita readyQueue
void *setPcbToProperQueue(pcb_t *pcb);

#endif