#include "pcb.h"
#include "asl.h"
#include "umps3/umps/const.h"
#include "scheduler.h"

extern void test();

extern void uTLB_refillHandler();

extern void exception();

passupvector_t *passUpVector;

//Da usare in caso non vada 
state_t initState;

int main(){
    
    processCount = 0;           //Contatore processi vivi
    int softBlockCounter = 0;       //Contatore processi bloccati
    currentProcess = NULL;   //Puntatore al pcb corrente allo stato di running

    mkEmptyProcQ(&readyQueue);      //Inizializzo la readyQueue come una lista di PCB vuota

    //Inizializzazione PCB e ASL
    initPcbs();
    initASL();

    //Inizializzazione passUpVector
    passUpVector = (passupvector_t*) PASSUPVECTOR;
    passUpVector->exception_handler = (memaddr) KERNELSTACK;
    passUpVector->exception_stackPtr = (memaddr) KERNELSTACK;
    passUpVector->tlb_refill_handler = (memaddr) uTLB_refillHandler;
    passUpVector->tlb_refill_stackPtr = (memaddr) exception;

    //Inizializzazione device semafori a 0
    int i;
    for(i = 0; i < DEVICECNT; i++){
        semDevice[i] = 0;
    }

    //Imposto a 100ms l'Interval Timer
    LDIT(PSECOND);

    //Creazione processo
    pcb_t *p = allocPcb();
    p->p_prio = PROCESS_PRIO_LOW;               //Setto priorità bassa al processo
    p->p_s.status = IEPON | IMON | TEBITON;     //Abilitazione degli interrupt
    p->p_s.cause = LOCALTIMERINT;               //Abilitazione interrupt timer locale
    p->p_s.gpr[26] = RAMSTART;                  //Setto sp all'inizio della RAM
    p->p_s.pc_epc = (memaddr) test;             //Imposto PC
    p->p_s.gpr[24] = &p->p_s.pc_epc;    
    

    //Process Tree Fields to NULL
    p->p_parent = NULL;
    p->p_child.next = NULL;
    p->p_sib.next = NULL;
    
    //Imposto il tempo del processo a 0
    p->p_time = 0;

    //Setto il puntatore al semafor a NULL
    p->p_semAdd = NULL;

    //Setto la struttura di supporto a NULL
    p->p_supportStruct = NULL;

    processCount ++;

    currentProcess = NULL;
    insertProcQ(&(readyQueue),p);

    scheduler();
}