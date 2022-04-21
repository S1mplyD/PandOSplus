#include "pcb.h"
#include "asl.h"
#include "exception.h"
#include "scheduler.h"
#include "utility.h"

extern int test();
extern int uTLB_RefillHandler();

// processid
int pid;

int processCount;

int softBlockCounter;

// ReadyQueue ad alta e bassa priorità
struct list_head LO_readyQueue;
struct list_head HI_readyQueue;

// Il processo correntemente in esecuzione
pcb_t *currentProcess;

// Semafori dei device
int semDevice[MAXSEM];

passupvector_t *passUpVector = (passupvector_t *)PASSUPVECTOR;

int main(void)
{

  // Inizializzo il passUpVector
  passUpVector->tlb_refill_handler = (memaddr)uTLB_RefillHandler;
  passUpVector->tlb_refill_stackPtr = KERNELSTACK;
  passUpVector->exception_handler = (memaddr)exceptionHandler;
  passUpVector->exception_stackPtr = KERNELSTACK;

  // Inizializzo le strutture base
  initPcbs();
  initASL();

  // Inizializzo le variabili
  pid = 0;
  processCount = 0;
  softBlockCounter = 0;
  mkEmptyProcQ(&LO_readyQueue);
  mkEmptyProcQ(&HI_readyQueue);
  currentProcess = NULL;

  // Setto tutti i semafori a 0
  for (int i = 0; i < MAXSEM; i++)
  {
    semDevice[i] = 0;
  }

  // Leggo il TOD
  STCK(ITtimeS);
  ITtimeS += 2 * PSECOND;
  // Carico 100 ms nell'interval timer
  LDIT(PSECOND);

  // Alloco un processo
  pcb_t *p = allocPcb();
  // // Abilitazione degli interrupt
  p->p_s.status = IEPON | TEBITON | IMON;

  // Setto sp all'inizio della RAM
  RAMTOP(p->p_s.reg_sp);

  // Imposto PC
  p->p_s.pc_epc = (memaddr)test;
  p->p_s.reg_t9 = (memaddr)test;

  // Aggiungo il processo alla Low Priority readyQueue
  insertProcQ(&LO_readyQueue, p);
  processCount++;

  scheduler();

  return 0;
}
