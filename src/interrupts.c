#include <umps3/umps/libumps.h>
#include <umps3/umps/arch.h>
#include <umps3/umps/cp0.h>

#include <interrupts.h>
#include <utility.h>
#include <exception.h>
#include <scheduler.h>
#include <asl.h>

extern pcb_t *currentProcess;
extern unsigned int ITtimeS;
extern int semDevice[49];
extern unsigned int cPStartT;
extern int softBlockCounter;

void interruptHandler(state_t *exceptionState)
{

  int line;
  for (line = 0; line < 8; line++)
  {
    if (exceptionState->cause & CAUSE_IP(line))
    {
      interrupt(line, exceptionState);
    }
  }
}
void interrupt(int lineNumber, state_t *exceptionState)
{
  if (lineNumber == 1)
  {
    // Processor Local Timer - PLT
    setTIMER(0xFFFFFFFF);
    // Copio lo stato del processore nel processo corrente
    if (currentProcess != NULL)
    {
      currentProcess->p_s = *exceptionState;
      // Metto il processo corrente nella readyQueue
      setPcbToProperQueue(currentProcess);
      setTimeNoSchedule(currentProcess);
      currentProcess = NULL;
    }

    scheduler();
  }
  else if (lineNumber == 2)
  {
    // Interval Timer - Global timer

    unsigned int time;
    STCK(time);
    unsigned int time_until_next_tick = ITtimeS - time;
    LDIT(time_until_next_tick);
    ITtimeS += PSECOND;

    int *semAddr = &semDevice[48];
    pcb_t *un = NULL;
    int rc;
    do
    {
      rc = verhogen(semAddr, NULL, &un);
    } while (rc == 1);

    semDevice[48] = 0;

    resumeIfTimeLeft(currentProcess, 0);
    STCK(cPStartT);
    LDST(exceptionState);
  }
  
  else if (lineNumber > 2)
  {
    // Device
    devregarea_t *deviceRegs = (devregarea_t *)RAMBASEADDR;
    unsigned int device_bitmap = deviceRegs->interrupt_dev[lineNumber - 3];
    int dev = 0x1;
    for (int deviceNumber = 0; deviceNumber < DEVPERINT; deviceNumber++)
    {
      if (device_bitmap & dev)
      {
        int *semAddr;
        unsigned int returnStatus;
        // Salvo il codice di stato dal registro di device del device
        memaddr devAddrBase = DEV_REG_ADDR(lineNumber, deviceNumber);
        // Non è terminale
        if (lineNumber != 7)
        {
          // Acknowledge dell'interrupt
          dtpreg_t *devStruct = (dtpreg_t *)devAddrBase;

          // save the status code
          returnStatus = devStruct->status;

          // ACK
          devStruct->command = ACK;

          // Recover the semaphore key
          semAddr = getDeviceSemAddr(lineNumber, deviceNumber);
        }
        else
        {
          int mode;
          // Terminale
          termreg_t *termreg = (termreg_t *)devAddrBase;
          if ((termreg->recv_status & 0xFF) != READY && (termreg->recv_status & 0xFF) != BUSY)
          {
            mode = 0;
            returnStatus = termreg->recv_status;
            termreg->recv_command = ACK;
          }
          else
          {
            mode = 1;
            returnStatus = termreg->transm_status;
            termreg->transm_command = ACK;
          }
          semAddr = getTerminalSemAddr(deviceNumber, mode);
        }
        pcb_t *un = NULL;
        verhogen(semAddr, NULL, &un);
        if (un != NULL)
        {
          un->p_s.reg_v0 = returnStatus;
        }
        softBlockCounter--;
        regToCurrentProcess(un, currentProcess, exceptionState);
      }
    }
  }
}
