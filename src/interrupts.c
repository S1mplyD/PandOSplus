#include "interrupts.h"

extern int softBlockCounter;
extern int semDevice[49];  
extern pcb_t *currentProcess;
extern struct list_head HI_readyQueue;  
extern struct list_head LO_readyQueue;

void interruptHandler(int exceptionCode){
    int line = 2;
    for(int i = 0 ; i < 8 ; i++){
        if(exceptionCode & line){
            interrupt(i);
        }
        line *= 2;
    }
}

void interrupt(int lineNumber){
    //Non timer interrupts
    if(lineNumber > 2) {
        //Device
        memaddr device = CDEV_BITMAP_BASE + ((lineNumber - 3) * 0x4);
        int dev = 1;
        for (int deviceNumber = 0; deviceNumber < DEVPERINT; deviceNumber ++){
            if(device & dev){
                unsigned int returnStatus;
                //Salvo il codice di stato dal registro di device del device
                devreg_t *devAddrBase = (devreg_t *)(DEV_REG_START + 
                    ((lineNumber - 3) * 0x80) + (deviceNumber * 0x10));
                if(lineNumber < 7){
                    //Acknowledge dell'interrupt
                    returnStatus = devAddrBase->dtp.status;
                    devAddrBase->dtp.command = ACK;
                    
                }
                else{
                    //Terminale
                    termreg_t *termreg = (termreg_t*)devAddrBase;
                    if(termreg->recv_status != READY){
                        returnStatus = termreg->recv_status;
                        termreg->recv_command = ACK;
                    }
                    else{
                        returnStatus = termreg->transm_status;
                        termreg->transm_command = ACK;
                    }
                    deviceNumber *= 2;
                }
            //Faccio una operazione V
                int semIndex = (lineNumber - 3) * 8 + deviceNumber;
                semIndex++;
                pcb_t *proc = removeBlocked((int *)semIndex);
                if(proc !=NULL){
                    proc->p_s.reg_v0 = returnStatus;
                    proc->p_semAdd = NULL;
                    softBlockCounter--;
                    if(proc->p_prio == 0){
                        insertProcQ(&LO_readyQueue,proc);
                    }
                    else {
                        insertProcQ(&HI_readyQueue,proc);
                    }
                }    
                
            }
            dev *=2;
        }
        if(currentProcess == NULL){
            scheduler();
        }
        else{
            LDST((state_t*)BIOSDATAPAGE);
        }
    }
    //PLT Interrupts
    else if (lineNumber == 1){
        //Carico un nuovo valore nel timer
        setTIMER(0xFFFFFF);
        //Copio lo stato del processore nel processo corrente
        currentProcess->p_s = *(state_t*)BIOSDATAPAGE;
        //Metto il processo corrente nella readyQueue
        if(currentProcess->p_prio == 0){
                        insertProcQ(&LO_readyQueue,currentProcess);
                    }
                    else {
                        insertProcQ(&HI_readyQueue,currentProcess);

                    }
        scheduler();
    }
    //Timer interrupt
    else{
        //Carico 100ms nel timer
        LDIT(PSECOND);
        while(headBlocked((int *)semDevice[49 - 1]) != NULL){
            pcb_t *proc = removeBlocked((int *)semDevice[49 - 1]);

            if(proc != NULL){
                proc->p_semAdd = NULL;
                if(proc->p_prio == 0){
                        insertProcQ(&LO_readyQueue,proc);
                    }
                    else {
                        insertProcQ(&HI_readyQueue,proc);

                    }
                softBlockCounter --;

            }
        }
        semDevice[DEVICECNT - 1] = 0;
        if(currentProcess == NULL){
            scheduler();
        }
        else{
            LDST((state_t*)BIOSDATAPAGE);
        }
        
    }
}