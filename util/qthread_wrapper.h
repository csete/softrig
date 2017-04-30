/*
 * qthread_wrapper.h: Utility to make classes run in their own QThread.
 *
 * This file is part of softrig, a simple software defined radio transceiver.
 *
 * Copyright 2013 Moe Wheatley.
 * Copyright 2017 Alexandru Csete OZ9AEC.
 * All rights reserved.
 *
 * This software is released under the "Simplified BSD License".
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once

#include <QObject>
#include <QThread>

/* Convenience wrapper for QThread.
 * This base class is used to derive thread based objects that will have their
 * own event loop running in this worker thread. The initThread() function is
 * called by the worker thread when started to setup all signal-slot connections
 * The exitThread() function is called by the thread after cleanupThread() is
 * called. This can be used in cases where the worker thread must delete its own
 * resources as is the case for network objects.
 */
class QThreadWrapper : public QObject
{
    Q_OBJECT

public:
    QThreadWrapper()
    {
        this_thread = new QThread(this);
        this->moveToThread(this_thread);
        connect(this_thread, SIGNAL(started()), this, SLOT(initThread()));
        connect(this, SIGNAL(exitThreadSignal()), this, SLOT(exitThread()));
        this_thread->start();
    }
    virtual ~QThreadWrapper()
    {
        this_thread->exit();
        this_thread->wait();
        delete this_thread;
    }
    void stopThread()
    {
        emit    exitThreadSignal();
        this_thread->wait(10);
    }

signals:
    void            exitThreadSignal();

public slots:
    // Derived classes must implement these
    virtual void    initThread() = 0; // Called by new thread when it is started
    virtual void    exitThread() = 0; // Called by cleanupThread()

protected:
    QThread   *this_thread;
};
