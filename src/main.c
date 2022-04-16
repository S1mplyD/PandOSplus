#include "pcb.h"
#include "asl.h"
#include "umps3/umps/const.h"
#include "scheduler.h"
#include "klog.h"
#include "exception.h"

int processCount;
int softBlockCounter;
struct list_head HI_readyQueue;    //Coda processi ready high priority
struct list_head LO_readyQueue;    //Coda processi ready low priority
pcb_t *currentProcess;   //Puntatore al pcb corrente allo stato di running
int semDevice[49];       //Semaforo dei device
passupvector_t *passUpVector = (passupvector_t *) PASSUPVECTOR;
int pid;

extern void uTLB_RefillHandler();
extern void test();

int main(){
    //Inizializzazione passUpVector
    passUpVector->tlb_refill_handler = (memaddr) uTLB_RefillHandler;
    passUpVector->tlb_refill_stackPtr = KERNELSTACK;
    passUpVector->exception_handler = (memaddr) exceptionHandler;
    passUpVector->exception_stackPtr = KERNELSTACK;

    //Inizializzazione PCB e ASL
    initPcbs();
    initASL();

    processCount = 0;            //Contatore processi vivi   
    softBlockCounter = 0;   //Contatore processi bloccati
    mkEmptyProcQ(&HI_readyQueue);      //Inizializzo la HI_readyQueue come una lista di PCB vuota
    mkEmptyProcQ(&LO_readyQueue);      //Inizializzo la LO_readyQueue come una lista di PCB vuota
    currentProcess = NULL;   //Puntatore al pcb corrente allo stato di running
    pid = 0;

    //Inizializzazione device semafori a 0
    int i;
    for(i = 0; i < 49; i++){
        semDevice[i] = 0;
    }

    //Imposto a 100ms l'Interval Timer
    LDIT(PSECOND);

    //Creazione processo
    pcb_t *p = allocPcb();
    p->p_prio = PROCESS_PRIO_LOW;               //Setto priorità bassa al processo
    p->p_s.status = IEPON | IMON | TEBITON | USERPON;     //Abilitazione degli interrupt
    
    RAMTOP(p->p_s.reg_sp);                  //Setto sp all'inizio della RAM
    
    p->p_s.pc_epc = (memaddr) test;             //Imposto PC
    p->p_s.pc_epc = (memaddr) test;
    p->p_pid = pid;
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
    insertProcQ(&LO_readyQueue,p);
    processCount++;
    
    scheduler();

    return 0;
}