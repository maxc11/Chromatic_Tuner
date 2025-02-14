#include "interrupt.h"

XStatus Status;
//extern XIntc sys_intc;
//extern XTmrCtr tmrctr;
//int count = 0;
uint32_t Control;

//void timer_setup() {
//    // Initialize the interrupt controller
//    XIntc_Initialize(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);
//
//    // Connect the timer interrupt handler
//    XIntc_Connect(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR,
//                  (XInterruptHandler)timer_handler, &tmrctr);
//
//    // Start and enable the interrupt controller
//    XIntc_Start(&sys_intc, XIN_REAL_MODE);
//    // enable was here
//    // Initialize the timer
//    XTmrCtr_Initialize(&tmrctr, XPAR_AXI_TIMER_0_DEVICE_ID);
//    Control = XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION; // Enable auto-reload for continuous interrupt
//    XTmrCtr_SetOptions(&tmrctr, 0, Control);
//
//    // Set reset value for the timer to control interrupt frequency
//    XTmrCtr_SetResetValue(&tmrctr, 0, 0xFFFFFFFF - RESET_VALUE);
//    XTmrCtr_Start(&tmrctr, 0);
//
//    // Register and enable interrupts for MicroBlaze
//    microblaze_register_handler((XInterruptHandler)XIntc_DeviceInterruptHandler,
//                                (void*)XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);
//    microblaze_enable_interrupts();
//}
