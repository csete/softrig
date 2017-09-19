/*
 * Audio output interface
 */
#pragma once

#include <QAudioOutput>
#include <QObject>

#define AUDIO_OUT_OK        0
#define AUDIO_OUT_ERROR     1

class AudioOutput : public QObject
{
    Q_OBJECT

public:
    explicit AudioOutput(QObject *parent = 0);
    virtual ~AudioOutput();

    int    init(void);

private:
    bool            initialized;
    QAudioOutput   *audio_out;
};
