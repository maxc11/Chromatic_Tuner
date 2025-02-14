/*****************************************************************************
* lab2a.h for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 23,2014
*****************************************************************************/

#ifndef lab2a_h
#define lab2a_h

enum Lab2ASignals {
	ENCODER_UP_SIG = Q_USER_SIG,
	ENCODER_DOWN_SIG,
	ENCODER_CLICK_SIG,
	BTN_SIG,
	BTN_TIMEOUT_SIG,
	UPDATE_NOTE_SIG,
	TWIST_TIMEOUT_SIG,
	COMPUTE_FFT_SIG,
	DRAW_SCREEN_SIG,
};


extern struct Lab2ATag AO_Lab2A;


void Lab2A_ctor(void);
void GpioHandler(void *CallbackRef);
void TwistHandler(void *CallbackRef);

#endif  
