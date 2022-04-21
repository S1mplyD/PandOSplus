#include "scheduler.h"

void scheduler()
{

    while (TRUE)
    {

        if (emptyProcQ(&HI_readyQueue))
        {

            // Non ci sono processi ad alta priorità
            if (emptyProcQ(&LO_readyQueue))
            {
                // Non ci sono processi ad bassa priorità
                if (processCount <= 0)
                {
                    // I processi sono finiti
                    HALT();
                }

                if (processCount > 0 && softBlockCounter > 0)
                {

                    // Abilito gli interrupt
                    setSTATUS((getSTATUS() | IECON | IMON));
                    // disabilito il PLT
                    setTIMER(0xFFFFFFFF);

                    WAIT();
                }
                else if (processCount > 0 && softBlockCounter == 0)
                {
                    // else if per sicurezzas
                    // Ci sono processi non terminati, e nessun processo è nello stato "blocked"
                    PANIC();
                }
            }
            else
            {
                // Sono presenti processi con priorità bassa
                // Prendo il primo processo dalla Low priority readyQueue
                currentProcess = removeProcQ(&LO_readyQueue);
                // Setto il tempo di inizio del processo
                STCK(cPStartT);
                // Metto nel PLT 5ms
                setTIMER(TIMESLICE);

                LDST((void *)&currentProcess->p_s);
            }
        }
        else
        {
            // Sono presenti processi con priorità alta
            currentProcess = removeProcQ(&HI_readyQueue);
            // Setto il tempo di inizio del processo
            STCK(cPStartT);

            LDST((void *)&currentProcess->p_s);
        }
    }
}