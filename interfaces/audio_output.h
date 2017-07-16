/*
 * Audio output interface
 */
#pragma once

#include <QAudioOutput>
#include <QObject>
#include <QThread>

class AudioOutput : public QObject
{
    Q_OBJECT

public:
    explicit AudioOutput(QObject *parent = 0);

private:
    QAudioOutput   *audio;
    QThread        *this_thread;
};
