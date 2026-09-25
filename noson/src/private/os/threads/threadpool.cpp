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

#include "threadpool.h"

#include <cassert>

#define WTH_KEEPALIVE 5000

#ifdef NSROOT
using namespace NSROOT::OS;
#else
using namespace OS;
#endif

namespace NSROOT
{
namespace OS
{
  class ThreadPool::PooledThread : public Thread
  {
  public:
    static PooledThread * create_thread_unlocked(ThreadPool& pool)
    {
      PooledThread* p = new PooledThread(pool);
      // chain the new thread in the linked list
      if (pool.m_pool)
      {
        p->_old = pool.m_pool;
        pool.m_pool->_new = p;
      }
      pool.m_pool = p;
      // update the pool size
      pool.m_poolSize += 1;

      // enable the implicit finalizer
      p->m_finalizeOnStop = true;

      return p;
    }

    PooledThread* next() { return _old; }

    void* process(void)
    {
      bool waiting = false;

      while (!is_stopped())
      {
        Worker* worker = _pool.pop_queue(this);
        if (worker != nullptr)
        {
          worker->process();
          delete worker;
          waiting = false;
        }
        else if (!waiting)
        {
          _pool.wait_queue(this);
          waiting = true;
        }
        else
          break;
      }

      return nullptr;
    }

    void finalize(void)
    {
      LockGuard lock(_pool.m_mutex);
      // update the pool size
      _pool.m_poolSize -= 1;
      // unchain this from the linked list
      if (_new)
      {
        _new->_old = _old;
        if (_old)
          _old->_new = _new;
      }
      else if (_old)
      {
        // cut the head
        _old->_new = nullptr;
        _pool.m_pool = _old;
      }
      else
      {
        _pool.m_pool = nullptr;
        _pool.m_empty = true;
        _pool.m_condition.notify_all();
      }
      delete this;
    }

  private:
    ThreadPool&     _pool;      /// the owner
    PooledThread*   _old;       /// the link to old
    PooledThread*   _new;       /// the link to new

    PooledThread(ThreadPool& pool)
    : Thread()
    , _pool(pool)
    , _old(nullptr)
    , _new(nullptr)
    { }
  };
} // namespace OS
} // namespace NSROOT


ThreadPool::ThreadPool()
: m_maxSize(1)
, m_keepAlive(WTH_KEEPALIVE)
, m_poolSize(0)
, m_waitingCount(0)
, m_stopped(false)
, m_suspended(false)
, m_empty(false)
, m_pool(nullptr)
{
}

ThreadPool::ThreadPool(unsigned size)
: m_maxSize(size)
, m_keepAlive(WTH_KEEPALIVE)
, m_poolSize(0)
, m_waitingCount(0)
, m_stopped(false)
, m_suspended(false)
, m_empty(false)
, m_pool(nullptr)
{
}

ThreadPool::~ThreadPool()
{
  m_mutex.lock();
  // Reject new runs
  m_stopped = true;
  // Destroy all queued workers
  while (!m_queue.empty())
  {
    delete m_queue.front();
    m_queue.pop();
  }
  // Finalize all running
  if (m_pool)
  {
    m_empty = false;
    // Signal stop
    PooledThread* it = m_pool;
    do
    {
      it->stop_thread(false);
      it = it->next();
    } while (it != nullptr);
    // Wake sleeper
    m_queueFill.notify_all();
    // Waiting all finalized
    m_condition.wait(m_mutex, m_empty);
  }
}

bool ThreadPool::enqueue(Worker* worker)
{
  assert(worker->m_queued != true);
  LockGuard lock(m_mutex);
  if (!m_stopped)
  {
    worker->m_queued = true;
    m_queue.push(worker);
    if (!m_suspended)
    {
      if (m_waitingCount)
      {
        // Wake a thread
        m_queueFill.notify_one();
        return true;
      }
      else
      {
        __resize();
        return true;
      }
    }
    // Delayed work
    return true;
  }
  return false;
}

void ThreadPool::set_max_size(unsigned size)
{
  LockGuard lock(m_mutex);
  m_maxSize = size;
  if (!m_suspended)
    __resize();
}

void ThreadPool::set_keep_alive(unsigned millisec)
{
  LockGuard lock(m_mutex);
  m_keepAlive = millisec;
}

unsigned ThreadPool::size() const
{
  LockGuard lock(m_mutex);
  return m_poolSize;
}

unsigned ThreadPool::queue_size() const
{
  LockGuard lock(m_mutex);
  return static_cast<unsigned>(m_queue.size());
}

bool ThreadPool::is_queue_empty() const
{
  LockGuard lock(m_mutex);
  return m_queue.empty();
}

bool ThreadPool::wait_empty()
{
  return is_queue_empty() || m_queueEmpty.wait();
}

bool ThreadPool::wait_empty_for(unsigned millisec)
{
  return is_queue_empty() || m_queueEmpty.wait_for(millisec);
}

void ThreadPool::suspend()
{
  LockGuard lock(m_mutex);
  m_suspended = true;
}

void ThreadPool::resume()
{
  LockGuard lock(m_mutex);
  m_suspended = false;
  __resize();
}

bool ThreadPool::is_suspended() const
{
  LockGuard lock(m_mutex);
  return m_suspended;
}

void ThreadPool::reset()
{
  LockGuard lock(m_mutex);
  m_stopped = true;
  // Destroy all queued workers
  while (!m_queue.empty())
  {
    delete m_queue.front();
    m_queue.pop();
  }
}

void ThreadPool::stop()
{
  LockGuard lock(m_mutex);
  m_stopped = true;
}

void ThreadPool::start()
{
  LockGuard lock(m_mutex);
  m_stopped = false;
}

bool ThreadPool::is_stopped() const
{
  LockGuard lock(m_mutex);
  return m_stopped;
}

Worker* ThreadPool::pop_queue(PooledThread* pt)
{
  (void)pt;
  LockGuard lock(m_mutex);
  if (!m_suspended)
  {
    m_queueEmpty.notify_one();
    if (!m_queue.empty())
    {
      Worker* worker = m_queue.front();
      m_queue.pop();
      return worker;
    }
  }
  return nullptr;
}

void ThreadPool::wait_queue(PooledThread* pt)
{
  (void)pt;
  m_mutex.lock();
  ++m_waitingCount;
  unsigned millisec = m_keepAlive;
  m_mutex.unlock();
  m_queueFill.wait_for(millisec);
  m_mutex.lock();
  --m_waitingCount;
  m_mutex.unlock();
}

void ThreadPool::__resize()
{
  if (m_poolSize < m_maxSize && !m_queue.empty())
  {
    for (unsigned i = m_queue.size(); i > 0; --i)
    {
      if (m_poolSize >= m_maxSize)
        break;
      PooledThread* pt = PooledThread::create_thread_unlocked(*this);
      // The new thread will check the queue
      if (!pt->start_thread(false))
        pt->finalize();
    }
  }
  else if (m_poolSize > m_maxSize)
  {
    unsigned i = m_poolSize - m_maxSize;
    PooledThread* it = m_pool;
    while (it && i > 0)
    {
      it->stop_thread(false);
      it = it->next();
      --i;
    }
    // Wake up the waiting threads to stop
    if (m_waitingCount)
        m_queueFill.notify_all();
  }
}
