/*
 * Fractional resampler using windowed sinc interpolation
 *
 * Originally from CuteSdr, modified for nanosdr.
 */
#pragma once

#include "common/datatypes.h"

class FractResampler  
{
public:
	FractResampler();
	virtual ~FractResampler();

	void    init(int max_input);
	int     resample(int input_length, real_t rate, real_t * inbuf, real_t * outbuf);
	int     resample(int input_length, real_t rate, complex_t * inbuf, complex_t * outbuf);

private:
	real_t      float_time;     // output time accumulator
	real_t     *sinc_table;     // pointer to sinc table
	complex_t  *input_buffer;   // sample buffer

    int         max_input_length;
};
