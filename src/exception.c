#include "exception.h"

extern int exceptionCode;

state_t *exState;
extern cpu_t time;

void exceptionHandler(){

    state_t *exceptionState = (state_t*)BIOSDATAPAGE;
    exceptionCode = exceptionState->cause;

    if(exceptionCode == 0){
        interruptHandler(exceptionCode);
    }
    else if (exceptionCode >= 1 && exceptionCode <=3) {
        passUpOrDie(PGFAULTEXCEPT,exceptionState);
    }
    else if (exceptionCode >= 4 && exceptionCode <= 7 && exceptionCode >= 9 && exceptionCode <= 12) {
        passUpOrDie(GENERALEXCEPT,exceptionState);
    }
    else{
        syscallExceptionHandler(exState);
    }
}

void passUpOrDie(int exceptionIndex, state_t *exceptionState){
    if( currentProcess->p_supportStruct == NULL){
            Terminate_Process();
        }
    else{
        (currentProcess->p_supportStruct)->sup_exceptState[exceptionIndex] = *exceptionState;
        context_t context = (currentProcess->p_supportStruct)->sup_exceptContext[exceptionIndex];
        LDCXT(context.stackPtr,context.status,context.pc);
    }
}

void syscallExceptionHandler(state_t *excstate){
    int syscall = excstate->reg_a0;


    switch (syscall)
    {
    case CREATEPROCESS:
        Create_Process();
        break;
    case TERMPROCESS:
        Terminate_Process();
        break;
    case PASSEREN:
        Passeren(excstate->reg_a1);
        break;
    case VERHOGEN:
        Verhogen(excstate->reg_a1);
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
        Get_Process_ID(excstate->reg_a1);
        break;
    case YIELD:
        Yield();
        break;
    default:
        passUpOrDie(GENERALEXCEPT,excstate);
        break;
    }
}

void Create_Process(){

    //Alloco un nuovo processo
    pcb_t *proc = allocPcb();

    if(proc == NULL){
        exState->reg_v0 =-1;
    }
    else{
        proc->p_s = *(state_t*)exState->reg_a1;
        proc->p_prio = exState->reg_a2;
        proc->p_supportStruct = exState->reg_a3;
        //TODO: generare valore pid
        insertProcQ(&(readyQueue),proc);
        insertChild(currentProcess,proc);
        proc->p_time = 0;
        proc->p_semAdd = NULL;
    }
}

//AKA Batman
void Terminate_Process(){
    outChild(currentProcess);

    if(currentProcess == NULL){
        return;
    }
    if(!emptyChild(currentProcess)){
        removeChild(currentProcess);
    }
    
    if(currentProcess->p_semAdd < 0){
        currentProcess->p_semAdd ++;
    }
    else{
        processCount -=1;
        softBlockCounter -=1;
    }
    
    outBlocked(currentProcess);
    freePcb(currentProcess);
    currentProcess = NULL;
    scheduler();
}

void Passeren(int *semaddr){
    semaddr--;
    if(semaddr < 0){
        insertBlocked(semaddr,currentProcess);
        scheduler();
    }
}

void Verhogen(int *semaddr){
    semaddr++;
    pcb_t *proc = removeBlocked(semaddr);
    if(proc != NULL){
        insertProcQ(&readyQueue,proc);
    }
}

void Do_IO_Device(){
    int addr = exState->reg_a1;     //commandAddr
    int value = exState->reg_a2;    //commandValue
    int term = exState->reg_a3;
    //Se il device da usare è il terminale
    if(addr == 7){
        value = (value * 2) + term;
    }
    int semIndex = (addr - 3) * 8 + value;
    int sem = semDevice[semIndex];
    sem--;
    insertBlocked(&sem,currentProcess);
    softBlockCounter++;
    currentProcess->p_s= *exState;
    scheduler();
}

void Get_CPU_Time(){
    currentProcess->p_time += (TODLOADDR - time);
    exState->reg_v0 = currentProcess->p_time;
}

void Wait_For_Clock(){
    softBlockCounter++;
    Passeren(&semDevice[DEVICECNT]);
}

void Get_SUPPORTA_Data(){
    exState->reg_v0 = currentProcess->p_supportStruct;
}

void Get_Process_ID(int parent){
    if(parent == 0){
        exState->reg_v0= currentProcess->p_pid;
    }
    else {
        currentProcess->p_s.reg_v0 = currentProcess->p_parent->p_pid;
    }
}

void Yield(){
    processCount --;
    insertProcQ(&readyQueue,currentProcess);
}