#include "scheduler.h"

extern struct list_head HI_readyQueue;
extern struct list_head LO_readyQueue;
extern int processCount;      // Contatore processi vivi
extern int softBlockCounter;  // Contatore processi bloccati
extern pcb_t *currentProcess; // Puntatore al pcb corrente allo stato di running

unsigned int process_job_start_time;

void scheduler()
{
    klog_print("scheduler//");
    if (emptyProcQ(&HI_readyQueue))
    {
        klog_print("HI empty//");
        if (emptyProcQ(&LO_readyQueue))
        {  
            klog_print("LO empty//");
            if (processCount == 0)
            {
                klog_print("No more process");
                HALT();
            }
            else if (processCount > 0 && softBlockCounter > 0)
            {
                // Abilito gli interrupt
                setSTATUS(getSTATUS() | IECON | IMON);
                // Carico un valore altissimo nel PLT
                setTIMER(0xFFFFFF);

                WAIT();
            }
            else if (processCount > 0 && softBlockCounter == 0)
            {
                PANIC();
            }
        }
        else
        {
            klog_print("LOW//");
            currentProcess = removeProcQ(&LO_readyQueue);
            klog_print("removeProcQLow//");
            STCK(process_job_start_time);
            setTIMER(TIMESLICE);
            klog_print("setTimer//");
            LDST((void *)&currentProcess->p_s);
            klog_print("fagiolini3//");
        }
    }
    else
    {
        klog_print("HIGH//");
        currentProcess = removeProcQ(&HI_readyQueue);
        klog_print("fagiolini3.2//");
        LDST(&currentProcess->p_s);
        klog_print("fagiolini3.1//");
    }

}