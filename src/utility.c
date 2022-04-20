#include "utility.h"

// ToD da quando è startato il currentProcess
unsigned int cPStartT;
// ToD dallo start dell'eccezione
unsigned int excTimeStart;
// Tempo del prossimo IT
unsigned int ITtimeS;
// Quanto rimane da vivere al PLT
unsigned int PLTTL;
// Log del kernel
unsigned int logT;

extern struct list_head semd_h;
extern pcb_t *currentProcess;
extern int semDevice[49];
extern int softBlockCounter;
extern struct list_head LO_readyQueue;
extern struct list_head HI_readyQueue;
extern int processCount;

void killProcess(pcb_t *pcb) {

  struct list_head *it;
  list_for_each(it, &pcb->p_child)
  {
    pcb_t *child = container_of(it, pcb_t, p_sib);
    killProcess(child);
  }

  outChild(pcb);

  // Se il pid è quello del processo corrente, lo elimino
  if (currentProcess != NULL && currentProcess->p_pid == pcb->p_pid)
  {

    currentProcess = NULL;
  }
  // Controllo se il pcb è bloccato su un semaforo
  else if (pcb->p_semAdd != 0)
  {

    if (pcb->p_semAdd >= semDevice && pcb->p_semAdd <= semDevice + 49 * sizeof(int))
    {
      // Il processo è bloccato su un dispositivo
      softBlockCounter--;
    }

    outBlocked(pcb);
  }
  else
  {
    if (pcb->p_prio == 0)
    {
      outProcQ(&LO_readyQueue, pcb);
    }
    else
      outProcQ(&HI_readyQueue, pcb);
  }

  freePcb(pcb);

  processCount--;
}
/*
Funzione che ritorna un processo con un determinato id
*/

pcb_t *getPcb(int pid) {
  pcb_t * result = NULL;

  // check the currentProcess
  if (currentProcess->p_pid == pid) {
    return currentProcess;
  }

  // check the LO_readyQueue
  result =  findPcb(&LO_readyQueue, pid);
  if (result != NULL)
    return result;

  // check the HI_readyQueue
  result =  findPcb(&HI_readyQueue, pid);
  if (result != NULL)
    return result;

  // check every active semaphore
  struct list_head* iter_sem;
  list_for_each(iter_sem,&semd_h) {
    semd_t * sem = container_of(iter_sem, semd_t, s_link);
    // check its blocked process queue
    result =  findPcb(&sem->s_procq, pid);
    if (result != NULL)
      return result;
  }

  return result;
}

/*
Funzione che controlla tutti i pcb della coda finché non ne trova uno con il
pid cercato
*/
pcb_t * findPcb(struct list_head *list, int pid) {
  struct list_head* iter;
  list_for_each(iter,list) {
    pcb_t *pcb = container_of(iter, pcb_t, p_list);
    if (pcb->p_pid == pid) {
      return pcb;
    }
  }
  return NULL;
}

/*
Funzione che mette un pcb nella sua apposita coda in base alla sua priorità
 */
void setPcbToProperQueue(pcb_t * pcb) {
  if (pcb != NULL) {
    if (pcb->p_prio == 0) {
      insertProcQ(&LO_readyQueue, pcb);
    }
    else {
      insertProcQ(&HI_readyQueue, pcb);
    }
  }
}

// La seguente funziona alloca n caratteri dall'area di memoria src all'area di memoria dest
void memcpy(void* to, void* from, size_tt n){
  // since we want to copy byte by byte, treat both pointers as char*
  char *cfrom = (char *)from;
  char *cto = (char *)to;
  // copy byte by byte
  for (int i=0; i<n; i++)
    cto[i] = cfrom[i];
}

/*
Funzione da richiamare alla fine di un handling di una eccezione prima di ridare il controllo al currentProcess.
Salva il tempo utilizzato dall'handling dell'eccezione in regTime
*/
void regExcTime(pcb_t *regTime, pcb_t *timeSlice)
{
  unsigned int excTime = getExcTime();
  if (regTime != NULL)
  {
    regTime->p_time += excTime;
  }
  resumeIfTimeLeft(timeSlice, excTime);
  STCK(cPStartT);
}

/*
Funzione da richiamare alla fine di un handling di una eccezione e ritorna il controllo al currentProcess.
Salva il tempo utilizzato dall'handling dell'eccezione in regTime
*/
void regToCurrentProcess(pcb_t *regTime, pcb_t *timeSlice, state_t *exceptionState)
{
  regExcTime(regTime, timeSlice);
  LDST(exceptionState);
}

/*
Funzione da chiamare alla fine di un handling di una eccezione prima di ridare il controllo allo scheduler.
*/
void setTimeNoSchedule(pcb_t *target)
{
  if (target != NULL)
  {
    target->p_time += getExcTime();
  }
}

/*
Funzione da chiamare alla fine di un handling di una eccezione per poi ridare il controllo allo scheduler.
*/
void setTimeAndSchedule(pcb_t *target)
{
  setTimeNoSchedule(target);
  scheduler();
}

unsigned int getExcTime()
{
  unsigned int time;
  STCK(time);
  return time - excTimeStart - logT;
}

void resumeIfTimeLeft(pcb_t *timeSlice, unsigned int excTime)
{
  if (timeSlice != NULL && timeSlice->p_prio == 0 && PLTTL > 0U)
  {
    //Tempo rimanente
    unsigned int timeLeft = PLTTL - excTime;
    if ((int)timeLeft > 0)
    {
      setTIMER(timeLeft);
    }
    else
    {
      setTIMER(0);
    }
  }
}

/*
Trova il semaforo nell'array dei semafori del device utilizzando il commandAddr
*/
int *findDeviceSemKey(memaddr command_addr) {
  memaddr devAddrBase;
  for(int IntlineNo = 3; IntlineNo <= 6; IntlineNo++) {
    for(int DevNo = 0; DevNo < 8; DevNo++) {
      devAddrBase = DEV_REG_ADDR(IntlineNo, DevNo);
      if ((memaddr) command_addr == devAddrBase + 0x4 ) {
        return getDeviceSemAddr(IntlineNo, DevNo);
      }
    }
  }

  int IntlineNo = 7;
  int baseTermSem = 32;
  int foundMode = -1;
  int DevNo = 0;;

  for(; DevNo < 8; DevNo++) {

    devAddrBase = DEV_REG_ADDR(IntlineNo, DevNo);
    if ((memaddr) command_addr == devAddrBase + 0x4) {
      // recv
      foundMode = 0;
      break;
    }
    else if ((memaddr) command_addr == devAddrBase + 0xc) {
      // transm
      foundMode = 1;
      break;
    }
    baseTermSem = baseTermSem + 2;
  }

  return getTerminalSemAddr(DevNo, foundMode);
}

/*
Ritorna la chiave del semaforo associato ad un dispositivo
*/
int *getDeviceSemAddr(int line, int device) {
  int key_index = (line - 3) * 8 + device;
  return &semDevice[key_index];
}

/*
Ritorna la chiave del semaforo associato al terminale
*/
int *getTerminalSemAddr(int device, int mode) {
  int key_index = (7 - 3) * 8 + device + mode;
  return &semDevice[key_index];
}