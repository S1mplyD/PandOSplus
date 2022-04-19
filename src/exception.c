#include <umps3/umps/cp0.h>
#include <umps3/umps/libumps.h>
#include <umps3/umps/arch.h>

#include <exception.h>
#include <utility.h>
#include <asl.h>
#include <pcb.h>
#include <syscalls.h>
#include <interrupts.h>
#include <scheduler.h>

extern struct list_head semd_h;
extern pcb_t *currentProcess;
extern int semDevice[49];
extern int softBlockCounter;
extern struct list_head LO_readyQueue;
extern struct list_head HI_readyQueue;
extern int processCount;

void exceptionHandler()
{

  STCK(excTimeStart);

  if (currentProcess != NULL)
  {
    PLTTL = getTIMER();

    if ((int)PLTTL > 0)
    {
      setTIMER(0xFFFFFFFF);
    }

    // tempo da quando è iniziata l'exception - tempo di inizio del currentProcess
    currentProcess->p_time += excTimeStart - cPStartT;
  }

  logT = 0;

  state_t *exceptionState = (state_t *)BIOSDATAPAGE;
  int exc_code = CAUSE_GET_EXCCODE(exceptionState->cause);

  if (exc_code == 0)
  {
    interruptHandler(exceptionState); //... semplicemente richiama l'Interrupt Handler
  }
  else if (exc_code > 0 && exc_code < 4) // Qui vengono gestite le eccezioni di codice da 1 a 3 compresi
  {
    passUpOrDie(PGFAULTEXCEPT, exceptionState);
  }
  else if (exc_code == 8) // Qui vengono gestite delle eccezioni specifiche, diverse da quelle precedenti
  {
    syscallExceptionHandler(exceptionState);
  }
  else
  {
    passUpOrDie(GENERALEXCEPT, exceptionState);
  }
};

void passUpOrDie(int index, state_t *exceptionState)
{

  if (currentProcess->p_supportStruct != NULL)
  {

    // Salvo l'eccezione nel currentProcess
    currentProcess->p_supportStruct->sup_exceptState[index] = *exceptionState;

    context_t context = currentProcess->p_supportStruct->sup_exceptContext[index];

    regExcTime(currentProcess, currentProcess);

    // Carico il context
    LDCXT(context.stackPtr, context.status, context.pc);
  }
  else
  {
    killProcess(currentProcess);

    scheduler();
  }
}

/*
Funzione che effettua una operazione P su un semaforo
*/
int passeren(int *semAddr, pcb_t *pcb, pcb_t **unblock)
{

  int result;

  if (*semAddr == 0)
  {
    insertBlocked(semAddr, pcb);
    result = 0;
  }
  else if (headBlocked(semAddr) != NULL)
  {
    // Sblocco il pcb bloccato
    pcb_t *p = removeBlocked(semAddr);
    setPcbToProperQueue(p);
    *unblock = p;
    result = 1;
  }
  else
  {
    (*semAddr)--;
    result = 2;
  }

  return result;
}

/*
Funzione che effettua una operazione V su un semaforo
*/
int verhogen(int *semAddr, pcb_t *pcb, pcb_t **unblock)
{

  int result;

  if (*semAddr == 1)
  {
    insertBlocked(semAddr, pcb);
    result = 0;
  }
  else if (headBlocked(semAddr) != NULL)
  {
    // Sblocco il pcb bloccato
    pcb_t *p = removeBlocked(semAddr);
    setPcbToProperQueue(p);
    *unblock = p;
    klog_print("setPcbToProperQueyeV_OK//");
    result = 1;
  }
  else
  {
    (*semAddr)++;
    result = 2;
  }

  return result;
}