/*
 * AM demodulator.
 *
 * Originally from CuteSdr and modified for nanosdr.
 *
 */
#pragma once

#include "common/datatypes.h"
#include "fir.h"

class AmDemod
{
public:
    AmDemod();

    /**
     * Setup demodulator parameters
     * @param input rate    The sample rate of the input data
     * @param bandwidth     Bandwidth of the post-demod audio filter.
     */
	void        setup(real_t input_rate, real_t bandwidth);
	int         process(int num, complex_t * data_in, real_t * data_out);

private:
	Fir         audio_filter;
	real_t      sample_rate;
	real_t      z1;
};
