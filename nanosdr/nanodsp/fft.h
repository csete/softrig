/*
 * FFT class for nanosdr.
 */
#pragma once

#include <stdint.h>

#include "common/datatypes.h"
#include "common/ring_buffer_cplx.h"
#include "kiss_fft.h"

#define FFT_MIN_SIZE    128
#define FFT_MAX_SIZE    32768


class CFft
{
public:
    CFft();
    virtual    ~CFft();

    /**
     * Initialise FFT.
     * @param   size    The FFT size.
     * @retval   0      FFT init was successful.
     * @retval  -1      FFT size out of valid range.
     * @retval  -2      Error initializing FFT engine.
     */
    int         init(uint32_t size);

    /**
     * Add input samples.
     * @param num   The number of elements in @ref data.
     * @param inbuf Pointer to the data.
     */
    void        add_input_samples(uint32_t num, complex_t * inbuf);

    /**
     * Get FFT output
     * @param outbuf Pointer to the output buffer where FFT data should be
     *               copied. The buffer must be able to hold @ref fft_size
     *               number of elements.
     * @return The number of samples copied into @ref data. This is always
     *         either @ref fft_size or 0 if there isn't sufficient data in the
     *         input_buffer to run an FFT.
     */
    uint32_t    get_output_samples(complex_t * outbuf);

    void        process(complex_t * input, complex_t * output);

private:
    kiss_fft_cfg    fft_cfg;
    uint32_t        fft_size;
    real_t         *fft_window;

    complex_t      *fft_work_buffer;
    ring_buffer_t  *fft_input_buffer;

    void            free_memory();
};

