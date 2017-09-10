/*
 * Main SDR sequencer thread
 */
#pragma once

#include <QObject>
#include <QThread>

class SdrThread : public QObject
{
    Q_OBJECT

public:
    explicit SdrThread(QObject *parent = 0);
    virtual ~SdrThread();

private:
    QThread   *this_thread;
};
