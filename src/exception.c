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
unsigned int timestamp_exception_start;
unsigned int remaining_time_slice;
unsigned int timestamp_process_job_start;
unsigned int logger_time;

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

    STCK(timestamp_exception_start);
    if (currentProcess != NULL)
    {
        remaining_time_slice = getTIMER();
        if ((int)remaining_time_slice > 0)
        {
            setTIMER(0xFFFFFFFF);
        }
        currentProcess->p_time += timestamp_exception_start - timestamp_process_job_start;
    }
    logger_time = 0;

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
    exceptionState->pc_epc += WORDLEN;
    if (p_rc == 0)
    {
        currentProcess->p_s = *exceptionState;
        currentProcess = NULL;
        exception_end_bill_before_schedule(currentProcess);
    }
    else
    {
        exception_end_bill_and_continue(currentProcess, currentProcess, exceptionState);
    }
}

void Verhogen()
{
    // Indirizzo del semaforo
    int *semaddr = (int *)exceptionState->reg_a1;

    pcb_t *un;
    int v_rc = verhogen(semaddr, currentProcess, &un);
    exceptionState->pc_epc += WORDLEN;

    if (v_rc == 0)
    {
        currentProcess->p_s = *exceptionState;
        currentProcess = NULL;
        exception_end_bill_and_schedule(currentProcess);
    }
    else
    {
        exception_end_bill_and_continue(currentProcess, currentProcess, exceptionState);
    }
}

void Do_IO_Device()
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

    exception_end_bill_and_schedule(currentProcess);
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
        *unblocked_pcb = setPcbToProperQueue(removeBlocked(sem_key));
        return_code = 1;
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
        pcb_t *p = removeBlocked(sem_key);
        *unblocked_pcb = setPcbToProperQueue(p);
        return_code = 1;
    }
    else
    {
        (*sem_key)++;
        return_code = 2;
    }

    return return_code;
}
unsigned int calculate_exception_time()
{
    unsigned int time;
    STCK(time);
    return time - timestamp_exception_start - logger_time;
}

void bill_exception_time_to_process(pcb_t *pcb, unsigned int exception_time)
{
    // bill the process the exception time
    pcb->p_time += exception_time;
}

void restart_timeslice(unsigned int exception_time)
{
    unsigned int timeslice = remaining_time_slice - exception_time;
    if ((int)timeslice > 0)
    {
        // there's still time in this timeslice
        setTIMER(timeslice);
    }
    else
    {
        // timeslice ended with this systemcall, make it trigger
        setTIMER(0);
    }
}

void exception_end_bill_before_continue(pcb_t *time_bill_pcb, pcb_t *time_slice_pcb)
{
    unsigned int exception_time = calculate_exception_time();
    if (time_bill_pcb != NULL)
    {
        bill_exception_time_to_process(time_bill_pcb, exception_time);
    }
    if (time_slice_pcb != NULL)
    {
        if (time_bill_pcb->p_prio == 0 && remaining_time_slice > 0U)
        {
            restart_timeslice(exception_time);
        }
    }
    STCK(timestamp_process_job_start);
}

void exception_end_bill_and_continue(pcb_t *time_bill_pcb, pcb_t *time_slice_pcb, state_t *saved_state)
{
    exception_end_bill_before_continue(time_bill_pcb, time_slice_pcb);
    LDST(saved_state);
}

void exception_end_bill_before_schedule(pcb_t *time_bill_target)
{
    if (time_bill_target != NULL)
    {
        unsigned int exception_time = calculate_exception_time();
        bill_exception_time_to_process(time_bill_target, exception_time);
    }
}

void exception_end_bill_and_schedule(pcb_t *time_bill_target)
{
    exception_end_bill_before_schedule(time_bill_target);
    scheduler();
}

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
                return _findDeviceSemKey(IntlineNo, DevNo);
            }
        }
    }

    int IntlineNo = 7;
    int baseTermSem = 32;
    int foundMode = -1;
    int DevNo = 0;
    ;

    for (; DevNo < 8; DevNo++)
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

    return _findTerminalSemKey(DevNo, foundMode);
}

int *_findDeviceSemKey(int line, int device)
{
    int key_index = (line - 3) * 8 + device;
    return &semDevice[key_index];
}

int *_findTerminalSemKey(int device, int mode)
{
    int key_index = (7 - 3) * 8 + device + mode;
    return &semDevice[key_index];
}
