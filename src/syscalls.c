#include <umps3/umps/cp0.h>
#include <umps3/umps/libumps.h>
#include <syscalls.h>
#include <utility.h>
#include <pcb.h>
#include <asl.h>
#include <exception.h>
#include <scheduler.h>

extern struct list_head semd_h;
extern pcb_t *currentProcess;
extern int semDevice[49];
extern int softBlockCounter;
extern struct list_head LO_readyQueue;
extern struct list_head HI_readyQueue;
extern int processCount;

void syscallExceptionHandler(state_t *exceptionState)
{
  int processor_mode = (exceptionState->status & STATUS_KUp) >> 3;
  int syscall = (int)exceptionState->reg_a0;
  if (processor_mode == 0 && syscall < 0)
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
      passUpOrDie(GENERALEXCEPT,exceptionState);
      break;
    }
  }
  else if (processor_mode == 1 && syscall < 0)
  {
    klog_print("processor_mode == 1 && syscall < 0//\n");
    //Processore in usermode
    exceptionState->cause = (exceptionState->cause & ~CAUSE_EXCCODE_MASK) | (EXC_RI << CAUSE_EXCCODE_BIT);
    passUpOrDie(GENERALEXCEPT, exceptionState);
  }
  else
  {
    klog_print("syscall >=0//\n");
    passUpOrDie(GENERALEXCEPT, exceptionState);
  }

}

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

void Get_CPU_Time(state_t *exceptionState)
{
  exceptionState->reg_v0 = currentProcess->p_time;
  exceptionState->pc_epc += WORDLEN;
  regToCurrentProcess(currentProcess, currentProcess, exceptionState);
}

void Wait_For_Clock(state_t *exceptionState)
{
  pcb_t *unblocked = NULL;                              
  passeren(&semDevice[48], currentProcess, &unblocked); 

  softBlockCounter++;

  exceptionState->pc_epc += WORDLEN;
  currentProcess->p_s = *exceptionState;

  setTimeNoSchedule(currentProcess);

  currentProcess = NULL;

  scheduler();
}

void Get_SUPPORT_Data(state_t *exceptionState)
{
  exceptionState->reg_v0 = (memaddr)currentProcess->p_supportStruct;
  exceptionState->pc_epc += WORDLEN;
  regToCurrentProcess(currentProcess, currentProcess, exceptionState);
}

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

void Yield(state_t *exceptionState)
{
  exceptionState->pc_epc += WORDLEN;
  currentProcess->p_s = *exceptionState;

  setPcbToProperQueue(currentProcess);

  setTimeNoSchedule(currentProcess);

  currentProcess = NULL;

  scheduler();
}