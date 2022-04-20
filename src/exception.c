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


/**
 * Handle exceptions based on the exception code.
 **/
void exceptionHandler(){

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
  int exCode = CAUSE_GET_EXCCODE(exceptionState->cause);

  if (exCode == 0)
  {
    interruptHandler(exceptionState); //... semplicemente richiama l'Interrupt Handler
  }
  else if (exCode > 0 && exCode < 4) // Qui vengono gestite le eccezioni di codice da 1 a 3 compresi
  {
    passUpOrDie(PGFAULTEXCEPT, exceptionState);
  }
  else if (exCode == 8) // Qui vengono gestite delle eccezioni specifiche, diverse da quelle precedenti
  {
    syscallExceptionHandler(exceptionState);
  }
  else
  {
    passUpOrDie(GENERALEXCEPT, exceptionState);
  }
};


//-------------------------
//        UTILITIES
//-------------------------


/**
 * Perform a standard pass-up-or-die operation, delegating the the user's exception handler, if defined, killProcessing the
 * process otherwise.
 *
 * @param index Either GENERALEXCEPT or PGFAULTEXCEPT
 * @param exceptionState The state before the exception, saved in the bios data page
 **/
void passUpOrDie(int index, state_t *exceptionState) {

  if (currentProcess->p_supportStruct != NULL) {
    // PASS UP

    // save to the process passup vector the current exception state
    currentProcess->p_supportStruct->sup_exceptState[index] = *exceptionState;

    // recover the process passup vector context
    context_t context = currentProcess->p_supportStruct->sup_exceptContext[index];

    // Handle time from here
     regExcTime(currentProcess, currentProcess);
    
    // load the context
    LDCXT(context.stackPtr, context.status, context.pc);
  }
  else {
    // DIE
    killProcess(currentProcess);

    scheduler();
  }
}


/**
 * A Passeren operation a binary semaphore. Sometimes access to the unblocked pcb is needed, hence why the pointer to
 * the pointer of a pcb in the arguments. This function is specular to the verhogen function.
 *
 * @param semAddr The address of the semaphore
 * @param pcb The pcb of the process that called the P
 * @param unblocked_pcb A pointer to the pointer to the unblocked pcb
 *
 * @return 0, if the calling process is blocked on the semaphore; 1, if the operation unblocked a pcb; 2, if the
 * semaphore value got decremented
 **/
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


/**
 * A Verhogen operation a binary semaphore. Sometimes access to the unblocked pcb is needed, hence why the pointer to
 * the pointer of a pcb in the arguments. This function is specular to the passeren function.
 *
 * @param semAddr The address of the semaphore
 * @param pcb The pcb of the process that called the P
 * @param unblocked_pcb A pointer to the pointer to the unblocked pcb
 *
 * @return 0, if the calling process is blocked on the semaphore; 1, if the operation unblocked a pcb; 2, if the
 * semaphore value got decremented
 **/
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
    result = 1;
  }
  else
  {
    (*semAddr)++;
    result = 2;
  }

  return result;
}

