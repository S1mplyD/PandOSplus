#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "pandos_types.h"
#include "umps3/umps/const.h"
#include "umps3/umps/libumps.h"
#include "pcb.h"

extern int processCount; //Contatore processi vivi
   
extern int softBlockCounter;       //Contatore processi bloccati
extern pcb_t *readyQueue;    //Coda processi ready
extern pcb_t *currentProcess;   //Puntatore al pcb corrente allo stato di running
extern int semDevice[DEVICECNT];       //Semaforo dei device

void scheduler();

#endif