#pragma once
/*
 *      Copyright (C) 2015 Jean-Luc Barriere
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation; either version 3, or (at your option)
 *  any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this library; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "mutex.h"
#include "condition.h"

#ifdef NSROOT
namespace NSROOT {
#endif
namespace OS
{

  class Thread
  {
  public:
    Thread()
    : m_finalizeOnStop(false)
    , m_handle(new Handle()) { }

    virtual ~Thread()
    {
      delete m_handle;
    }

    Thread(const Thread& _thread)
    {
      this->m_handle = new Handle();
      this->m_finalizeOnStop = _thread.m_finalizeOnStop;
    }

    Thread& operator=(const Thread& _thread)
    {
      if (this != &_thread)
      {
        delete this->m_handle;
        this->m_handle = new Handle();
        this->m_finalizeOnStop = _thread.m_finalizeOnStop;
      }
      return *this;
    }

    thread_t* native_handle()
    {
      return &(m_handle->nativeHandle);
    }

    bool start_thread(bool wait = true)
    {
      LockGuard lock(m_handle->mutex);
      if (!m_handle->running)
      {
        m_handle->notifiedStop = false;
        if (thread_create(&(m_handle->nativeHandle), Thread::ThreadHandler, ((void*)static_cast<Thread*>(this))))
        {
          if (wait)
            m_handle->condition.wait(m_handle->mutex, m_handle->running);
          return true;
        }
      }
      return false;
    }

    void stop_thread(bool wait = true)
    {
      // First signal stop
      {
        LockGuard lock(m_handle->mutex);
        m_handle->notifiedStop = true;
        m_handle->condition.notify_all();
      }
      // Waiting stopped
      if (wait)
      {
        LockGuard lock(m_handle->mutex);
        m_handle->condition.wait(m_handle->mutex, m_handle->stopped);
      }
    }

    bool wait_thread(unsigned millisec)
    {
      LockGuard lock(m_handle->mutex);
      return m_handle->stopped ? true : m_handle->condition.wait_for(m_handle->mutex, millisec, m_handle->stopped);
    }

    bool is_running()
    {
      LockGuard lock(m_handle->mutex);
      return m_handle->running;
    }

    bool is_stopped()
    {
      LockGuard lock(m_handle->mutex);
      return m_handle->notifiedStop || m_handle->stopped;
    }

    void pause(unsigned millisec)
    {
      Timeout _timeout(millisec);
      LockGuard lock(m_handle->mutex);
      while (!m_handle->notifiedStop && !m_handle->notifiedWake && m_handle->condition.wait_for(m_handle->mutex, _timeout));
      m_handle->notifiedWake = false; // Reset the wake flag
    }

    void wake()
    {
      LockGuard lock(m_handle->mutex);
      m_handle->notifiedWake = true;
      m_handle->condition.notify_all();
    }

  protected:
    virtual void* process(void) = 0;
    virtual void finalize(void) { };
    bool m_finalizeOnStop;

  private:
    struct Handle
    {
      thread_t      nativeHandle;
      volatile bool running;
      volatile bool stopped;
      volatile bool notifiedStop;
      volatile bool notifiedWake;
      Condition<volatile bool> condition;
      Mutex         mutex;

      Handle()
      : nativeHandle(0)
      , running(false)
      , stopped(true)
      , notifiedStop(false)
      , notifiedWake(false)
      , condition()
      , mutex() { }
    };

    Handle* m_handle;

    static void* ThreadHandler(void* _thread)
    {
      Thread* thread = static_cast<Thread*>(_thread);
      void* ret = nullptr;

      if (thread)
      {
        bool finalize = thread->m_finalizeOnStop;
        thread->m_handle->mutex.lock();
        thread->m_handle->running = true;
        thread->m_handle->stopped = false;
        thread->m_handle->condition.notify_all();
        thread->m_handle->mutex.unlock();
        ret = thread->process();
        thread->m_handle->mutex.lock();
        thread->m_handle->running = false;
        thread->m_handle->stopped = true;
        thread->m_handle->condition.notify_all();
        thread->m_handle->mutex.unlock();

        // Thread without finalizer could be freed here
        if (finalize)
          thread->finalize();
      }

      return ret;
    }

  };

}
#ifdef NSROOT
}
#endif
