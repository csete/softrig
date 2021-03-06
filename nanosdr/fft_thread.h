/*
 * FFT thread class
 */
#pragma once

#include <mutex>
#include <stdint.h>

#include "common/datatypes.h"
#include "common/thread_class.h"
#include "nanodsp/fft.h"

struct fft_settings {
    unsigned int    fft_rate;
    uint32_t        fft_size;
};

struct fft_stats {
    uint_fast64_t       samples_in;
    uint_fast64_t       samples_out;
    uint_fast64_t       underruns;
};

class FftThread : public ThreadClass
{
public:
    FftThread();
    virtual ~FftThread();

    /*
     * Inintialize FFT thread
     *   fft_size The number of points in the FFT.
     *   fft_rate The rate in Hz.
     *
     * Returns 0 if FFT initialized without errors, 1 otherwise
     *
     * FIXME: Add more error codes: fft_rate or size out of range, fft_size
     *        not optimal, etc.
     */
    int         init(uint32_t fft_size, unsigned int fft_rate);
    void        start();
    void        stop();

    void        add_fft_input(uint32_t num_samples, complex_t * input_data);
    uint32_t    get_fft_output(complex_t * output_data);

    void        reset_stats();
    void        print_stats();

protected:
    void        thread_func();

private:
    CFft        fft;

    struct fft_settings     settings;
    struct fft_stats        stats;

    std::mutex      buffer_mutex;

    uint_fast64_t   delta_t_ms;
    complex_t      *fft_out;
    bool            have_fft_out;
    bool            running;

};

