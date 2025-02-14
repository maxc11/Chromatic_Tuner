/*****************************************************************************
* bsp.c for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 27,2019
*****************************************************************************/

/**/
#include "qpn_port.h"
#include "bsp.h"
#include "lab2a.h"
#include "interrupt.h"

#include "xil_exception.h"

//led stuff
#include "xspi.h"
#include "xspi_l.h"
#include "lcd.h"

// fft stuff
#include "fft.h"
#include "note.h"
//#include "interrupt.h"


/*****************************/

/* Define all variables and Gpio objects here  */

XIntc sys_intc;
XTmrCtr sys_tmrctr;

int turned = 0; //global flag to tell when and which direction turned
int count = 0; //count for timer
int twist_timeout_time = 0; // rotary encoder timeout counter
int push_timeout_time = 0; // push button timeout
int pressed = 0; //for encoder, 1 if has been pressed, 0 if not pressed
volatile int button = 0;
int volume_level = 0;
int controller;

enum states {
	rest,
	cw1,
	cw2,
	cw3,
	ccw1,
	ccw2,
	ccw3
};

struct FSM {
	enum states cur;
	enum states prev;
} rotary_fsm;

unsigned int encoder_read; // add extern?
int wtf = 1;
typedef enum{
	not_click,
	new_click
} button_states;

typedef enum
{
	THREE_CCW= 1,
	ONE_CW = 1,//0b001,
	TWO_CCW = 0,
	TWO_CW = 0,//0b000,
	ONE_CCW = 2,
	THREE_CW = 2,//0b010,
	INITIAL = 3,//0b011,
	CLICK = 4,//0b100,
}  rotate_bits;

//for btn



//--------------------


//gpio objects IN HEADER


static XGpio sys_btn;
static XGpio sys_encoder;
static XGpio rgb_led;
static XGpio scroll_led;
static XGpio dc;
static XSpi spi;
//-------

#define GPIO_CHANNEL1 1

void debounceInterrupt(); // Write This function

// Create ONE interrupt controllers XIntc
// Create two static XGpio variables
// Suggest Creating two int's to use for determining the direction of twist

/*..........................................................................*/
void BSP_init(void) {
/* Setup LED's, etc */
/* Setup interrupts and reference to interrupt handler function(s)  */

	//for lcd

	u32 status;
	u32 controlReg;

	XSpi_Config *spiConfig;

	XGpio_Initialize(&dc, XPAR_SPI_DC_DEVICE_ID);
	XGpio_SetDataDirection(&dc, 1, 0x0);
	spiConfig = XSpi_LookupConfig(XPAR_SPI_DEVICE_ID);
	//i suspect this part might break.... potentially
	status = XSpi_CfgInitialize(&spi, spiConfig, spiConfig->BaseAddress);
	if (status != XST_SUCCESS) {
		xil_printf("Initialize spi fail!\n");
		return XST_FAILURE;
	}
	XSpi_Reset(&spi);

	controlReg = XSpi_GetControlReg(&spi);
	XSpi_SetControlReg(&spi,
			(controlReg | XSP_CR_ENABLE_MASK | XSP_CR_MASTER_MODE_MASK) &
			(~XSP_CR_TRANS_INHIBIT_MASK));

	XSpi_SetSlaveSelectReg(&spi, ~0x01);

			/*
			while (1) {
				if (timerTrigger > 0) {
					secTmp = sec++ % 1000;
					secStr[0] = secTmp / 100 + 48;
					secTmp -= (secStr[0] - 48) * 100;
					secStr[1] = secTmp / 10 + 48;
					secTmp -= (secStr[1] - 48) * 10;
					secStr[2] = secTmp + 48;
					secStr[3] = '\0';

					lcdPrint(secStr, 70, 190);

					timerTrigger = 0;
				}
			} */



	XIntc_Initialize(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);

	XIntc_Connect(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR,
			(XInterruptHandler) timer_handler, &sys_tmrctr);
	//below added
	XIntc_Connect(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_BTN_IP2INTC_IRPT_INTR,
				(XInterruptHandler) button_handler, &sys_btn);

	XIntc_Connect(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_ENCODER_IP2INTC_IRPT_INTR,
				(XInterruptHandler) encoder_handler, &sys_encoder);
	//above added (for button)


	XIntc_Start(&sys_intc, XIN_REAL_MODE);


	XIntc_Enable(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR);
	//below added for btn
	XIntc_Enable(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_BTN_IP2INTC_IRPT_INTR);
	XIntc_Enable(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_ENCODER_IP2INTC_IRPT_INTR);
	// above added for btn
	XTmrCtr_Initialize(&sys_tmrctr, XPAR_AXI_TIMER_0_DEVICE_ID);
	//below for btn
	XGpio_Initialize(&sys_btn, XPAR_AXI_GPIO_BTN_DEVICE_ID);
	XGpio_InterruptEnable(&sys_btn, 1);
	XGpio_InterruptGlobalEnable(&sys_btn);

	XGpio_Initialize(&sys_encoder, XPAR_AXI_GPIO_ENCODER_DEVICE_ID);
	XGpio_InterruptEnable(&sys_encoder, 1);
	XGpio_InterruptGlobalEnable(&sys_encoder);
	   // above added for btn

	XTmrCtr_SetOptions(&sys_tmrctr, 0, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION | 1);

	XTmrCtr_SetResetValue(&sys_tmrctr, 0, 0xFFFFFFFF - RESET_VALUE);// 1000 clk cycles @ 100MHz = 10us

	XTmrCtr_Start(&sys_tmrctr, 0);



	microblaze_register_handler(
			(XInterruptHandler) XIntc_DeviceInterruptHandler,
			(void*) XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);

	microblaze_enable_interrupts();
	xil_printf("Interrupts enabled!\r\n");

	/*
	 * Initialize the interrupt controller driver so that it's ready to use.
	 * specify the device ID that was generated in xparameters.h
	 *
	 * Initialize GPIO and connect the interrupt controller to the GPIO.
	 *
	 */

	// Press Knob

	// Twist Knob
		
}
/*..........................................................................*/
void QF_onStartup(void) {                 /* entered with interrupts locked */

/* Enable interrupts */
	xil_printf("\n\rQF_onStartup\n"); // Comment out once you are in your complete program

	// Press Knob
	// Enable interrupt controller
	// Start interupt controller
	// register handler with Microblaze
	// Global enable of interrupt
	// Enable interrupt on the GPIO

	// Twist Knob

	// General
	// Initialize Exceptions
	// Press Knob
	// Register Exception
	// Twist Knob
	// Register Exception
	// General
	// Enable Exception

	// Variables for reading Microblaze registers to debug your interrupts.
//	{
//		u32 axi_ISR =  Xil_In32(intcPress.BaseAddress + XIN_ISR_OFFSET);
//		u32 axi_IPR =  Xil_In32(intcPress.BaseAddress + XIN_IPR_OFFSET);
//		u32 axi_IER =  Xil_In32(intcPress.BaseAddress + XIN_IER_OFFSET);
//		u32 axi_IAR =  Xil_In32(intcPress.BaseAddress + XIN_IAR_OFFSET);
//		u32 axi_SIE =  Xil_In32(intcPress.BaseAddress + XIN_SIE_OFFSET);
//		u32 axi_CIE =  Xil_In32(intcPress.BaseAddress + XIN_CIE_OFFSET);
//		u32 axi_IVR =  Xil_In32(intcPress.BaseAddress + XIN_IVR_OFFSET);
//		u32 axi_MER =  Xil_In32(intcPress.BaseAddress + XIN_MER_OFFSET);
//		u32 axi_IMR =  Xil_In32(intcPress.BaseAddress + XIN_IMR_OFFSET);
//		u32 axi_ILR =  Xil_In32(intcPress.BaseAddress + XIN_ILR_OFFSET) ;
//		u32 axi_IVAR = Xil_In32(intcPress.BaseAddress + XIN_IVAR_OFFSET);
//		u32 gpioTestIER  = Xil_In32(sw_Gpio.BaseAddress + XGPIO_IER_OFFSET);
//		u32 gpioTestISR  = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_ISR_OFFSET ) & 0x00000003; // & 0xMASK
//		u32 gpioTestGIER = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_GIE_OFFSET ) & 0x80000000; // & 0xMASK
//	}
}


void QF_onIdle(void) {        /* entered with interrupts locked */

    QF_INT_UNLOCK();                       /* unlock interrupts */

    {
    	// Write code to increment your interrupt counter here.
    	 QActive_postISR((QActive *)&AO_Lab2A, COMPUTE_FFT_SIG); // is used to post an event to your FSM



// 			Useful for Debugging, and understanding your Microblaze registers.
//    		u32 axi_ISR =  Xil_In32(intcPress.BaseAddress + XIN_ISR_OFFSET);
//    	    u32 axi_IPR =  Xil_In32(intcPress.BaseAddress + XIN_IPR_OFFSET);
//    	    u32 axi_IER =  Xil_In32(intcPress.BaseAddress + XIN_IER_OFFSET);
//
//    	    u32 axi_IAR =  Xil_In32(intcPress.BaseAddress + XIN_IAR_OFFSET);
//    	    u32 axi_SIE =  Xil_In32(intcPress.BaseAddress + XIN_SIE_OFFSET);
//    	    u32 axi_CIE =  Xil_In32(intcPress.BaseAddress + XIN_CIE_OFFSET);
//    	    u32 axi_IVR =  Xil_In32(intcPress.BaseAddress + XIN_IVR_OFFSET);
//    	    u32 axi_MER =  Xil_In32(intcPress.BaseAddress + XIN_MER_OFFSET);
//    	    u32 axi_IMR =  Xil_In32(intcPress.BaseAddress + XIN_IMR_OFFSET);
//    	    u32 axi_ILR =  Xil_In32(intcPress.BaseAddress + XIN_ILR_OFFSET) ;
//    	    u32 axi_IVAR = Xil_In32(intcPress.BaseAddress + XIN_IVAR_OFFSET);
//
//    	    // Expect to see 0x00000001
//    	    u32 gpioTestIER  = Xil_In32(sw_Gpio.BaseAddress + XGPIO_IER_OFFSET);
//    	    // Expect to see 0x00000001
//    	    u32 gpioTestISR  = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_ISR_OFFSET ) & 0x00000003;
//
//    	    // Expect to see 0x80000000 in GIER
//    		u32 gpioTestGIER = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_GIE_OFFSET ) & 0x80000000;


    }
}

/* Q_onAssert is called only when the program encounters an error*/
/*..........................................................................*/
void Q_onAssert(char const Q_ROM * const Q_ROM_VAR file, int line) {
    (void)file;                                   /* name of the file that throws the error */
    (void)line;                                   /* line of the code that throws the error */
    QF_INT_LOCK();
    printDebugLog();
    for (;;) {
    }
}

/* Interrupt handler functions here.  Do not forget to include them in lab2a.h!
To post an event from an ISR, use this template:
QActive_postISR((QActive *)&AO_Lab2A, SIGNALHERE);
Where the Signals are defined in lab2a.h  */

/******************************************************************************
*
* This is the interrupt handler routine for the GPIO for this example.
*
******************************************************************************/

void initial_background() {
	//draws background, just for initial

	initLCD();
	clrScr();

	for(int i = 0; i < 5; i++) {
		for (int j = 0; j < 7; j++) {
		setColor(252, 201, 255);
		fillRect(20 + 40*i, 20 + 40 * j, 60+40*i, 59 + 40 * j);
		setColor(10 + 30*j, 0, 220);
		draw_triangle(20 + 40*i, 20 + 40 * j, 40, 40);
		}
	}

}

void draw_background() {
	//setColor(255, 0, 0);
	//fillRect(20, 20, 220, 300);

	//setColor(0, 255, 0);
	//setColorBg(255, 0, 0);
	//lcdPrint("Hello !!!!!", 40, 60);

	//drawHLine(30, 250, 30);

	//setFont(BigFont);
	//lcdPrint("<FARTTTTTT>", 40, 80);

	setFont(SevenSegNumFont);
	setColor(238, 64, 0);
	setColorBg(0, 50, 100);
}

void update_display(int volume) {
	static int prev_volume = 0;
	setColor(0, 50, 100);
	if (volume != 65) {
		fillRect(59 + 2 * volume, 185, 185, 215);
	}
	setColor(13, 255, 166);
	fillRect(56, 185, 56 + 2 * volume, 215);
	if (volume == 65) {
		fillRect(183, 185, 185, 215);
	}
	setColor(0, 50, 100);
	if (volume == 0) {
		fillRect(56, 185, 58, 215);
	}
	prev_volume = volume;
}


void update_btn_display1() {
//	xil_printf("updatingbtn1display\r\n");
	setColor(0, 50, 100);
	fillRect(40, 145, 200, 175);

	setColor(255, 135, 0);
	setColorBg(20, 50, 100);
	lcdPrint("1. LT", 50, 155);
}

void update_btn_display2() {
//	xil_printf("updatingbtn2display\r\n");
	setColor(0, 50, 100);
	fillRect(40, 145, 200, 175);

	setColor(255, 135, 0);
	setColorBg(20, 50, 100);
	lcdPrint("2. Hi Ryan c:", 50, 155);
}

void update_btn_display3() {
//	xil_printf("updatingbtn3display\r\n");
	setColor(20, 50, 100);
	fillRect(40, 145, 200, 175);

	setColor(255, 135, 0);
	setColorBg(20, 50, 100);
	lcdPrint("3. Woohoo", 50, 155);
}

void update_btn_display4() {
//	xil_printf("updatingbtn4display\r\n");
	setColor(0, 50, 100);
	fillRect(40, 145, 200, 175);

	setColor(255, 135, 0);
	setColorBg(20, 50, 100);
	lcdPrint("4. Hello Brandon :)", 50, 155);
}

void update_btn_display5() {
//	xil_printf("updatingbtn5display\r\n");
	setColor(0, 50, 100);
	fillRect(40, 145, 200, 175);

	setColor(255, 135, 0);
	setColorBg(20, 50, 100);
	lcdPrint("5. Hey Janani!!", 50, 155);
}

void reset_display(int height){ //count down, including first. so for 5th triangle from top, its 4, etc. 4th triangle is 3
	int j = height; //re use code from other thing.
	for(int i = 0; i < 5; i++) {
		setColor(252, 201, 255);
		fillRect(20 + 40*i, 20 + 40 * j, 60+40*i, 59 + 40 * j);
		setColor(10 + 30*j, 0, 220);
		draw_triangle(20 + 40*i, 20 + 40 * j, 40, 40);
	}
}

void draw_triangle(int x, int y, int height, int length) { // x,y is bottom left corner of triangle
	int i;
	float slope;
	slope = (float)(2*height)/(length);
	for(i = 0; i<height; i++) {
		drawHLine(x+(i/slope), y + i, length - 2*(i/slope));
	}
}

/*
 * FFT code below, related to timer_handler() as it will gather fft every 0.1s (100ms). FFT currently needs 9ms to execute
 *
 */



void timer_handler(void *CallbackRef) {
	/*
	 * run fft twice. first on 64pt then on 128pt
	 * 1. run 64pt fft normally
	 * 2. take the freq and then decide decimation value
	 * 3.
	 * ALL THE FFT COMPUTATION SHOULD LIVE SOMEWHERE ELSE AND BE CALLED BY A QPNANO SIGNAL
	 * this handler should only be dispatching qp signals to trigger events:
	 * such as draw display, compute fft, etc.
	 *
	 */
    Xuint32 ControlStatusReg;
    ControlStatusReg = XTmrCtr_ReadReg(sys_tmrctr.BaseAddress, 0, XTC_TCSR_OFFSET);
    count++;
//    xil_printf("timer interrupt went off count: %d\r\n", count);


    // post signals depending on the preferred timing of updating display or running fft
    QActive_postISR((QActive *) &AO_Lab2A, DRAW_SCREEN_SIG);


    // Clear the interrupt
    XTmrCtr_WriteReg(sys_tmrctr.BaseAddress, 0, XTC_TCSR_OFFSET, ControlStatusReg | XTC_CSR_INT_OCCURED_MASK);
}

void enable_timer() {
	XIntc_Enable(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR);
}

void disable_timer() {
	XIntc_Disable(&sys_intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR);
}

void encoder_handler(void *CallbackRef) {
	//XGpio_DiscreteRead( &twist_Gpio, 1);
	XGpio *GpioPtr = (XGpio *)(CallbackRef);
	encoder_read = XGpio_DiscreteRead(&sys_encoder, 1);
	rotary_debouncer();
	if (encoder_read > 3) {
		pressed = (pressed) ? 0 : 1;
	}
	if(pressed){
//		xil_printf("pressed \r\n");
		encoder_press_interrupt();
	}
//	xil_printf("encoderhandlerworking\r\n");
	XGpio_InterruptClear(GpioPtr, 1);
}

void button_handler(void *CallbackRef) {
    XGpio *GpioPtr = (XGpio *)CallbackRef;
    button = XGpio_DiscreteRead(&sys_btn, 1);
    // Switch statements to determine what happens based on btn value
    // xil_printf("buttonhandlerworking\r\n");
    QActive_postISR((QActive *) &AO_Lab2A, BTN_SIG);
//    if (button == 0b00001) {
//    	xil_printf("uppressed\r\n");
//    }

    // Clear the interrupt for button GPIO
    XGpio_InterruptClear(GpioPtr, 1);
}

void rotary_debouncer(void) {
	switch(rotary_fsm.cur) {
		case rest:
			switch(encoder_read){
			case 0b10: // counter clockwise to ccw1
				rotary_fsm.cur = ccw1;
//					xil_printf("cur now ccw1 from rest\r\n");
				break;
			case 0b01: //clockwise transition to cw1
				rotary_fsm.cur = cw1; //At rest, point to
//					xil_printf("cur now cw1 from rest\r\n");
				break;
			case 0b11: //At rest, stay at rest
				rotary_fsm.cur = rest;
				break;
			}
			break;
		case cw1:
			switch(encoder_read) {
			case 0b00: //to cw2, (continue on)
				rotary_fsm.cur = cw2;
//					xil_printf("cur now cw2 from cw1\r\n");
				break;
			case 0b10: // back to self,
				rotary_fsm.cur = cw1; //
//					xil_printf("cur now cw1 from cw1\r\n");
				break;
			case 0b11: // bounce handler, back to rest
				rotary_fsm.cur = rest;
//					xil_printf("cur now rest from cw1\r\n");
				break;
			}
			break;
		case cw2:
			switch(encoder_read) {
			case 0b00: // point back to self
				rotary_fsm.cur = cw2;
				break;
			case 0b10: //continue on
				rotary_fsm.cur = cw3;
				break;
			case 0b01: // bounce handler, back to prev state
				rotary_fsm.cur = cw1;
				break;
			}
			break;
		case cw3:
			switch(encoder_read) {
			case 0b00: // bounce handler, back to prev state
				rotary_fsm.cur = cw2;
				break;
			case 0b10: // point back to self
				rotary_fsm.cur = cw3;
				break;
			case 0b11: //back to rest, cycle complete! Light up LED
				// increase volume
				QActive_postISR((QActive*) &AO_Lab2A, ENCODER_UP_SIG);
				rotary_fsm.cur = rest;
				break;
			}
			break;
		case ccw1:
			switch(encoder_read) {
			case 0b11:
				rotary_fsm.cur = rest;
				break;
			case 0b10: // Stay at current state
				rotary_fsm.cur = ccw1;
				break;
			case 0b00:
				rotary_fsm.cur = ccw2;
				break;
			}
			break;
		case ccw2:
			switch(encoder_read) {
			case 0b10:
				rotary_fsm.cur = ccw1;
				break;
			case 0b00: // Stay at current state
				rotary_fsm.cur = ccw2;
				break;
			case 0b01:
				rotary_fsm.cur = ccw3;
				break;
			}
			break;
		case ccw3:
			switch(encoder_read) {
			case 0b11:
				// decrease volume
				QActive_postISR((QActive*) &AO_Lab2A, ENCODER_DOWN_SIG);
				rotary_fsm.cur = rest;
				break;
			case 0b00: // Stay at current state
				rotary_fsm.cur = ccw2;
				break;
			case 0b01:
				rotary_fsm.cur = ccw3;
				break;
			}
			break;
		default:
//			xil_printf("someshit happened\r\n");
			break;
	}
}

void encoder_press_interrupt() {
	if (pressed) {
		QActive_postISR((QActive*) &AO_Lab2A, ENCODER_CLICK_SIG);
	}
}
/*
void debounceInterrupt(int button_bits) {
	static button_states button_state = not_click;
		static button_states new_button_state = not_click;
		static int count_at_last_press = 0;
		switch(button_state) {
					   	    case(not_click):
					   			if(button_bits != CLICK){new_button_state = not_click;}
					   			else{
					   				new_button_state = new_click;
					   				pressed = !pressed;
					   				QActive_postISR((QActive *)&AO_Lab2A, ENCODER_PUSH_SIG);
					   			}
					   		break;
					   	    case(new_click):
					   	    	if((count > count_at_last_press + 2)) { //4 for 400ms
					   	    		if(button_bits == CLICK) {
					   	    		new_button_state = new_click;
					   	    		pressed = !pressed;
					   	    		QActive_postISR((QActive *)&AO_Lab2A, ENCODER_PUSH_SIG);
					   	    		}
					   	    		else{new_button_state = not_click;}
					   	    	}
					   	    	count_at_last_press = count;
					   	    break;
					   	    }
		   button_state = new_button_state;
		   //xil_printf("%i", pressed);
	// XGpio_InterruptClear(&sw_Gpio, GPIO_CHANNEL1); // (Example, need to fill in your own parameters
}
*/
