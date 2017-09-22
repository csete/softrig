/*
 * SDR-IQ driver for nanosdr.
 */
#pragma once


#include <stdint.h>

#define SDRIQ_STATE_IDLE   0x01
#define SDRIQ_STATE_RUN    0x02

struct  _sdriq;
typedef struct _sdriq sdriq_t;

#ifdef __cplusplus
extern "C"
{
#endif


sdriq_t *sdriq_new(void);
void sdriq_free(sdriq_t * sdr);

int sdriq_open(sdriq_t * sdr);
int sdriq_close(sdriq_t * sdr);

int sdriq_start(sdriq_t * sdr);
int sdriq_stop(sdriq_t * sdr);
int sdriq_set_state(sdriq_t * sdr, unsigned char state);

int sdriq_set_freq(sdriq_t * sdr, uint32_t freq);
int sdriq_set_sample_rate(sdriq_t * sdr, uint32_t rate);
uint32_t sdriq_get_sample_rate(const sdriq_t * sdr);

int sdriq_set_fixed_rf_gain(sdriq_t * sdr, int8_t gain);
int sdriq_set_fixed_if_gain(sdriq_t * sdr, int8_t gain);

int sdriq_set_input_rate(sdriq_t * sdr, uint32_t rate);

uint_fast32_t sdriq_get_num_samples(const sdriq_t * sdr);
uint_fast32_t sdriq_get_samples(sdriq_t * sdr,
                                unsigned char * buffer,
                                uint_fast32_t num);

#ifdef __cplusplus
}
#endif

