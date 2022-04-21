#include "pcb.h"

static pcb_t pcbFree_table[MAXPROC];
LIST_HEAD(pcbFree_h);

/*
Inizializza la lista pcbFree in modo da
contenere tutti gli elementi della
pcbFree_table. Questo metodo deve
essere chiamato una volta sola in fase di
inizializzazione della struttura dati.
*/

void initPcbs()
{

    // Lista dei PCB liberi o inutilizzati
    static pcb_t *pcbFree;

    // Sentinella pcbFree
    static pcb_t sen;
    INIT_LIST_HEAD(&(sen.p_list));
    pcbFree = &sen;

    pcbFree_h = pcbFree->p_list;

    // Aggiunta di tutti i pcb nella lista dei pcb liberi
    for (int i = 0; i < MAXPROC; i++)
    {
        freePcb(&pcbFree_table[i]);
    }
}

/*
Inserisce il PCB puntato da p nella lista
dei PCB liberi (pcbFree_h)
*/

void freePcb(pcb_t *p)
{
    list_add(&(p->p_list), &pcbFree_h);
}

/*
Restituisce NULL se la pcbFree_h è vuota.
Altrimenti rimuove un elemento dalla
pcbFree, inizializza tutti i campi (NULL/0)
e restituisce l’elemento rimosso.
*/

pcb_t *allocPcb()
{

    // Controllo se la lista è vuota
    if (list_empty(&pcbFree_h))
    {
        return NULL;
    }

    // Rimuovo un elemento da pcbFree_h
    struct list_head *head = list_next(&pcbFree_h);
    pcb_t *rem = container_of(head, pcb_t, p_list);
    list_del(&(rem->p_list));

    // Setto tutto a NULL/0
    rem->p_parent = NULL;
    INIT_LIST_HEAD(&rem->p_child);
    INIT_LIST_HEAD(&rem->p_list);
    INIT_LIST_HEAD(&rem->p_sib);
    rem->p_prio = 0;
    rem->p_time = 0;
    rem->p_semAdd = 0;
    rem->p_pid = ++pid;
    rem->p_supportStruct = NULL;

    return rem;
}

/*
Crea una lista di PCB, inizializzandola
come lista vuota
*/

void mkEmptyProcQ(struct list_head *head)
{
    INIT_LIST_HEAD(head);
}

/*
Restituisce TRUE se la lista puntata da
head è vuota, FALSE altrimenti.
*/

int emptyProcQ(struct list_head *head)
{
    if (list_empty(head))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*
Inserisce l’elemento puntato da p nella
coda dei processi puntata da head
*/

void insertProcQ(struct list_head *head, pcb_t *p)
{
    list_add_tail(&p->p_list, head);
}

/*
Restituisce l’elemento di testa della coda
dei processi da head, SENZA
RIMUOVERLO. Ritorna NULL se la coda
non ha elementi.
*/

pcb_t *headProcQ(struct list_head *head)
{
    if (emptyProcQ(head))
    {
        return NULL;
    }
    return container_of(head->next, pcb_t, p_list);
}

/*
Rimuove il primo elemento dalla coda dei
processi puntata da head. Ritorna NULL se la
coda è vuota. Altrimenti ritorna il puntatore
all’elemento rimosso dalla lista
*/

pcb_t *removeProcQ(struct list_head *head)
{
    if (!list_empty(head))
    {
        // the list is not empty, recover the first object
        struct list_head *next = head->next;
        struct pcb_t *head_pcb = container_of(next, pcb_t, p_list);
        // remove the object from the list
        list_del(next);
        return head_pcb;
    }
    else
    {
        return NULL;
    }
}

/*
Rimuove il PCB puntato da p dalla coda dei
processi puntata da head. Se p non è presente
nella coda, restituisce NULL. (NOTA: p può
trovarsi in una posizione arbitraria della coda).
*/

pcb_t *outProcQ(struct list_head *head, pcb_t *p)
{
    if (!emptyProcQ(head))
    {
        struct list_head *it;

        list_for_each(it, head)
        {
            pcb_t *curr = container_of(it, pcb_t, p_list);
            if (curr == p)
            {
                list_del(it);
                return p;
            }
        }
    }
    return NULL;
}

/*
Restituisce TRUE se il PCB puntato da p
non ha figli, FALSE altrimenti.
*/

int emptyChild(pcb_t *p)
{
    if (list_empty(&p->p_child))
        return TRUE;
    return FALSE;
}

/*
Inserisce il PCB puntato da p come figlio
del PCB puntato da prnt
*/

void insertChild(pcb_t *prnt, pcb_t *p)
{
    list_add_tail(&p->p_sib, &prnt->p_child);
    p->p_parent = prnt;
}

/*
Rimuove il primo figlio del PCB puntato
da p. Se p non ha figli, restituisce NULL.
*/

pcb_t *removeChild(pcb_t *p)
{
    if (!emptyChild(p))
    {
        // Rimozione primo nodo
        pcb_t *rem = container_of(p->p_child.next, pcb_t, p_sib);
        list_del(&rem->p_child);
        return rem;
    }
    else
    {
        return NULL;
    }
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

pcb_t *outChild(pcb_t *p)
{
    if (p->p_parent != NULL)
    {
        list_del(&p->p_sib);
        return p;
    }
    return NULL;
}