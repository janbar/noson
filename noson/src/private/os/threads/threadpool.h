#pragma once
/*
 *      Copyright (C) 2015-2026 Jean-Luc Barriere
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

#include <queue>

#ifdef NSROOT
namespace NSROOT {
#endif
namespace OS
{

  class Worker;

  class ThreadPool
  {
    class PooledThread;
    friend class PooledThread;
  public:
    ThreadPool();
    ThreadPool(unsigned size);
    ~ThreadPool();

    bool enqueue(Worker* worker);

    unsigned max_size() const { return m_maxSize; }

    void set_max_size(unsigned size);

    void set_keep_alive(unsigned millisec);

    unsigned size() const;

    unsigned queue_size() const;
    bool is_queue_empty() const;
    bool wait_empty();
    bool wait_empty_for(unsigned millisec);

    void suspend();
    void resume();
    bool is_suspended() const;

    void reset();
    void stop();
    void start();
    bool is_stopped() const;

  private:
    unsigned      m_maxSize;
    unsigned      m_keepAlive;
    unsigned      m_poolSize;
    unsigned      m_waitingCount;
    volatile bool m_stopped;
    volatile bool m_suspended;
    volatile bool m_empty;

    PooledThread*             m_pool;
    std::queue<Worker*>       m_queue;
    mutable Mutex             m_mutex;
    Condition<volatile bool>  m_condition;
    Event                     m_queueFill;
    Event                     m_queueEmpty;

    Worker* pop_queue(PooledThread* pt);
    void wait_queue(PooledThread* pt);
    void __resize();
  };

  class Worker
  {
    friend class ThreadPool;
  public:
    Worker() : m_queued(false) { }
    virtual ~Worker() { }
    virtual void process() = 0;

  private:
    bool m_queued;
  };

}
#ifdef NSROOT
}
#endif
