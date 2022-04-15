#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "umps3/umps/libumps.h"
#include "umps3/umps/arch.h"
#include "pandos_const.h"
#include "scheduler.h"
#include "umps3/umps/const.h"
#include "asl.h"


void interruptHandler(int exceptionCode);
void interrupt(int lineNumber);

#endif