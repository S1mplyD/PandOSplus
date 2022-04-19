#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "umps3/umps/libumps.h"
#include "umps3/umps/arch.h"
#include "pandos_const.h"
#include "scheduler.h"
#include "umps3/umps/const.h"
#include "asl.h"
#include "umps3/umps/cp0.h"
#include "utility.h"
#include "exception.h"

void interruptHandler(state_t *exceptionState);
void interrupt(int lineNumber, state_t *exceptionState);

#endif