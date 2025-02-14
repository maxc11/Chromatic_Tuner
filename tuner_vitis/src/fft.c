#include "fft.h"
#include "complex.h"
#include "stream_grabber.h"
//#include "trig.h"
#include <math.h>
#include "lcd.h"
#include "stdint.h"
/*

FFT NOTES

run fft twice. once to determine octave (128 pt sample)
second fft should be smaller? or same size but only concerning octave range

implement kissfft with the existing code (fixed point to be super fast) otherwise floatingpoint 

decimate depending on the initial fft. should increase decimation the higher the frequency

64fft first then 128fft. 

*/

#define BASE_FS 48828.125

static float new_[512];
static float new_im[512];

static float s[513][513];
static float c[513][513]; // LUT
float sample_freqs[6];


// for fft
#define SAMPLES 512 // AXI4 Streaming Data FIFO has size 512
//#define M 8 // 2^m=samples
int sample_l2 = 9;
//#define CLOCK 100000000.0 // clock speed
int64_t int_buffer[SAMPLES];
//static float q[SAMPLES];
kiss_fft_scalar q[SAMPLES];
//static float w[SAMPLES];
extern float sample_freqs[6];
volatile float frequency = 0;
uint16_t decimation = 0;
kiss_fftr_cfg cfg = NULL;
kiss_fft_cpx out[SAMPLES];

/*
 * variables for bar calculations
 */
int len_new_ = 0;
int total = 0;
int bins[8];
int VPB;
int temp = 0;
int static_new_[512];
int BOT[10][32];
int temp_arr[32];

void init_lut(void) {
	for (int b = 1; b <= 512; b <<= 1) {
		float recip = 1.0f / b;
		for (int k = 0; k < b; ++k) {
			float angle = -PI * k * recip;
			s[b][k] = sin(angle);
			c[b][k] = cos(angle);
		}
	}
}

void init_fs_values(void) {
	float fs = 48828.125f;
//	xil_printf("Test: %d", fs);
	for (int i = 0; i < 6; ++i) {
		sample_freqs[i] = fs / pow(2, i);
//		xil_printf("nombre %d\r\n", (int)sample_freqs[i]);
	}
}

static inline float get_sin(int b, int k) {
	return s[b][k];
}

static inline float get_cos(int b, int k) {
	return c[b][k];
}

void read_fsl_values(kiss_fft_scalar *q, int decimation, int n) {
    int i, mean;
    unsigned int x;
    stream_grabber_start(); // move stream grabber start to be periodically triggered
    stream_grabber_wait_enough_samples(4096); // this shouldn't be here...
    // reading every other value from stream to build 256 size arr from 512 (decimating)s
    int grab = 0;
    for(i = 0; i < n; i++) {
        int_buffer[i] = stream_grabber_read_sample(grab);
        // fill up normally
//        q[i] = int_buffer[i];
        x = int_buffer[i];
        q[i] = (x * 13) >> 28;
        mean += q[i];
        grab += decimation;
    }
    mean /= 256;
    for (i = 0; i < n; i++) {
    	q[i] -= mean;
    }
}

void fft_init() {
//	stream_grabber_start();
	cfg = kiss_fftr_alloc(256, 0, NULL, NULL);
	xil_printf("init alloc area for fftr \r\n");
	if (cfg == NULL) {
		xil_printf("err allocating cfg \r\n");
	}
}

void fft_free() {
	free(cfg);
}

float fft(kiss_fftr_cfg cfg, int size, int log2_size, float sample_freq) {
	len_new_ = 0;
	float freq;
	kiss_fftr(cfg, q, out);
	int max = 0;
	int place = 1;
	for (int i = 1; i < (size / 2) + 1; i++) {
		new_[i] = out[i].r * out[i].r + out[i].i * out[i].i;
		len_new_++;
		//xil_printf("Length New:%d \r\n", len_new_);
		if (max < new_[i]) {
			max = new_[i];
			place = i;
		}

	}
	float s= sample_freq / size; //spacing of bins

	freq = s * place;

	//curve fitting for more accuarcy
	//assumes parabolic shape and uses three point to find the shift in the parabola
	//using the equation y=A(x-x0)^2+C
	float y1=new_[place-1],y2=new_[place],y3=new_[place+1];
	float x0=s+(2*s*(y2-y1))/(2*y2-y1-y3);
	x0=x0/s-1;

	if(x0 <0 || x0 > 2) { //error
//		xil_printf("err occurred interpolating %d\r\n", (int)(freq + 0.5));
		return 0;
	}
	if(x0 <= 1)  {
		freq=freq-(1-x0)*s;
	}
	else {
		freq=freq+(x0-1)*s;
	}

	return freq;
}

void run_fft(void) {
	float sample_f = 48828.125;// 3051.7578125 24414.0625, 48828.125;
	uint16_t sample_size = 256;
	sample_l2 = 8;
//    xil_printf("this works! \r\n");
	decimation = 2;
	read_fsl_values(q, decimation, sample_size); // decimate by 2 at first (256pt from 512)
	frequency = fft(cfg, sample_size, sample_l2, sample_f / 2);
//	xil_printf("256pt frequency: %d Hz\r\n", (int) (frequency + 0.5));
	// averaging is important for windowing, also important for values

	if (frequency < 1525) {
		decimation = 16;
		sample_l2 = 8;
		sample_f = sample_f / decimation;
		sample_size = 256;
	} else if (frequency < 3050) {
		decimation = 8;
		sample_l2 = 8;
		sample_f = 6103.515625;
		sample_size = 256;
	} else if (frequency < 6104) {
		decimation = 4;
		sample_l2 = 8;
		sample_f = 12207.03125;
		sample_size = 256;
	} else {
		decimation = 2;
		sample_l2 = 8;
		sample_f = 24414.0625;
		sample_size = 256;
	}
	read_fsl_values(q, decimation, sample_size);
	frequency = fft(cfg, sample_size, sample_l2, sample_f);
	//xil_printf("decimated frequency: %d Hz\r\n", (int) (frequency + 0.5));
//	return frequency;
}

int* calc_bins(void) {
    static int bins[32];// Static to return the array
    // store to global 2d array,
    int total = 0;
    int counter = 0;
    int temp = 0;
    int VPB = 4;
    // Process bins
    for (int i = 0; i < 128; i++) {
        total += new_[i];
        counter++;
        if (counter == VPB) { // Bin is full
            bins[temp] = total<<2;
     //  xil_printf("Total in bin %d: %d\r\n", i, total );
            total = 0;         // Reset total for the next bin
            temp++;
            counter = 0;       // Reset counter

    	//	xil_printf("Bin 1: %d\r\n", bins[7]);
        }

    }

	return bins;
}


void grid(void){
	setColor(255, 255, 255);//black!
	fillRect(20, 75, 22, 275); //y axis

	for(int i =0; i<17; i++) {
		fillRect(20, 275.00-i*10, 30, 274.00-i*10);
	}
}
// 	fillRect(20, 75, 220, 275); background specs
void spect(void) {
    int *temp = calc_bins(); // Load recent bins
    int highest = 0;

    // Draw white background
    setColor(0, 0, 0);
    fillRect(20, 75, 220, 275);

    /*
     * Rescale bin values on log2 and find max value
     */
    for (int i = 0; i < 32; i++) {
        int value = log2(temp[i] + 1) * 10; // Avoid log2(0) by adding 1
        if (value > highest) highest = value;
        temp[i] = value;
    }

    // Debug print for max value
//    xil_printf("Max value in bin: %d\r\n", highest);

    // Draw rectangles with corresponding colors
    for (int i = 0; i < 32; i++) {
        // Normalize bin value for color mapping
        int normalized = (temp[i] * 255) / highest; // Normalize to 0-255 range

        // Assign RGB values based on normalized value (gradient)
        int r = 0, g = 0, b = 0;
        if (normalized <= 127) {
            // Transition from blue to green
            r = 0;
            g = normalized * 2;   // Gradual increase
            b = 255 - g;          // Gradual decrease
        } else {
            // Transition from green to red
            r = (normalized - 127) * 2; // Gradual increase
            g = 255 - r;                // Gradual decrease
            b = 0;
        }

        // Set the color for the current bin
        setColor(r, g, b);

        // Calculate rectangle positions with boundaries
        int y1 = 275 - temp[i] + 3;
        int y2 = 275 - temp[i];

        // Apply boundary logic
        if (y1 > 275) y1 = 275;  // Clamp bottom boundary
        if (y2 > 275) y2 = 275;
        if (y1 < 75) y1 = 75;    // Clamp top boundary
        if (y2 < 75) y2 = 75;

        // Ensure valid rectangle (y1 >= y2 to avoid inverted rectangles)
        if (y1 < y2) {
            int temp = y1;
            y1 = y2;
            y2 = temp;
        }

        // Draw the rectangle for the bin
        fillRect(20, y1, 220, y2);
    }
}

//


// LOGIC FOR MAX/MIN SCALED BIN LCD VALUE
// Value <75
//Value >275


// get max value or static value
// convert bins to colors
// get bins on the screen according to color


/*
 * LCD screen:
 *
 */

// LOG2 scale for y axis
// return max bar height
int bars(void) {
	int max = 0;
	int *b = calc_bins();
	//xil_printf("Proof im not crazy: %d\r\n", b[2]);
//	for(int i =0; i<30; i++){
//		xil_printf("Bin %d: %d \r\n",i, b[i]);
//	}
	for(int i =0; i<32; i++) {
		int scaled = log2(b[i]) * 10;
		if (scaled > max) max = scaled;
		b[i]= scaled;
	}

	setColor(43, 255, 177);//green!
// 4 pixels wide with 1 pixel space inbetween
//	xil_printf("Value: %f \r\n", b[0]);

	fillRect(36, 275, 40, 275-b[0]);    // i=0
	fillRect(42, 275, 46, 275-b[1]);    // i=1
	fillRect(48, 275, 52, 275-b[2]);    // i=2
	fillRect(54, 275, 58, 275-b[3]);    // i=3
	fillRect(60, 275, 64, 275-b[4]);    // i=4
	fillRect(66, 275, 70, 275-b[5]);    // i=5
	fillRect(72, 275, 76, 275-b[6]);    // i=6
	fillRect(78, 275, 82, 275-b[7]);    // i=7
	fillRect(84, 275, 88, 275-b[8]);    // i=8
	fillRect(90, 275, 94, 275-b[9]);    // i=9
	fillRect(96, 275, 100, 275-b[10]);  // i=10
	fillRect(102, 275, 106, 275-b[11]); // i=11
	fillRect(108, 275, 112, 275-b[12]); // i=12
	fillRect(114, 275, 118, 275-b[13]); // i=13
	fillRect(120, 275, 124, 275-b[14]); // i=14
	fillRect(126, 275, 130, 275-b[15]); // i=15
	fillRect(132, 275, 136, 275-b[16]); // i=16
	fillRect(138, 275, 142, 275-b[17]); // i=17
	fillRect(144, 275, 148, 275-b[18]); // i=18
	fillRect(150, 275, 154, 275-b[19]); // i=19
	fillRect(156, 275, 160, 275-b[20]); // i=20
	fillRect(162, 275, 166, 275-b[21]); // i=21
	fillRect(168, 275, 172, 275-b[22]); // i=22
	fillRect(174, 275, 178, 275-b[23]); // i=23
	fillRect(180, 275, 184, 275-b[24]); // i=24
	fillRect(186, 275, 190, 275-b[25]); // i=25
	fillRect(192, 275, 196, 275-b[26]); // i=26
	fillRect(198, 275, 202, 275-b[27]); // i=27
	fillRect(204, 275, 208, 275-b[28]); // i=28
	fillRect(210, 275, 214, 275-b[29]); // i=29
	fillRect(216, 275, 220, 275-b[30]); // i=30

	return 275 - max;
}




float kfft(float* real, int size, int log2_size, float sample_freq) {
	float frequency;
	kiss_fft_cpx in[size], out[size];
	for (int i = 0; i < size; ++i) {
		in[i].r = real[i];
		in[i].i = 0;
	}
// this doesnt work alloc before running fft then free at end of program
	kiss_fft_cfg cfg;
	if ((cfg = kiss_fft_alloc(size, 0, NULL, NULL)) != NULL) {
		kiss_fft(cfg, in, out);
		free(cfg);
	}

	int max = 0;
	int place = 1;
	for (int i = 1; i < (size / 2) + 1; ++i) {
		new_[i] = out[i].r * out[i].r + out[i].i * out[i].i;
		if (max < new_[i]) {
			max = new_[i];
			place = i;
		}
	}
//perhaps we calcualte bins[] here


	float s= sample_freq / size; //spacing of bins

	frequency = s * place;

	//curve fitting for more accuarcy
	//assumes parabolic shape and uses three point to find the shift in the parabola
	//using the equation y=A(x-x0)^2+C
	float y1=new_[place-1],y2=new_[place],y3=new_[place+1];
	float x0=s+(2*s*(y2-y1))/(2*y2-y1-y3);
	x0=x0/s-1;

	if(x0 <0 || x0 > 2) { //error
		return 0;
	}
	if(x0 <= 1)  {
		frequency=frequency-(1-x0)*s;
	}
	else {
		frequency=frequency+(x0-1)*s;
	}
	return frequency;
}



/*
 *  USED KFFT :D
 */


// old fft code
float old_fft(float* real_part, float* imag_part, int size, int log2_size, float sample_freq) {
	int half_size,butterfly_count,r,d,reorder_idx,c;
	int k,place;
	half_size=size/2;
	butterfly_count=1;
	int i,j;
	float real=0,imagine=0;
	float max,frequency;

	// ORdering algorithm
	for(i=0; i<(log2_size-1); i++){
		d=0;
		for (j=0; j<butterfly_count; j++){
			for (c=0; c<half_size; c++){
				reorder_idx=c+d;
				new_[reorder_idx]=real_part[(c*2)+d];
				new_im[reorder_idx]=imag_part[(c*2)+d];
				new_[reorder_idx+half_size]=real_part[2*c+1+d];
				new_im[reorder_idx+half_size]=imag_part[2*c+1+d];
			}
			d+=(size/butterfly_count);
		}
		for (r=0; r<size;r++){
			real_part[r]=new_[r];
			imag_part[r]=new_im[r];
		}
		butterfly_count*=2;
		half_size=size/(2*butterfly_count);
	}
	//end ordering algorithm

	butterfly_count=1;
	k=0;
	for (j=0; j<log2_size; j++){
	//MATH
		for(i=0; i<size; i+=2){
			if (i%(size/butterfly_count)==0 && i!=0) k++;
	//			real=mult_real(q[i+1], w[i+1], cosine(-PI*k/b), sine(-PI*k/b));
	//			imagine=mult_im(q[i+1], w[i+1], cosine(-PI*k/b), sine(-PI*k/b));
			real=mult_real(real_part[i+1], imag_part[i+1], get_cos(butterfly_count, k), get_sin(butterfly_count, k));
			imagine=mult_im(real_part[i+1], imag_part[i+1], get_cos(butterfly_count, k), get_sin(butterfly_count, k));
			new_[i]=real_part[i]+real;
			new_im[i]=imag_part[i]+imagine;
			new_[i+1]=real_part[i]-real;
			new_im[i+1]=imag_part[i]-imagine;
		}
		for (i=0; i<size; i++){
			real_part[i]=new_[i];
			imag_part[i]=new_im[i];
		}
	//END MATH

	//REORDER
		for (i=0; i<size/2; i++){
			new_[i]=real_part[2*i];
			new_[i+(size/2)]=real_part[2*i+1];
			new_im[i]=imag_part[2*i];
			new_im[i+(size/2)]=imag_part[2*i+1];
		}
		for (i=0; i<size; i++){
			real_part[i]=new_[i];
			imag_part[i]=new_im[i];
		}
	//END REORDER
		butterfly_count*=2;
		k=0;
	}

	//find magnitudes
	max=0;
	place=1;
	for(i=1;i<(size/2);i++) {
		new_[i]=real_part[i]*real_part[i]+imag_part[i]*imag_part[i];
		if(max < new_[i]) {
			max=new_[i];
			place=i;
		}
	}

	float s=sample_freq/size; //spacing of bins

	frequency = s * place;

	//curve fitting for more accuarcy
	//assumes parabolic shape and uses three point to find the shift in the parabola
	//using the equation y=A(x-x0)^2+C
	float y1=new_[place-1],y2=new_[place],y3=new_[place+1];
	float x0=s+(2*s*(y2-y1))/(2*y2-y1-y3);
	x0=x0/s-1;

	if(x0 <0 || x0 > 2) { //error
		return 0;
	}
	if(x0 <= 1)  {
		frequency=frequency-(1-x0)*s;
	}
	else {
		frequency=frequency+(x0-1)*s;
	}

	return frequency;
}
