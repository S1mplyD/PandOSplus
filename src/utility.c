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
Funzione che setta il tempo di un processo a quello della durata di una eccezione*/
void setPtimeToExcTime(pcb_t *target)
{
  if (target != NULL)
  {
    target->p_time += getExcTime();
  }
}

/*
Funzione da chiamare alla fine di un handling di una eccezione prima di ridare il controllo allo scheduler.
*/
void setTimeAndSchedule(pcb_t *target)
{
  setPtimeToExcTime(target);
  scheduler();
}

/*
Funzione che ritorna la durata di una eccezione
*/
unsigned int getExcTime()
{
  unsigned int time;
  STCK(time);
  return time - excTimeStart - logT;
}

/*
Funzione che reimposta il PLT se è rimasto del tempo dopo una eccezione*/
void resumeIfTimeLeft(pcb_t *timeSlice, unsigned int excTime)
{
  if (timeSlice != NULL && timeSlice->p_prio == 0 && PLTTL > 0U)
  {
    unsigned int timeslice = PLTTL - excTime;
    if ((int)timeslice > 0)
    {
      setTIMER(timeslice);
    }
    else
    {
      setTIMER(0);
    }
  }
}

/*
Funzione che uccide un processo e tutta la sua progenie
*/
void killProcess(pcb_t *pcb)
{

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
    setPcbToProperQueue(pcb);
  }

  freePcb(pcb);

  processCount--;
}
/*
Funzione che mette un pcb nella sua apposita coda in base alla sua priorità
 */
void setPcbToProperQueue(pcb_t *pcb)
{
  if (pcb != NULL)
  {
    if (pcb->p_prio == 0)
    {
      insertProcQ(&LO_readyQueue, pcb);
    }
    else
    {
      insertProcQ(&HI_readyQueue, pcb);
    }
  }
}

/*
Funzione che controlla tutti i pcb della coda finché non ne trova uno con il
pid cercato
*/
pcb_t *findPcb(struct list_head *list, int pid)
{
  struct list_head *iter;
  list_for_each(iter, list)
  {
    pcb_t *pcb = container_of(iter, pcb_t, p_list);
    if (pcb->p_pid == pid)
    {
      return pcb;
    }
  }
  return NULL;
}

/*
Funzione che ritorna un processo con un determinato id
*/
pcb_t *getPcb(int pid)
{
  pcb_t *result = NULL;

  if (currentProcess->p_pid == pid)
  {
    return currentProcess;
  }

  result = findPcb(&LO_readyQueue, pid);
  if (result != NULL)
    return result;

  result = findPcb(&HI_readyQueue, pid);
  if (result != NULL)
    return result;

  struct list_head *iter_sem;
  list_for_each(iter_sem, &semd_h)
  {
    semd_t *sem = container_of(iter_sem, semd_t, s_link);
    result = findPcb(&sem->s_procq, pid);
    if (result != NULL)
      return result;
  }

  return result;
}

/*
Trova il semaforo nell'array dei semafori del device utilizzando il commandAddr
*/
int *findDeviceSemKey(memaddr command_addr)
{
  memaddr devAddrBase;
  for (int IntlineNo = 3; IntlineNo <= 6; IntlineNo++)
  {
    for (int DevNo = 0; DevNo < 8; DevNo++)
    {
      devAddrBase = DEV_REG_ADDR(IntlineNo, DevNo);
      if ((memaddr)command_addr == devAddrBase + 0x4)
      {
        return getDeviceSemAddr(IntlineNo, DevNo);
      }
    }
  }

  int IntlineNo = 7;
  int baseTermSem = 32;
  int foundMode;
  int DevNo = 0;

  for (DevNo = 0; DevNo < 8; DevNo++)
  {

    devAddrBase = DEV_REG_ADDR(IntlineNo, DevNo);
    if ((memaddr)command_addr == devAddrBase + 0x4)
    {
      // recv
      foundMode = 0;
      break;
    }
    else if ((memaddr)command_addr == devAddrBase + 0xc)
    {
      // transm
      foundMode = 1;
      break;
    }
    baseTermSem = baseTermSem + 2;
  }

  return getTerminalSemAddr(DevNo, foundMode);
}

int *getDeviceSemAddr(int line, int device)
{
  int key_index = (line - 3) * 8 + device;
  return &semDevice[key_index];
}

int *getTerminalSemAddr(int device, int mode)
{
  int key_index = (7 - 3) * 8 + device + mode;
  return &semDevice[key_index];
}

// La seguente funziona alloca n caratteri dall'area di memoria src all'area di memoria dest
void memcpy(void *dest, void *src, size_t n)
{
  // Typecast src and dest addresses to (char *)
  char *csrc = (char *)src;
  char *cdest = (char *)dest;

  // Copy contents of src[] to dest[]
  for (int i = 0; i < n; i++)
    cdest[i] = csrc[i];
}