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

#include "thread.h"
#include "event.h"

// Compatibility with C++98 remains
#include <cstddef> // for NULL
#include <queue>
#include <set>

#ifdef NSROOT
namespace NSROOT {
#endif
namespace OS
{

  class Worker;

  class WorkerThread;

  class ThreadPool
  {
    friend class WorkerThread;
  public:
    ThreadPool();
    ThreadPool(unsigned size);
    ~ThreadPool();

    bool Enqueue(Worker* worker);

    unsigned GetMaxSize() const { return m_size; }

    void SetMaxSize(unsigned size);

    void SetKeepAlive(unsigned millisec);

    unsigned Size() const;

    unsigned QueueSize() const;
    bool IsQueueEmpty() const;
    bool waitEmpty(unsigned millisec);
    bool waitEmpty();

    void Suspend();
    void Resume();
    bool IsSuspended() const;

    void Reset();
    void Stop();
    void Start();
    bool IsStopped() const;

  private:
    unsigned      m_size;
    unsigned      m_keepAlive;
    unsigned      m_poolSize;
    unsigned      m_waitingCount;
    volatile bool m_stopped;
    volatile bool m_suspended;
    volatile bool m_empty;

    std::queue<Worker*>       m_queue;
    std::set<WorkerThread*>   m_pool;
    mutable Mutex             m_mutex;
    Condition<volatile bool>  m_condition;
    Event                     m_queueFill;
    Event                     m_queueEmpty;

    Worker* PopQueue(WorkerThread* _thread);
    void WaitQueue(WorkerThread* _thread);
    void StartThread(WorkerThread* _thread);
    void FinalizeThread(WorkerThread* _thread);
    void __resize();
  };

  class Worker
  {
    friend class ThreadPool;
  public:
    Worker() : m_queued(false) { }
    virtual ~Worker() { }
    virtual void Process() = 0;

  private:
    bool m_queued;
  };

  class WorkerThread : public Thread
  {
  public:
    WorkerThread(ThreadPool& pool)
    : Thread()
    , m_threadPool(pool) { m_finalizeOnStop = true; }

    void* Process(void)
    {
      bool waiting = false;

      while (!IsStopped())
      {
        Worker* worker = m_threadPool.PopQueue(this);
        if (worker != NULL)
        {
          worker->Process();
          delete worker;
          waiting = false;
        }
        else if (!waiting)
        {
          m_threadPool.WaitQueue(this);
          waiting = true;
        }
        else
          break;
      }

      return NULL;
    }

    void Finalize(void)
    {
      m_threadPool.FinalizeThread(this);
    }

  private:
    ThreadPool& m_threadPool;
  };

}
#ifdef NSROOT
}
#endif
