#include "pcb.h"

pcb_t pcbFree_table[MAXPROC];

struct list_head pcbFree_h = LIST_HEAD_INIT(pcbFree_h);

void initPcbs(){
    for(int i = 0; i < MAXPROC; i++)
        list_add_tail(&pcbFree_table[i],&pcbFree_h);
}

void freePcb(pcb_t * p){
    list_add_tail(&p,&pcbFree_h);
}

pcb_t *allocPcb(){
    pcb_t *rem;
    
    //controllo se la lista è vuota
    if(list_empty(&pcbFree_h))
        return NULL;

    //rimuovo un elemento da pcbFree_h
    list_del(&pcbFree_h.next);

    //setto tutto a NULL/0
    rem->p_parent = NULL;
    INIT_LIST_HEAD(&rem->p_child);
    INIT_LIST_HEAD(&rem->p_list);
    INIT_LIST_HEAD(&rem->p_sib);
    rem->p_s.cause = 0;
    rem->p_s.entry_hi = 0;
    for(int i = 0; i < STATE_GPR_LEN; i++)
        rem->p_s.gpr[i] = 0;
    rem->p_s.hi = 0;
    rem->p_s.lo = 0;
    rem->p_s.pc_epc = 0;
    rem->p_s.status = 0;
    rem->p_time = 0;
    rem->p_semAdd = 0;

    return rem;

}

void mkEmptyProcQ(struct list_head *head){
    INIT_LIST_HEAD(head);
}

int emptyProcQ(struct list_head *head){
    //WARNING: se da errore aggiunger &
    if(list_empty(head))
        return TRUE;
    else 
        return FALSE;
}

void insertProcQ(struct list_head* head, pcb_t* p){
    list_add_tail(head,p);
}

pcb_t *headProcQ(struct list_head* head){
    if(emptyProcQ(head))
        return NULL;
    return container_of(head,pcb_t,p_list.next);
}

pcb_t* removeProcQ(struct list_head* head){
    pcb_t *p = NULL;

    if(emptyProcQ(head))
        return NULL;
    list_del(container_of(head,pcb_t,p_list.next));
    p = container_of(head,pcb_t,p_list.next);
    return p;
}

pcb_t* outProcQ(struct list_head* head, pcb_t *p){
    if(!emptyProcQ(head)){
        struct list_head *it;

        list_for_each(it,head){
            pcb_t *curr = container_of(it,pcb_t,p_list);
            if(curr == it){
                list_del(it);
            }
        }
    }
    
}

int emptyChild(pcb_t *p){
    if(list_empty(&p->p_child))
        return TRUE;
    return FALSE;
}

void insertChild(pcb_t *prnt,pcb_t *p){
    INIT_LIST_HEAD(&p->p_sib);
    list_add_tail(&p->p_sib,&prnt->p_child);
    p->p_parent = prnt;
}

pcb_t* removeChild(pcb_t *p){
    if(emptyChild(p))
        return NULL;
    //rimozione primo nodo
    pcb_t *rem = container_of(p->p_child.next,pcb_t,p_sib);
    list_del(&rem->p_sib);
}

pcb_t *outChild(pcb_t* p){
    if(p->p_parent !=NULL){
        list_del(&p->p_sib);
        return p;
    }
    return NULL;
}