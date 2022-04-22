#include "asl.h"

static struct semd_t semd_table[MAXPROC];

LIST_HEAD(semdFree_h);

LIST_HEAD(semd_h);

/*
Restituisce il semaforo che ha chiave semKey
*/
semd_t *getSemd(int *semKey)
{
  struct list_head *it;
  list_for_each(it, &semd_h)
  {
    semd_t *curr = container_of(it, semd_t, s_link);
    // Il semaforo è presente nella ASL
    if (curr->s_key == semKey)
    {
      return curr;
    }
  }
  return NULL;
}

/*
Viene inserito il PCB puntato da p nella coda dei
processi bloccati associata al SEMD con chiave
semAdd.
Se il semaforo corrispondente non è
presente nella ASL, alloca un nuovo SEMD dalla
lista di quelli liberi (semdFree) e lo inserisce nella
ASL, settando I campi in maniera opportuna (i.e.
key e s_procQ). Se non è possibile allocare un
nuovo SEMD perché la lista di quelli liberi è vuota,
restituisce TRUE. In tutti gli altri casi, restituisce
FALSE.
*/
int insertBlocked(int *semAdd, pcb_t *p)
{

  semd_t *sem;

  sem = getSemd(semAdd);

  if (sem == NULL)
  { // Semaforo non presente
    if (emptyProcQ(&semdFree_h))
    { // Nessun semd disponibile
      return TRUE;
    }

    sem = container_of(list_next(&semdFree_h), semd_t, s_link);
    // Set dei parametri
    sem->s_key = semAdd;
    INIT_LIST_HEAD(&(sem->s_procq));

    list_del(&(sem->s_link));
    list_add_tail(&(sem->s_link), &semd_h);
  }

  // Aggiungo p alla coda dei processi di sem
  p->p_semAdd = semAdd;
  insertProcQ(&(sem->s_procq), p);

  return FALSE;
}

/*
Ritorna il primo PCB dalla coda dei processi
bloccati (s_procq) associata al SEMD della
ASL con chiave semAdd. Se tale descrittore
non esiste nella ASL, restituisce NULL.
Altrimenti, restituisce l’elemento rimosso. Se
la coda dei processi bloccati per il semaforo
diventa vuota, rimuove il descrittore
corrispondente dalla ASL e lo inserisce nella
coda dei descrittori liberi (semdFree_h)
*/
pcb_t *removeBlocked(int *semAdd)
{

  semd_t *sem;
  sem = getSemd(semAdd);

  if (sem == NULL)
  {
    return NULL;
  }

  pcb_t *p = removeProcQ(&sem->s_procq);

  if (emptyProcQ(&(sem->s_procq)))
  {
    list_del(&(sem->s_link));
    list_add_tail(&(sem->s_link), &semdFree_h);
  }

  return p;
}

/*
Rimuove il PCB puntato da p dalla coda del semaforo
su cui è bloccato (indicato da p->p_semAdd). Se il PCB
non compare in tale coda, allora restituisce NULL
(condizione di errore). Altrimenti, restituisce p. Se la
coda dei processi bloccati per il semaforo diventa
vuota, rimuove il descrittore corrispondente dalla ASL
e lo inserisce nella coda dei descrittori liberi
*/
pcb_t *outBlocked(pcb_t *p)
{

  semd_t *sem;
  sem = getSemd(p->p_semAdd);

  if (sem == NULL)
  {
    return NULL;
  }

  pcb_t *pnew = outProcQ(&(sem->s_procq), p);
  if (emptyProcQ(&(sem->s_procq)))
  {
    list_del(&(sem->s_link));
    list_add_tail(&(sem->s_link), &semdFree_h);
  }

  return pnew;
}

/*
Restituisce (senza rimuovere) il puntatore al PCB che
si trova in testa alla coda dei processi associata al
SEMD con chiave semAdd. Ritorna NULL se il SEMD
non compare nella ASL oppure se compare ma la sua
coda dei processi è vuota.
*/
pcb_t *headBlocked(int *semAdd)
{
  semd_t *sem;
  sem = getSemd(semAdd);

  if (sem == NULL)
    return NULL;

  return headProcQ(&(sem->s_procq));
}

/*
Inizializza la lista dei semdFree in
modo da contenere tutti gli elementi
della semdTable. Questo metodo
viene invocato una volta sola durante
l’inizializzazione della struttura dati.
*/
void initASL()
{
  for (int i = 0; i < MAXPROC; i++)
  {
    list_add(&semd_table[i].s_link, &semdFree_h);
  }
}
