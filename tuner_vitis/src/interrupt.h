#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "xtmrctr.h"
#include "xintc.h"
#include "xparameters.h"
#include <xbasic_types.h>

#define RESET_VALUE 50000000 // (100000000 for 1s) (10000000 for .1s) (1000000 for 0.01s)

void timer_handler(void *CallbackRef);
void timer_setup(void);

void enable_timer(void);
void disable_timer(void);


#endif
