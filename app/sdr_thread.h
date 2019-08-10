/*
 * Main SDR sequencer thread
 */
#pragma once

#include <QObject>
#include <QThread>

#include "app_config.h"
#include "interfaces/audio_output.h"
#include "interfaces/sdr/sdr_device.h"
#include "nanosdr/common/datatypes.h"
#include "nanosdr/common/sdr_data.h"
#include "nanosdr/common/time.h"
#include "nanosdr/fft_thread.h"
#include "nanosdr/nanodsp/filter/decimator.h"
#include "nanosdr/receiver.h"


// error codes
#define SDR_THREAD_OK       0
#define SDR_THREAD_ERROR   -1  // unspecified error
#define SDR_THREAD_EDEV    -2  // device error

#define FFT_SIZE 2*8192  // FIXME

class SdrThread : public QObject
{
    Q_OBJECT

public:
    explicit SdrThread(QObject *parent = nullptr);
    virtual ~SdrThread();

    int     start(const app_config_t * conf, SdrDevice *dev);
    void    stop(void);
    bool    isRunning(void) const
    {
        return is_running;
    }

    quint32 getFftData(real_t *);
    float   getSignalStrength(void);

public slots:
    void    setRxFrequency(quint64 freq);
    void    setDemod(sdr_demod_t);
    void    setRxFilter(real_t, real_t);
    void    setRxTuningOffset(real_t offset);
    void    setRxCwOffset(real_t);

private slots:
    void    process(void);
    void    thread_finished(void);

private:
    void    resetStats(void);

private:
    QThread       *thread;
    SdrDevice     *device;
    FftThread     *fft;
    Receiver      *rx;
    Decimator      input_decim;
    AudioOutput    audio_out;

    bool           is_running;
    quint32        buflen;          // buffer size in samples
    unsigned int   buflen_ms;       // buffer size in milliseconds
    unsigned int   decimation;

    complex_t     *fft_data_buf;    // FFT samples from FFT thread
    complex_t     *fft_swap_buf;    // Buffer for swapping FFT
    complex_t     *input_samples;   // sample buffer for IQ input
    real_t        *output_samples;  // sample buffer for audio output
    qint16        *aout_buffer;     // audio output buffer

    struct {
        uint64_t    tstart;
        uint64_t    tstop;
        uint64_t    samples_in;
        uint64_t    samples_out;
    } stats;
};
