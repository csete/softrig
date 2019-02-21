/*
 * Simple pthread wrapper class
 *
 * Copyright  2015  Alexandru Csete OZ9AEC
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

#include <pthread.h>

/*
 * Base class for enabling threads in classes.
 * See for example nanosdr-server/fft_thread.cpp for how to use it.
 */
class ThreadClass
{
  public:
    ThreadClass()
    {
    }
    virtual ~ThreadClass()
    {
    }

    /*
     * Start the thread.
     * Returns true if the thread was successfully started, false if an error
     * occurred.
     */
    bool start_thread()
    {
        return (pthread_create(&_thread, NULL, thread_func_entry, this) == 0);
    }

    /*
     * Wait for thread to exit. This will happen when thread_func returns.
     * Returns 0 if the thread exited successfully or a non-zero error code
     * otherwise.
     */
    int exit_thread()
    {
        return pthread_join(_thread, NULL);
    }

  protected:
    /*
     * Thread function.
     *
     * This is the thread function that should be implemented by the subclass.
     * If the thread function contains an infinite loop, make sure that it can
     * be exited, e.g.
     *
     *     while (running)
     *     {
     *         do_something();
     *     }
     *     pthread_exit(NULL);
     *
     * then set running=false before calling the exit_thread() method.
     */
    virtual void thread_func() = 0;

  private:
    static void *thread_func_entry(void *_this)
    {
        ((ThreadClass *)_this)->thread_func();
        return NULL;
    }

    pthread_t _thread;
};
