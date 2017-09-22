/*
 * This is a modified version of Takuya OOURA's radix 4 FFT package
 * available at http://www.kurims.kyoto-u.ac.jp/~ooura/fft.html
 * C++ wrapper was added by Moe Weatley.
 * Modified to be included in nanodsp by Alexandru Csete.
 */
#pragma once
#include <stdint.h>
#include "common/datatypes.h"

#define MAX_FFT_SIZE 65536
#define MIN_FFT_SIZE 512

class CuteFft
{
public:
	CuteFft();
	virtual    ~CuteFft();
	void        setup(int32_t size);

	// Methods for doing Fast convolutions using forward and reverse FFT
	void        fwd_fft(complex_t * iobuf);
	void        rev_fft(complex_t * iobuf);

private:
	void        free_memory();
	void        cpx_fft(int32_t n, real_t * a, real_t * w);
	void        makewt(int32_t nw, int32_t * ip, real_t * w);
	void        makect(int32_t nc, int32_t * ip, real_t * c);
	void        bitrv2(int32_t n, int32_t * ip, real_t * a);
	void        cftfsub(int32_t n, real_t * a, real_t * w);
	void        rftfsub(int32_t n, real_t * a, int32_t nc, real_t * c);
	void        cft1st(int32_t n, real_t * a, real_t * w);
	void        cftmdl(int32_t n, int32_t l, real_t * a, real_t * w);
	void        bitrv2conj(int n, int * ip, real_t * a);
	void        cftbsub(int n, real_t * a, real_t * w);

	int32_t     fft_size;
	int32_t     last_fft_size;

    int32_t    *work_area;
    real_t     *sincos_tbl;
    real_t     *fft_in_buf;
};

