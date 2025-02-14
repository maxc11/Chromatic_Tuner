/*****************************************************************************
* bsp.h for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 23,2014
*****************************************************************************/
#ifndef bsp_h
#define bsp_h

#include "xtmrctr.h"
#include "xintc.h"
#include "xgpio.h"

/* bsp functions ..........................................................*/

void BSP_init(void);
void ISR_gpio(void);
void ISR_timer(void);

void timer_handler(void *CallbackRef);
void encoder_handler(void *CallbackRef);
void button_handler(void *CallbackRef);
//#define RESET_VALUE 10000000

void printDebugLog(void);

void update_btn_display1(void);
void update_btn_display2(void);
void update_btn_display3(void);
void update_btn_display4(void);
void update_btn_display5(void);


#define BSP_showState(prio_, state_) ((void)0)


#endif                                                             /* bsp_h */


