#include "exception.h"

void exceptionHandler()
{
  // Controllo il tempo dell'inizio dell'eccezione
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
  else // Qui vengono gestite le eccezioni da 4 a 7 e da 9 a 12
  {
    passUpOrDie(GENERALEXCEPT, exceptionState);
  }
};

void passUpOrDie(int index, state_t *exceptionState)
{

  if (currentProcess->p_supportStruct != NULL)
  {
    // Salvo l'eccezione corrente nel currentProcess
    currentProcess->p_supportStruct->sup_exceptState[index] = *exceptionState;

    // Recupero il context dal currentProcess
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
Operazione P su semaforo
*/

int passeren(int *semAddr, pcb_t *pcb, pcb_t **unblock)
{

  int result;

  if (*semAddr == 0)
  {
    // Il processo è bloccato
    insertBlocked(semAddr, pcb);
    result = 0;
  }
  else if (headBlocked(semAddr) != NULL)
  {
    // Sblocco il pcb bloccato
    pcb_t *p = removeBlocked(semAddr);
    // Metto il processo sbloccato nella sua apposita coda
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
Operazione V su semaforo
*/
int verhogen(int *semAddr, pcb_t *pcb, pcb_t **unblock)
{

  int result;

  if (*semAddr == 1)
  {
    // Il processo è bloccato
    insertBlocked(semAddr, pcb);
    result = 0;
  }
  else if (headBlocked(semAddr) != NULL)
  {
    // Sblocco il pcb bloccato
    pcb_t *p = removeBlocked(semAddr);
    // Metto il processo sbloccato nella sua apposita coda
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
