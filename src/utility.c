#include "utility.h"

void killProcess(pcb_t *p)
{
    struct list_head *i;
    list_for_each(i, &p->p_child)
    {
        pcb_t *child = container_of(i, pcb_t, p_sib);
        killProcess(child);
    }

    // orphan this child
    outChild(p);

    if (currentProcess != NULL && currentProcess->p_pid == p->p_pid)
    {
        // It's the current process
        currentProcess = NULL;
    }
    else if (p->p_semAdd != NULL)
    {
        // it's blocked on a semaphores
        if (p->p_semAdd >= semDevice && p->p_semAdd <= semDevice + 49 * sizeof(int))
        {
            // if it's blocked on a system device semaphore, adjust soft_block_count
            softBlockCounter--;
        }

        // remove it from its semaphore. do not modify the semaphore value, the P/V will take care of this
        outBlocked(p);
    }
    else
    {
        // it's on a ready queue, remove it from the queue
        if (p->p_prio == 0)
        {
            outProcQ(&LO_readyQueue, p);
        }
        else
            outProcQ(&HI_readyQueue, p);
    }

    // return the pcb to the free pcb list
    freePcb(p);

    processCount--;
}

pcb_t *getPcb(int pid)
{
    pcb_t *p;
    if (currentProcess->p_pid == pid)
    {
        return currentProcess;
    }

    p = findPcb(&LO_readyQueue, pid);
    if (p != NULL)
        return p;
    p = findPcb(&HI_readyQueue, pid);
    if (p != NULL)
        return p;
    return 0;
}

pcb_t *findPcb(struct list_head *queue, int pid)
{
    struct list_head *i;
    list_for_each(i, &LO_readyQueue)
    {
        pcb_t *res = container_of(i, pcb_t, p_list);
        if (res->p_pid == pid)
            return res;
    }

    // Controllo se è nella readyQueue ad alta priorità
    list_for_each(i, &HI_readyQueue)
    {
        pcb_t *res = container_of(i, pcb_t, p_list);
        if (res->p_pid == pid)
            return res;
    }
    return 0;
}

pcb_t *setPcbToProperQueue(pcb_t *pcb)
{
    if(pcb != NULL){
        if(pcb->p_prio == 0){
            insertProcQ(&LO_readyQueue,pcb);
        }
        else {
            insertProcQ(&HI_readyQueue,pcb);
        }
    }
    return pcb;
}