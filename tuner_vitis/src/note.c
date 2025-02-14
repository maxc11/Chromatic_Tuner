#include "note.h"
//#include "lcd.h"
#include "xil_printf.h"
#include <stdio.h>
#include <math.h>
#include "lcd.h"

//array to store note names for findNote
static char notes[12][3]={"C ","C#","D ","D#","E ","F ","F#","G ","G#","A ","A#","B "};
extern volatile float frequency;
char note_buf[4];
char octave[2];
char freq_buf[7];
char cents_buf[6];
char a4_buf[8];
int cents_err = 0;
//finds and prints note of frequency and deviation from note
void findNote(/*float f*/) {
	float captured_freq = frequency;
	float c4 = 261.63;
//	xil_printf("c4 before: %d \r\n", (int)c4);
	c4 = a4 * (0.59461363); //	 / 8.173; // * INV_8_176; // 261.63 was c4 before;
//	xil_printf("a4 val: %d c4: %d \r\n", a4, (int)c4);
	float r;
	int oct=4;
	int note=0;
	//determine which octave frequency is in
	if(frequency >= c4) {
		while(frequency > c4*2) {
			c4=c4*2;
			oct++;
		}
	}
	else { //f < C4
		while(frequency < c4) {
			c4=c4/2;
			oct--;
		}
	}
	if (oct >= 0) {
		snprintf(octave, sizeof(octave), "%d", oct);
	}
	//find note below frequency
	//c=middle C
	r=c4*root2;
	while(frequency > r) {
		c4=c4*root2;
		r=r*root2;
		note++;
	}

	if (!((frequency - c4) <= (r - frequency))) {
		c4 = r;
		note++;
	}

	cents_err = (int)round(1200 * log2(frequency / c4));

//	xil_printf("note: %s cents %d\r\n", notes[note], cents_err);



//		xil_printf("cents: %d\r\n", cents_err);
//	xil_printf("note: %s \r\n", notes[note]);
//	memset(note_buf, 0, sizeof(note_buf));
	if (!isnan(frequency) && frequency != 0) { // check if frequency is !NaN
		snprintf(cents_buf, sizeof(cents_buf), " %d  ", cents_err);
		snprintf(freq_buf, sizeof(freq_buf), "%0.2f ", frequency);
		snprintf(note_buf, sizeof(note_buf), "%s", notes[note]);
		snprintf(a4_buf, sizeof(a4_buf), "a4: %d", a4);
	}

   /*
	//determine which note frequency is closest to
	if((f-c) <= (r-f)) { //closer to left note
		WriteString("N:");
		WriteString(notes[note]);
		WriteInt(oct);
		WriteString(" D:+");
		WriteInt((int)(f-c+.5));
		WriteString("Hz");
	}
	else { //f closer to right note
		note++;
		if(note >=12) note=0;
		WriteString("N:");
		WriteString(notes[note]);
		WriteInt(oct);
		WriteString(" D:-");
		WriteInt((int)(r-f+.5));
		WriteString("Hz");
	}
   */
}
