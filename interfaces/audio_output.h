/*
 * Audio output interface
 */
#pragma once

#include <QAudioOutput>
#include <QIODevice>
#include <QObject>

#define AUDIO_OUT_OK        0
#define AUDIO_OUT_ERROR    -1
#define AUDIO_OUT_EFORMAT  -2
#define AUDIO_OUT_EINIT    -3
#define AUDIO_OUT_EBUFWR   -4   // Error writing to buffer

class AudioOutput : public QObject
{
    Q_OBJECT

public:
    explicit AudioOutput(QObject *parent = 0);
    virtual ~AudioOutput();

    int     init(void);
    int     start(void);
    int     stop(void);
    int     write(const char * data, qint64 len);

private slots:
    void    aoutStateChanged(QAudio::State);

private:
    bool            initialized;
    QAudioOutput   *audio_out;
    QIODevice      *audio_buffer;
};
