/*
 * Main SDR sequencer thread
 */
#pragma once

#include <QObject>
#include <QThread>

// error codes
#define SDR_THREAD_OK       0
#define SDR_THREAD_ERROR    1

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

private slots:
    void    process(void);
    void    thread_finished(void);

private:
    QThread   *thread;

    bool       is_running;
};
