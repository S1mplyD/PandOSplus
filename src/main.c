#include "pcb.h"
#include "asl.h"
#include "umps3/umps/const.h"

void uTLB_RefillHandler () {
setENTRYHI(0x80000000);
setENTRYLO(0x00000000);
TLBWR();
LDST ((void*) 0x0FFFF000);
}

extern void test();

int main(){
    
    int processCount = 0;           //Contatore processi vivi
    int softBlockCounter = 0;       //Contatore processi bloccati
    struct list_head readyQueue;    //Coda processi ready
    pcb_t *currentProcess = NULL;   //Puntatore al pcb corrente allo stato di running
    int semDevice[DEVICECNT];       //Semaforo dei device

    mkEmptyProcQ(&readyQueue);      //Inizializzo la readyQueue come una lista di PCB vuota

    //Inizializzazione PCB e ASL
    initPcbs();
    initASL();
    
    //Inizializzazione device semafori a 0
    int i;
    for(i = 0; i < DEVICECNT; i++){
        semDevice[i] = 0;
    }

    pcb_t *p = allocPcb();
    p->p_prio = PROCESS_PRIO_LOW;       //Setto priorità bassa al processo
    p->p_s.status = IEPON | USERPON;    //Abilitazione degli interrupt
    p->p_s.cause = LOCALTIMERINT;       //Abilitazione interrupt timer locale
    p->p_s.gpr[26] = RAMSTART;          //Setto sp all'inizio della RAM
    p->p_s.pc_epc = (memaddr) test;     //Imposto PC
    p->p_s.gpr[24] = &p->p_s.pc_epc;    
    //Imposto a 100ms l'Interval Timer
    LDIT(100);

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

    scheduler();
}

