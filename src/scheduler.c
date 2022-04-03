#include "scheduler.h"

void scheduler(){
    
    //Prendo il processo dalla readyQueue
    currentProcess = removeProcQ(&readyQueue);

    //Se la coda dei processi non è vuota
    if(currentProcess != NULL){
        //Carico lo stato del processo
        LDST(&(currentProcess->p_s));
    }
    else{
        if(processCount == 0){
            HALT();
        } 
        else if (processCount > 0 && softBlockCounter > 0)
        {   
            //Abilito gli interrupt
            setSTATUS(IECON | IMON);
            //Carico un valore altissimo nel PLT
            setTIMER(9999999999);

            WAIT();
        }
        else if (processCount > 0 && softBlockCounter ==0)
        {
            PANIC();
        }
        
    }
}