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
        if ((void *)exceptionState->reg_a3 != NULL)
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
    int pidToKill = exceptionState->reg_a1;

    if (pidToKill == 0)
    {
        killProcess(currentProcess);
    }
    else
    {
        pcb_t *toKill = getPcb(pidToKill);
        killProcess(toKill);
    }

    scheduler();
}

void Passeren()
{
    int *semaddr = (int *)exceptionState->reg_a1;

    pcb_t *un = NULL;

    int p_rc = passeren(semaddr, currentProcess, &un);
    if (p_rc == 0)
    {
        currentProcess->p_s = *exceptionState;
        currentProcess = NULL;
        insertBlocked(semaddr, currentProcess);
    }
}

void Verhogen()
{
    int *semaddr = (int *)exceptionState->reg_a1;

    pcb_t *un;
    int v_rc = verhogen(semaddr, currentProcess, &un);
    if (v_rc == 0)
    {
        currentProcess->p_s = *exceptionState;
        currentProcess = NULL;
        insertBlocked(semaddr, currentProcess);
    }
}

void Do_IO_Device()
{
    int addr = exceptionState->reg_a1;  // commandAddr
    int value = exceptionState->reg_a2; // commandValue

    currentProcess->p_s = *exceptionState;
    currentProcess->p_s.pc_epc += WORDLEN;
    int *semAddr;
    int FLAG = FALSE;
    memaddr devAddrBase;
    for (int IntlineNo = 3; IntlineNo <= 6; IntlineNo++)
    {
        for (int DevNo = 0; DevNo < 8; DevNo++)
        {
            devAddrBase = DEV_REG_ADDR(IntlineNo, DevNo);
            if ((memaddr)addr == devAddrBase + 0x4)
            {
                int index = (IntlineNo - 3) * 8 + DevNo;
                semAddr = &semDevice[index];
                FLAG = TRUE;
            }
        }
    }
    if (FLAG != TRUE)
    {
        int IntlineNo = 7;
        int baseTermSem = 32;
        int foundMode = -1;
        int DevNo = 0;

        for (; DevNo < 8; DevNo++)
        {

            devAddrBase = DEV_REG_ADDR(IntlineNo, DevNo);
            if ((memaddr)addr == devAddrBase + 0x4)
            {
                // recv
                foundMode = 0;
                break;
            }
            else if ((memaddr)addr == devAddrBase + 0xc)
            {
                // transm
                foundMode = 1;
                break;
            }
            baseTermSem = baseTermSem + 2;
        }

        int index = (7 - 3) * 8 + DevNo + foundMode;
        semAddr = &semDevice[index];
    }

    pcb_t *un = NULL;
    int p_rc = passeren(semAddr, currentProcess, &un);
    if (p_rc == 0)
    {
        currentProcess = NULL;
    }
    softBlockCounter++;
    scheduler();
}

void Get_CPU_Time()
{
    exceptionState->reg_v0 = currentProcess->p_time;
    exceptionState->pc_epc += WORDLEN;
}
// TODO passeren
void Wait_For_Clock()
{
    pcb_t *un = NULL;
    passeren(&semDevice[48], currentProcess, &un);
    softBlockCounter++;
    exceptionState->pc_epc += WORDLEN;
    currentProcess->p_s = *exceptionState;
    LDST(&currentProcess->p_s);
    scheduler();
}

void Get_SUPPORTA_Data()
{
    exceptionState->reg_v0 = (memaddr)currentProcess->p_supportStruct;
    exceptionState->pc_epc += WORDLEN;
    LDST(&currentProcess->p_s);
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

int passeren(int *sem_key, pcb_t *pcb, pcb_t **unblocked_pcb)
{

    int return_code;

    if (*sem_key == 0)
    {
        insertBlocked(sem_key, pcb);
        return_code = 0;
    }
    else if (headBlocked(sem_key) != NULL)
    {
        // *sem_key = 1 and there's a pcb blocked on the sem
        pcb_t *p = removeBlocked(sem_key);
        if (p->p_prio == 0)
        {
            insertProcQ(&LO_readyQueue, p);
        }
        else
        {
            insertProcQ(&HI_readyQueue, p);
        }
        unblocked_pcb = p;
        if (unblocked_pcb)
            return_code = 1;
        // TOCHECK should we call the scheduler here or let the current process carry on?
    }
    else
    {
        (*sem_key)--;
        return_code = 2;
    }

    return return_code;
}

// todo document this kludge
int verhogen(int *sem_key, pcb_t *pcb, pcb_t **unblocked_pcb)
{

    int return_code;

    if (*sem_key == 1)
    {
        insertBlocked(sem_key, pcb);
        return_code = 0;
    }
    else if (headBlocked(sem_key) != NULL)
    {
        // *sem_key = 0 and there's a pcb blocked on the sem
        pcb_t *tmp = removeBlocked(sem_key);
        if (tmp->p_prio == 0)
        {
            insertProcQ(&LO_readyQueue, tmp);
        }
        else
        {
            insertProcQ(&HI_readyQueue, tmp);
        }
        *unblocked_pcb = tmp;
        return_code = 1;
        // TOCHECK should we call the scheduler here or let the current process carry on?
    }
    else
    {
        (*sem_key)++;
        return_code = 2;
    }

    return return_code;
}
