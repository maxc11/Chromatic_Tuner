/*
 * stream_grabber.c
 *
 *  Created on: Nov 10, 2017
 *      Author: davidmc
 */

#include <stdint.h>
#include "xparameters.h"
#include "stream_grabber.h"

static volatile uint32_t* const reg_start =
		(uint32_t*)(XPAR_MIC_BLOCK_STREAM_GRABBER_0_BASEADDR);
static volatile uint32_t* const reg_samples_captured =
		(uint32_t*)(XPAR_MIC_BLOCK_STREAM_GRABBER_0_BASEADDR);
static volatile uint32_t* const reg_readout_addr =
		(uint32_t*)(XPAR_MIC_BLOCK_STREAM_GRABBER_0_BASEADDR+4);
static volatile int32_t* const reg_readout_value =
		(int32_t*)(XPAR_MIC_BLOCK_STREAM_GRABBER_0_BASEADDR+8);
//static volatile uint32_t* const reg_seq_counter =
//		(uint32_t*)(XPAR_MIC_BLOCK_STREAM_GRABBER_0_BASEADDR+12);
//static volatile uint32_t* const reg_seq_counter_latched =
//		(uint32_t*)(XPAR_MIC_BLOCK_STREAM_GRABBER_0_BASEADDR+16);



inline void stream_grabber_start() {
	*reg_start = 0; //Value doesn't matter, the write does the magic
}

inline unsigned stream_grabber_samples_sampled_captures() {
	return *reg_samples_captured;
}

inline void stream_grabber_wait_enough_samples(unsigned required_samples){
	while((*reg_samples_captured)<required_samples) {}
}

inline int stream_grabber_read_sample(unsigned which_sample)
{
	*reg_readout_addr = which_sample;
	return *reg_readout_value;
}
