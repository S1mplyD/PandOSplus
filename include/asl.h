#include "pandos_types.h"
#include "pcb.h"

semd_t *getSemd(int *semKey);

int insertBlocked(int *semAdd,pcb_t *p);

pcb_t* removeBlocked(int *semAdd);

pcb_t* outBlocked(pcb_t *p);

pcb_t* headBlocked(int *semAdd);

void initASL();
