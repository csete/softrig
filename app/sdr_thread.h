/*
 * Main SDR sequencer thread
 */
#pragma once

#include <QObject>
#include <QThread>

#include "interfaces/audio_output.h"
#include "nanosdr/common/datatypes.h"
#include "nanosdr/common/sdr_data.h"
#include "nanosdr/common/time.h"
#include "nanosdr/interfaces/sdr_device.h"
#include "nanosdr/receiver.h"

// error codes
#define SDR_THREAD_OK       0
#define SDR_THREAD_ERROR   -1  // unspecified error
#define SDR_THREAD_EDEV    -2  // device error

class SdrThread : public QObject
{
    Q_OBJECT

public:
    explicit SdrThread(QObject *parent = 0);
    virtual ~SdrThread();

    int     start(void);
    void    stop(void);
    bool    isRunning(void) const
    {
        return is_running;
    }

public slots:
    void    setRxFrequency(qint64 freq);
    void    setDemod(sdr_demod_t);
    void    setRxFilter(real_t, real_t);
    void    setRxCwOffset(real_t);

private slots:
    void    process(void);
    void    thread_finished(void);

private:
    void    resetStats(void);

private:
    QThread       *thread;
    SdrDevice     *sdr_dev;
    Receiver      *rx;
    AudioOutput    audio_out;

    bool           is_running;
    quint32        buflen;

    complex_t     *input_samples;  // sample buffer for IQ input
    real_t        *output_samples; // sample buffer for audio output
    qint16        *aout_buffer; // audio output buffer

    struct {
        uint64_t    tstart;
        uint64_t    tstop;
        uint64_t    samples_in;
        uint64_t    samples_out;
    } stats;
};
