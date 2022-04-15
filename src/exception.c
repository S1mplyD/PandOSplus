#include "exception.h"

unsigned int exceptionCode;
extern pcb_t *currentProcess;
extern int softBlockCounter;
extern int processCount;
extern struct list_head HI_readyQueue;
extern struct list_head LO_readyQueue;
extern int semDevice[49];
extern int pid;
state_t *exceptionState;
cpu_t time;

void *memcpy(void *dest, const void *src, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        ((char *)dest)[i] = ((char *)src)[i];
    }
    return 0;
}

void exceptionHandler()
{

    exceptionState = (state_t *)BIOSDATAPAGE;
    exceptionCode = CAUSE_GET_EXCCODE(exceptionState->cause);

    if (exceptionCode == 0)
    {
        interruptHandler((int)exceptionState);
    }
    else if (exceptionCode >= 1 && exceptionCode <= 3)
    {
        passUpOrDie(PGFAULTEXCEPT);
    }
    else if (exceptionCode >= 4 && exceptionCode <= 7 && exceptionCode >= 9 && exceptionCode <= 12)
    {
        passUpOrDie(GENERALEXCEPT);
    }
    else
    {
        syscallExceptionHandler();
    }
}

void syscallExceptionHandler()
{

    int syscall = (int)exceptionState->reg_a0;

    switch (syscall)
    {
    case CREATEPROCESS:
        Create_Process();
        break;
    case TERMPROCESS:
        Terminate_Process();
        break;
    case PASSEREN:
        Passeren();
        break;
    case VERHOGEN:
        Verhogen();
        break;
    case DOIO:
        Do_IO_Device();
        break;
    case GETTIME:
        Get_CPU_Time();
        break;
    case CLOCKWAIT:
        Wait_For_Clock();
        break;
    case GETSUPPORTPTR:
        Get_SUPPORTA_Data();
        break;
    case GETPROCESSID:
        Get_Process_ID();
        break;
    case YIELD:
        Yield();
        break;
    default:
        passUpOrDie(GENERALEXCEPT);
        break;
    }
}

void passUpOrDie(int exceptionIndex)
{
    if (currentProcess->p_supportStruct == NULL)
    {
        Terminate_Process();
        scheduler();
    }
    else
    {
        (currentProcess->p_supportStruct)->sup_exceptState[exceptionIndex] = *exceptionState;
        context_t context = (currentProcess->p_supportStruct)->sup_exceptContext[exceptionIndex];
        LDCXT(context.stackPtr, context.status, context.pc);
    }
}

void Create_Process()
{

    // Alloco un nuovo processo
    pcb_t *proc = allocPcb();

    if (proc == NULL)
    {
        exceptionState->reg_v0 = -1;
    }
    else
    {
        proc->p_s = *(state_t *)exceptionState->reg_a1;
        proc->p_prio = exceptionState->reg_a2;
        if (exceptionState->reg_a3 != NULL)
            proc->p_supportStruct = (support_t *)exceptionState->reg_a3;
        proc->p_pid = ++pid;
        if (proc->p_prio == 0)
        {
            insertProcQ(&LO_readyQueue, proc);
        }
        else
        {
            insertProcQ(&HI_readyQueue, proc);
        }

        insertChild(currentProcess, proc);
        proc->p_time = 0;
        proc->p_semAdd = NULL;
    }
    exceptionState->pc_epc = exceptionState->pc_epc + WORDLEN;
    LDST(exceptionState);
}

// AKA Batman
void Terminate_Process()
{

    outChild(currentProcess);

    if (currentProcess == NULL)
    {
        return;
    }
    if (!emptyChild(currentProcess))
    {
        removeChild(currentProcess);
    }

    if (currentProcess->p_semAdd < 0)
    {
        currentProcess->p_semAdd++;
    }
    else
    {
        processCount -= 1;
        softBlockCounter -= 1;
    }

    outBlocked(currentProcess);
    freePcb(currentProcess);
    currentProcess = NULL;
    scheduler();
}

void Passeren()
{
    int *semaddr = (int *)exceptionState->reg_a1;
    semaddr--;
    if (semaddr < 0)
    {
        insertBlocked(semaddr, currentProcess);
        scheduler();
    }
}

void Verhogen()
{
    int *semaddr = (int *)exceptionState->reg_a1;
    semaddr++;
    pcb_t *proc = removeBlocked(semaddr);
    if (proc != NULL)
    {
        if (proc->p_prio == 0)
        {
            insertProcQ(&LO_readyQueue, proc);
        }
        else
        {
            insertProcQ(&HI_readyQueue, proc);
        }
    }
    scheduler();
}

void Do_IO_Device()
{
    int addr = exceptionState->reg_a1;  // commandAddr
    int value = exceptionState->reg_a2; // commandValue
    int term = exceptionState->reg_a3;
    // Se il device da usare è il terminale
    if (addr == 7)
    {
        value = (value * 2) + term;
    }
    // Trovo il semaforo
    int semIndex = (addr - 3) * 8 + value;
    int sem = semDevice[semIndex];
    sem--;
    insertBlocked(&sem, currentProcess);
    softBlockCounter++;
    currentProcess->p_s = *exceptionState;
    scheduler();
}

void Get_CPU_Time()
{
    currentProcess->p_time += (TODLOADDR);
    exceptionState->reg_v0 = currentProcess->p_time;
}
// TODO passeren
void Wait_For_Clock()
{
    // passeren(semDevice[48]);
    softBlockCounter++;
}

void Get_SUPPORTA_Data()
{
    exceptionState->reg_v0 = (unsigned int)currentProcess->p_supportStruct;
}

void Get_Process_ID()
{
    int parent = exceptionState->reg_a1;
    if (parent == 0)
    {
        exceptionState->reg_v0 = currentProcess->p_pid;
    }
    else
    {
        currentProcess->p_s.reg_v0 = currentProcess->p_parent->p_pid;
    }
}

void Yield()
{
    processCount--;
    currentProcess->p_s = *exceptionState;
    if (currentProcess->p_prio == 0)
    {
        insertProcQ(&LO_readyQueue, currentProcess);
    }
    else
    {
        insertProcQ(&HI_readyQueue, currentProcess);
    }
}

// int passeren(int *sem_key, pcb_t * pcb, pcb_t **unblocked_pcb){

//   int return_code;

//   if (*sem_key == 0) {
//     insertBlocked(sem_key, pcb);
//     return_code = 0;
//   }
//   else if (headBlocked(sem_key) != NULL){
//     // *sem_key = 1 and there's a pcb blocked on the sem
//     *unblocked_pcb = returnPcbToQueue(removeBlocked(sem_key));
//     return_code = 1;
//     // TOCHECK should we call the scheduler here or let the current process carry on?
//   }
//   else {
//     (*sem_key)--;
//     return_code = 2;
//   }

//   return return_code;
// }

// // todo document this kludge
// int verhogen(int *sem_key, pcb_t *pcb, pcb_t **unblocked_pcb) {

//   int return_code;

//   if (*sem_key == 1) {
//     insertBlocked(sem_key, pcb);
//     return_code = 0;
//   }
//   else if (headBlocked(sem_key) != NULL){
//     // *sem_key = 0 and there's a pcb blocked on the sem
//     pcb_t *tmp = removeBlocked(sem_key);
//     *unblocked_pcb = returnPcbToQueue(tmp);
//     return_code = 1;
//     // TOCHECK should we call the scheduler here or let the current process carry on?
//   }
//   else {
//     (*sem_key)++;
//     return_code = 2;
//   }

//   return return_code;
// }
