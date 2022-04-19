#include <umps3/umps/libumps.h>
#include <pcb.h>
#include <asl.h>
#include <utility.h>

extern struct list_head LO_readyQueue;
extern struct list_head HI_readyQueue;
extern int softBlockCounter;
extern int processCount;
extern pcb_t *currentProcess;

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
                    HALT();
                }

                if (softBlockCounter > 0)
                {

                    // Abilito gli interrupt
                    setSTATUS((getSTATUS() | IECON | IMON));
                    // disabilito il PLT
                    setTIMER(0xFFFFFFFF);

                    WAIT();
                }
                else
                {
                    PANIC();
                }
            }
            else
            {

                currentProcess = removeProcQ(&LO_readyQueue);

                STCK(cPStartT);
                setTIMER(TIMESLICE);

                LDST((void *)&currentProcess->p_s);
            }
        }
        else
        {

            currentProcess = removeProcQ(&HI_readyQueue);

            STCK(cPStartT);

            LDST((void *)&currentProcess->p_s);
        }
    }
}
