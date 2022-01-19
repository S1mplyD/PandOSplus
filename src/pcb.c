#include "pcb.h"

pcb_t pcbFree_table[MAXPROC];

struct list_head pcbFree_h = LIST_HEAD_INIT(pcbFree_h);

/*
Inizializza la lista pcbFree in modo da
contenere tutti gli elementi della
pcbFree_table. Questo metodo deve
essere chiamato una volta sola in fase di
inizializzazione della struttura dati.
*/

void initPcbs(){
    for(int i = 0; i < MAXPROC; i++)
        list_add_tail(&pcbFree_table[i],&pcbFree_h);
}

/*
Inserisce il PCB puntato da p nella lista
dei PCB liberi (pcbFree_h)
*/

void freePcb(pcb_t * p){
    list_add_tail(&p,&pcbFree_h);
}

/*
Restituisce NULL se la pcbFree_h è vuota.
Altrimenti rimuove un elemento dalla
pcbFree, inizializza tutti i campi (NULL/0)
e restituisce l’elemento rimosso.
*/

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

/*
Crea una lista di PCB, inizializzandola
come lista vuota
*/

void mkEmptyProcQ(struct list_head *head){
    INIT_LIST_HEAD(head);
}

/*
Restituisce TRUE se la lista puntata da
head è vuota, FALSE altrimenti.
*/

int emptyProcQ(struct list_head *head){
    //WARNING: se da errore aggiunger &
    if(list_empty(head))
        return TRUE;
    else 
        return FALSE;
}

/*
Inserisce l’elemento puntato da p nella
coda dei processi puntata da head
*/

void insertProcQ(struct list_head* head, pcb_t* p){
    list_add_tail(head,p);
}

/*
Restituisce l’elemento di testa della coda
dei processi da head, SENZA
RIMUOVERLO. Ritorna NULL se la coda
non ha elementi.
*/

pcb_t *headProcQ(struct list_head* head){
    if(emptyProcQ(head))
        return NULL;
    return container_of(head,pcb_t,p_list.next);
}

/*
Rimuove il primo elemento dalla coda dei
processi puntata da head. Ritorna NULL se la
coda è vuota. Altrimenti ritorna il puntatore
all’elemento rimosso dalla lista
*/

pcb_t* removeProcQ(struct list_head* head){
    pcb_t *p = NULL;

    if(emptyProcQ(head))
        return NULL;
    list_del(container_of(head,pcb_t,p_list.next));
    p = container_of(head,pcb_t,p_list.next);
    return p;
}

/*
Rimuove il PCB puntato da p dalla coda dei
processi puntata da head. Se p non è presente
nella coda, restituisce NULL. (NOTA: p può
trovarsi in una posizione arbitraria della coda).
*/

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

/*
Restituisce TRUE se il PCB puntato da p
non ha figli, FALSE altrimenti.
*/

int emptyChild(pcb_t *p){
    if(list_empty(&p->p_child))
        return TRUE;
    return FALSE;
}

/*
Inserisce il PCB puntato da p come figlio
del PCB puntato da prnt
*/

void insertChild(pcb_t *prnt,pcb_t *p){
    INIT_LIST_HEAD(&p->p_sib);
    list_add_tail(&p->p_sib,&prnt->p_child);
    p->p_parent = prnt;
}

/*
Rimuove il primo figlio del PCB puntato
da p. Se p non ha figli, restituisce NULL.
*/

pcb_t* removeChild(pcb_t *p){
    if(emptyChild(p))
        return NULL;
    //rimozione primo nodo
    pcb_t *rem = container_of(p->p_child.next,pcb_t,p_sib);
    list_del(&rem->p_sib);
}

/*
Rimuove il PCB puntato da p dalla lista
dei figli del padre. Se il PCB puntato da
p non ha un padre, restituisce NULL,
altrimenti restituisce l’elemento
rimosso (cioè p). A differenza della
removeChild, p può trovarsi in una
posizione arbitraria (ossia non è
necessariamente il primo figlio del
padre).
*/

pcb_t *outChild(pcb_t* p){
    if(p->p_parent !=NULL){
        list_del(&p->p_sib);
        return p;
    }
    return NULL;
}