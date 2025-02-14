/*
This function finds and prints to the lcd the closest musical note and octave corresponding to the 
input frequency. The Frequency is a floating point value and cannot be less than zero. This function also prints the 
distance in hertz between the frequency and the closest note.

The note is found by starting at middle C. Once the octave of the note is determined, the note is found by iterating
through the note frequencies, given the relation that the next higher or lower note is a factor of the twelfth root of 2 away.
An array containing the names of the notes is used instead of a switch statement. It saves code space and time by using a look up table instead of a case statement.

This function can possibly be improved by using an equation to model the relation between the notes 
instead of iterating through each note. This requires a log function.

Input
	f - the frequency
Returns 
	nothing 
*/

#ifndef NOTE_H
#define NOTE_H 

#define root2 1.0594631 //twelfth root of 2
#define INV_8_176 0.122312205
void findNote(/*float f*/);
extern int a4;
extern char note_buf[4];
extern char octave[2];
extern char freq_buf[7];
extern char cents_buf[6];
extern char a4_buf[8];
extern int cents_err; // Global cent variable
//extern volatile int octave;

#endif
