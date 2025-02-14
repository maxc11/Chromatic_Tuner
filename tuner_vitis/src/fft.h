/*
This function calculates the Fourier Transform of the sampled input using the Cooley-Tukey algorithm.
before this function is entered, 
	the static array q must be filled with the floating point values of the sampled input;
	w must be a static zero filled array of size samples;
	new_ and new_im must be static arrays of size samples;
	the functions mult_re, mult_im, sine and cosine must be defined;
Inputs
	q - the real part of the sampled input
	w - zero filled array used for the imaginary part of the input
	n - the number of samples
	m - the power of 2 that equals n
	f_sample - the sampling frequency
after the function has completed,
	q will contain the real parts of the Fourier Transformed values of the input;
	w will contain the imaginary parts of the Fourier Transformed values of the input;
	the begining half of new_ will contain the squared magnitudes of the first half of the output;
	place will contain the bin number containing the greatest squared magnitude;
	max will contain the value of the largest squared magnitude;
Returns
	frequency - the frequency of the input
	
*/

#ifndef FFT_H
#define FFT_H

#define PI 3.141592//65358979323846
#include "kiss_fftr.h"

float old_fft(float* q, float* w, int n, int m, float sample_f);
float kfft(float* q, int n, int m, float sample_f);
float fft(kiss_fftr_cfg cfg, int n, int m, float sample_f);
void read_fsl_values(kiss_fft_scalar *q, int decimation, int n);
void init_lut(void);
void init_fs_values(void);
void fft_init(void);
void fft_free(void);
void spect(void);

extern int BOT[10][32];

#endif
