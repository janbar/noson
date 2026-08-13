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

ThreadPool::ThreadPool()
: m_size(1)
, m_keepAlive(WTH_KEEPALIVE)
, m_poolSize(0)
, m_waitingCount(0)
, m_stopped(false)
, m_suspended(false)
, m_empty(false)
{
}

ThreadPool::ThreadPool(unsigned size)
: m_size(size)
, m_keepAlive(WTH_KEEPALIVE)
, m_poolSize(0)
, m_waitingCount(0)
, m_stopped(false)
, m_suspended(false)
, m_empty(false)
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
  if (!m_pool.empty())
  {
    m_empty = false;
    // Signal stop
    for (std::set<WorkerThread*>::iterator it = m_pool.begin(); it != m_pool.end(); ++it)
      (*it)->stop_thread(false);
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
  m_size = size;
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

Worker* ThreadPool::pop_queue(WorkerThread* _thread)
{
  (void)_thread;
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

void ThreadPool::wait_queue(WorkerThread* _thread)
{
  (void)_thread;
  m_mutex.lock();
  ++m_waitingCount;
  unsigned millisec = m_keepAlive;
  m_mutex.unlock();
  m_queueFill.wait_for(millisec);
  m_mutex.lock();
  --m_waitingCount;
  m_mutex.unlock();
}

void ThreadPool::start_thread(WorkerThread* _thread)
{
  ++m_poolSize;
  m_pool.insert(_thread);
  if (!_thread->start_thread(false))
    finalize_thread(_thread);
}

void ThreadPool::finalize_thread(WorkerThread* _thread)
{
  LockGuard lock(m_mutex);
  if (m_pool.erase(_thread))
  {
    --m_poolSize;
    delete _thread;
  }
  if (m_pool.empty())
  {
    m_empty = true;
    m_condition.notify_all();
  }
}

void ThreadPool::__resize()
{
  if (m_poolSize < m_size && !m_queue.empty())
  {
    for (unsigned i = m_queue.size(); i > 0; --i)
    {
      if (m_poolSize >= m_size)
        break;
      WorkerThread* _thread = new WorkerThread(*this);
      // The new thread will check the queue
      start_thread(_thread);
    }
  }
  else if (m_poolSize > m_size)
  {
    std::set<WorkerThread*>::iterator it = m_pool.begin();
    for (unsigned i = m_poolSize - m_size; i > 0; --i)
    {
      if (it == m_pool.end())
        break;
      (*it)->stop_thread(false);
      ++it;
    }
    // Wake up the waiting threads to stop
    if (m_waitingCount)
        m_queueFill.notify_all();
  }
}
