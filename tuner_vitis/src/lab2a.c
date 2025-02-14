/*****************************************************************************
* lab2a.c for Lab2A of ECE 153a at UCSB
* Updated: December 4, 2024
*****************************************************************************/

#define AO_LAB2A

#include "qpn_port.h"
#include "bsp.h"
#include "lab2a.h"
#include "fft.h"
#include "lcd.h"
#include "note.h"

#define UP 0b00001
#define LEFT 0b00010
#define RIGHT 0b00100
#define DOWN 0b01000
#define MIDDLE 0b10000
//#define UPDATE_NOTE_SIG 10

/*
 global vars
 */
extern volatile int button;
extern int volume_level; // max vol = 50, min = 0
extern int push_timeout_time;
extern int twist_timeout_time;
extern int count;
extern int pressed;
extern float frequency;
int a4 = 440;
int lol = 0;
int max_hist_bar = 275;
typedef struct Lab2ATag {               // Lab2A State machine
    QActive super;
} Lab2A;

/* Setup state machines */
static QState Lab2A_initial(Lab2A *me);
static QState parent(Lab2A *me);
static QState right(Lab2A *me);
static QState left(Lab2A *me);
static QState up(Lab2A *me);



/* Global variables */
Lab2A AO_Lab2A;
//static QStateHandler previous_state = NULL;
void Lab2A_ctor(void) {
    Lab2A *me = &AO_Lab2A;
    QActive_ctor(&me->super, (QStateHandler)&Lab2A_initial);
}

static QState Lab2A_initial(Lab2A *me) {
    xil_printf("\n\rInitialization\n\r");
	initLCD();
	clrScr();
    return Q_TRAN(&parent); // Start in the 'right' state
}

static QState parent(Lab2A *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
			xil_printf("entered parent state\r\n");
			initial_background();
			return Q_HANDLED();
		}
		case Q_INIT_SIG: {
			return Q_TRAN(&left);
		}
//		case COMPUTE_FFT_SIG: {
//			xil_printf("compute fft sig activated from timer interrupt\r\n");
//			// compute fft
//			if (previous_state != NULL) {
//				return Q_TRAN((QStateHandler)previous_state);
//			}
//			return Q_HANDLED();
//		}
		case BTN_SIG: {
			if (button == LEFT) {
//				xil_printf("LEFT button pressed from parent, tran\r\n");
				return Q_TRAN(&left);
			}
			if (button == RIGHT) {
//				xil_printf("RIGHT button pressed from parent, tran\r\n");
				return Q_TRAN(&right);
			}
			return Q_HANDLED();
		}
	}
	return Q_SUPER(&QHsm_top);
}

static QState right(Lab2A *me) {
    switch (Q_SIG(me)) {
        case Q_ENTRY_SIG: {
//            xil_printf("Entered Right state\r\n");
            // draw right state background (debug background)
//        	xil_printf("updatingbtn1display\r\n");
        	setColor(0, 0, 0);
        	fillRect(20, 75, 220, 275); //draw white background :D
//        	grid();
//        	bars();

            return Q_HANDLED();
        }
        case BTN_SIG: {
            if (button == LEFT) {  // Post UPDATE_NOTE_SIG and transition to Left state
              //  xil_printf("LEFT button pressed, posting UPDATE_NOTE_SIG and transitioning to Left state\r\n"); // takes us immediatley to UPDATE_NOTE_SIG
//                QActive_post((QActive *)me, UPDATE_NOTE_SIG); // Post the signal
                return Q_TRAN(&left); // Transition to the Left state
            }
            if(button ==UP){
            	 return Q_TRAN(&up);
            }
            return Q_HANDLED();
        }
        case COMPUTE_FFT_SIG: {
		//	xil_printf("fft sig received in right state\r\n");
			// compute fft
			run_fft();

//			return Q_TRAN(&right);


//			previous_state = (QStateHandler)&right;
//			return Q_TRAN(&parent);
        	return Q_HANDLED();
		}
        case DRAW_SCREEN_SIG: {
        	setColor(0, 0, 0);
        	fillRect(36, 275, 220, max_hist_bar);
        	grid();
			max_hist_bar = bars();
			return Q_HANDLED();
        }
    }
    return Q_SUPER(&parent);
}

static QState up(Lab2A *me) {
    switch (Q_SIG(me)) {
        case Q_ENTRY_SIG: {
//            xil_printf("Entered Up state\r\n");
            // draw right state background (debug background)



        	// print out spectrogram...
        	// logic to update lcd screen
            return Q_HANDLED();
        }
        case BTN_SIG: {
            if (button == LEFT) {  // Post UPDATE_NOTE_SIG and transition to Left state
              //  xil_printf("LEFT button pressed, posting UPDATE_NOTE_SIG and transitioning to Left state\r\n"); // takes us immediatley to UPDATE_NOTE_SIG
//                QActive_post((QActive *)me, UPDATE_NOTE_SIG); // Post the signal
                return Q_TRAN(&left); // Transition to the Left state
            }
            if(button ==RIGHT){
            	 return Q_TRAN(&right);
            }
            return Q_HANDLED();
        }
		case DRAW_SCREEN_SIG: {
			spect();
			// draw screen. signal is sent periodically every 40ms
//			xil_printf("drawingscreen\r\n");
			// color by "third" of max value
			//
			// draw
			return Q_HANDLED();
		}
        case COMPUTE_FFT_SIG: {
		//	xil_printf("fft sig received in right state\r\n");
			// compute fft
			run_fft();

		//	int *temp=calc_bins();

//			return Q_TRAN(&right);

//			previous_state = (QStateHandler)&right;
//			return Q_TRAN(&parent);
        	return Q_HANDLED();
		}
    }
    return Q_SUPER(&parent);
}


static QState left(Lab2A *me) {
    switch (Q_SIG(me)) {
        case Q_ENTRY_SIG: {
//            xil_printf("Entered Left state\r\n");
            init_tuner_screen();
            return Q_HANDLED();
//            setColor(0,0,0)
//            drawRect(20, 75, 220, 275);

        }
        case BTN_SIG: {
            if (button == RIGHT) {  // Transition to Right state on RIGHT button press
//                xil_printf("Transitioning back to Right state\r\n");
                return Q_TRAN(&right);
            }
            if (button ==UP){
            	 return Q_TRAN(&up);
            }
            return Q_HANDLED();
        }
//        case UPDATE_NOTE_SIG: {  // Now we can start calling our fft/calculating note values in here
//            xil_printf("We are in UPDATE_NOTE case in Left state\r\n");
//
//            return Q_HANDLED();
//        }
        case COMPUTE_FFT_SIG: {
//        	xil_printf("fft sig received in left state\r\n");
        	// compute fft
        	run_fft();
        	findNote();


//        	previous_state = (QStateHandler)&left;
//        	return Q_TRAN(&parent);
        	return Q_HANDLED();
        }
        case ENCODER_UP_SIG: {
			a4++;
			if (a4 > 460) a4 = 460;
//			xil_printf("a4 up: %d\r\n", a4);
			return Q_HANDLED();
		}
		case ENCODER_DOWN_SIG: {
			a4--;
			if (a4 < 420) a4 = 420;
//			xil_printf("a4 down: %d\r\n", a4);
			return Q_HANDLED();
		}
		case DRAW_SCREEN_SIG: {
			// draw screen. signal is sent periodically every 40ms
//			xil_printf("drawingscreen\r\n");
			draw_tuner_screen();
		}
    }
    return Q_SUPER(&parent);
}
