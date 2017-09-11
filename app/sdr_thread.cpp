/*
 * Main SDR sequencer thread
 *
 * This file is part of softrig, a simple software defined radio transceiver.
 *
 * Copyright 2017 Alexandru Csete OZ9AEC.
 * All rights reserved.
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
#include <QThread>

#include "sdr_thread.h"

SdrThread::SdrThread(QObject *parent) : QObject(parent)
{
    is_running = false;

    thread = new QThread();
    moveToThread(thread);
    connect(thread, SIGNAL(started()), this, SLOT(process()));
    connect(thread, SIGNAL(finished()), this, SLOT(thread_finished()));
    thread->setObjectName("SoftrigSdrThread");
    thread->start();
}

SdrThread::~SdrThread()
{
    if (is_running)
        stop();

    thread->requestInterruption();
    thread->quit();
    thread->wait(10000);
    delete thread;
}

int SdrThread::start(void)
{
    if (is_running)
        return SDR_THREAD_OK;

    qInfo("Starting SDR thread...");
    is_running = true;

    return SDR_THREAD_OK;
}

void SdrThread::stop(void)
{
    if (!is_running)
        return;

    qInfo("Stopping SDR thread...");
    is_running = false;
}

void SdrThread::process(void)
{
    qInfo("SDR process entered");

    while (!thread->isInterruptionRequested())
    {
        if (!is_running)
        {
            QThread::msleep(100);
            continue;
        }

        qInfo("thread func ...");
        QThread::sleep(1);
    }
}

void SdrThread::thread_finished(void)
{
    qInfo("SDR thread finished");
}
