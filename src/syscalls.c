#include "syscalls.h"

/*
Funzione che gestisce le syscall
*/

void syscallExceptionHandler(state_t *exceptionState)
{
  int processorMode = (exceptionState->status & STATUS_KUp) >> 3;
  int syscall = (int)exceptionState->reg_a0;
  if (processorMode == 0 && syscall < 0)
  {

    switch (syscall)
    {
    case CREATEPROCESS:
      Create_Process(exceptionState);
      break;
    case TERMPROCESS:
      Terminate_Process(exceptionState);
      break;
    case PASSEREN:
      Passeren(exceptionState);
      break;
    case VERHOGEN:
      Verhogen(exceptionState);
      break;
    case DOIO:
      Do_IO_Device(exceptionState);
      break;
    case GETTIME:
      Get_CPU_Time(exceptionState);
      break;
    case CLOCKWAIT:
      Wait_For_Clock(exceptionState);
      break;
    case GETSUPPORTPTR:
      Get_SUPPORT_Data(exceptionState);
      break;
    case GETPROCESSID:
      Get_Process_ID(exceptionState);
      break;
    case YIELD:
      Yield(exceptionState);
      break;
    default:
      exceptionState->cause = (exceptionState->cause & ~CAUSE_EXCCODE_MASK) | (EXC_RI << CAUSE_EXCCODE_BIT);
      passUpOrDie(GENERALEXCEPT, exceptionState);
      break;
    }
  }
  else if (processorMode == 1 && syscall < 0)
  {
    klog_print("processorMode == 1 && syscall < 0//\n");
    // Processore in usermode
    exceptionState->cause = (exceptionState->cause & ~CAUSE_EXCCODE_MASK) | (EXC_RI << CAUSE_EXCCODE_BIT);
    passUpOrDie(GENERALEXCEPT, exceptionState);
  }
  else
  {
    klog_print("syscall >=0//\n");
    passUpOrDie(GENERALEXCEPT, exceptionState);
  }
}

/*
SYSCALLS
*/

/*
SYSCALL -1: CREATEPROCESS
SYSCALL(CREATEPROCESS,state_t *statep, int prio, support_t *supportp)
Questa system call crea un nuovo processo come figlio del chiamante.

– prio indica se si tratta di un processo ad alta priorità
– supportp e’ un puntatore alla struttura di supporto del processo
– restituisce il pid del processo

Restituisce 0 se ha successo, -1 altrimenti
*/
void Create_Process(state_t *exceptionState)
{
  pcb_t *proc = allocPcb();

  if (proc == NULL)
  {
    exceptionState->reg_v0 = -1;
  }
  else
  {
    proc->p_s = *(state_t *)exceptionState->reg_a1;
    proc->p_prio = exceptionState->reg_a2;
    if ((void *)exceptionState->reg_a3 != NULL)
      proc->p_supportStruct = (support_t *)exceptionState->reg_a3;
    setPcbToProperQueue(proc);
    insertChild(currentProcess, proc);
    proc->p_time = 0;
    processCount++;
    exceptionState->reg_v0 = proc->p_pid;
  }
  exceptionState->pc_epc = exceptionState->pc_epc + WORDLEN;
  regToCurrentProcess(currentProcess, currentProcess, exceptionState);
}

/*
SYSCALL -2: Terminate_Process
void SYSCALL(TERMPROCESS, int pid, 0, 0)
– Quando invocata, la SYS2 termina il processo indicato dal secondo parametro insieme a tutta
    la sua progenie.
– Se il secondo parametro e’ 0 il bersaglio e’ il processo invocante.

*/

void Terminate_Process(state_t *exceptionState)
{
  int pidTokill = exceptionState->reg_a1;

  if (pidTokill == 0)
  {
    killProcess(currentProcess);
  }
  else
  {
    exceptionState->pc_epc += WORDLEN;
    currentProcess->p_s = *exceptionState;
    pcb_t *tokill = getPcb(pidTokill);
    killProcess(tokill);
    if (currentProcess != NULL)
    {
      regToCurrentProcess(currentProcess, currentProcess, exceptionState);
    }
  }

  scheduler();
}

/*
SYSCALL -3: Passeren
void SYSCALL(PASSEREN, int *semaddr, 0, 0)
– Operazione di richiesta di un semaforo binario. Il valore del semaforo è memorizzato nella
    variabile di tipo intero passata per indirizzo. L’indirizzo della variabile agisce da identificatore
    per il semaforo.

*/

void Passeren(state_t *exceptionState)
{
  int *semaddr = (int *)exceptionState->reg_a1;

  pcb_t *un = NULL;

  int p_rc = passeren(semaddr, currentProcess, &un);
  exceptionState->pc_epc += WORDLEN;
  if (p_rc == 0)
  {
    currentProcess->p_s = *exceptionState;
    currentProcess = NULL;
    setTimeAndSchedule(currentProcess);
  }
  else
  {
    regToCurrentProcess(currentProcess, currentProcess, exceptionState);
  }
}
/*
SYSCALL -4: Verhogen
void SYSCALL(VERHOGEN, int *semaddr, 0, 0)
– Operazione di rilascio su un semaforo binario. Il valore del semaforo è memorizzato nella
    variabile di tipo intero passata per indirizzo.
    L’indirizzo della variabile agisce da identificatore per il semaforo.
*/
void Verhogen(state_t *exceptionState)
{
  // Indirizzo del semaforo
  int *semaddr = (int *)exceptionState->reg_a1;

  pcb_t *un = NULL;
  int v_rc = verhogen(semaddr, currentProcess, &un);
  exceptionState->pc_epc += WORDLEN;

  if (v_rc == 0)
  {
    currentProcess->p_s = *exceptionState;
    currentProcess = NULL;
    setTimeAndSchedule(currentProcess);
  }
  else
  {
    regToCurrentProcess(currentProcess, currentProcess, exceptionState);
  }
}

/*
SYSCALL -5: DO_IO
int SYSCALL(DOIO, int *cmdAddr, int cmdValue, 0)
– Effettua un’operazione di I/O scrivendo il comando cmdValue nel registro cmdAddr, e mette in pausa il
    processo chiamante fino a quando non si e’ conclusa.
– L’operazione è bloccante, quindi il chiamante viene sospeso sino alla conclusione del comando. Il valore
    ritornato deve essere il contenuto del registro di status del dispositivo.

*/

void Do_IO_Device(state_t *exceptionState)
{
  currentProcess->p_s = *exceptionState;
  currentProcess->p_s.pc_epc += WORDLEN;
  int addr = exceptionState->reg_a1;  // commandAddr
  int value = exceptionState->reg_a2; // commandValue

  memaddr *ptr = (memaddr *)addr;
  *ptr = value;

  int *semAddr = findDeviceSemKey(addr);
  pcb_t *un = NULL;
  int p_rc = passeren(semAddr, currentProcess, &un);
  if (p_rc == 0)
  {
    currentProcess = NULL;
  }

  softBlockCounter++;

  setTimeAndSchedule(currentProcess);
}

/*
SYSCALL -6: Get_CPU_Time
int SYSCALL(GETTIME, 0, 0, 0)
– Quando invocata, la NSYS6 restituisce il tempo di esecuzione (in microsecondi) del processo che
    l’ha chiamata fino a quel momento.
– Questa System call implica la registrazione del tempo passato durante l’esecuzione di un
    processo.
*/
void Get_CPU_Time(state_t *exceptionState)
{
  exceptionState->reg_v0 = currentProcess->p_time;
  exceptionState->pc_epc += WORDLEN;
  regToCurrentProcess(currentProcess, currentProcess, exceptionState);
}

/*
SYSCALL -7: Wait_For_Clock
int SYSCALL(CLOCKWAIT, 0, 0, 0)
– Equivalente a una Passeren sul semaforo dell’Interval Timer.
– Blocca il processo invocante fino al prossimo tick del
    dispositivo.
*/
void Wait_For_Clock(state_t *exceptionState)
{
  pcb_t *unblocked = NULL;
  passeren(&semDevice[48], currentProcess, &unblocked);

  softBlockCounter++;

  exceptionState->pc_epc += WORDLEN;
  currentProcess->p_s = *exceptionState;

  setPtimeToExcTime(currentProcess);

  currentProcess = NULL;

  scheduler();
}

/*
SYSCALL -8: Get_Support_Data
support_t* SYSCALL(GETSUPPORTPTR, 0, 0, 0)
– Restituisce un puntatore alla struttura di supporto del processo corrente, ovvero il campo p_supportStruct del
    pcb_t.
*/
void Get_SUPPORT_Data(state_t *exceptionState)
{
  exceptionState->reg_v0 = (memaddr)currentProcess->p_supportStruct;
  exceptionState->pc_epc += WORDLEN;
  regToCurrentProcess(currentProcess, currentProcess, exceptionState);
}

/*
SYSCALL -9: Get_Process_Id
int SYSCALL(GETPROCESSID, int parent, 0, 0)
– Restituisce l’identificatore del processo invocante se parent == 0, 
    quello del genitore del processo invocante altrimenti.
*/
void Get_Process_ID(state_t *exceptionState)
{
  if (exceptionState->reg_a1 == 0)
  {
    exceptionState->reg_v0 = currentProcess->p_pid;
  }
  else
  {
    if (currentProcess->p_parent == 0)
    {
      exceptionState->reg_v0 = 0;
    }
    else
    {
      exceptionState->reg_v0 = currentProcess->p_parent->p_pid;
    }
  }
  exceptionState->pc_epc += WORDLEN;
  regToCurrentProcess(currentProcess, currentProcess, exceptionState);
}

/*
 SYSCALL -10: Yield
int SYSCALL(YIELD, 0, 0, 0)
– Un processo che invoca questa system call viene sospeso e messo in fondo alla coda corrispondente dei processi ready.
– Il processo che si e’ autosospeso, anche se rimane “ready”, non puo’ ripartire immediatamente
*/
void Yield(state_t *exceptionState)
{
  exceptionState->pc_epc += WORDLEN;
  currentProcess->p_s = *exceptionState;

  setPcbToProperQueue(currentProcess);

  setPtimeToExcTime(currentProcess);

  currentProcess = NULL;

  scheduler();
}