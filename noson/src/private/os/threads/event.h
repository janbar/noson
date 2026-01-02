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

#include "condition.h"

// Compatibility with C++98 remains

#ifdef NSROOT
namespace NSROOT {
#endif
namespace OS
{

  class Event
  {
  public:
    Event(bool autoReset = true)
    : m_notified(false)
    , m_notifyOne(false)
    , m_waitingCount(0)
    , m_autoReset(autoReset) {}

    ~Event() {}

    void Broadcast()
    {
      LockGuard lock(m_mutex);
      m_notifyOne = false;
      m_notified  = true;
      m_condition.Broadcast();
    }

    void Signal()
    {
      LockGuard lock(m_mutex);
      m_notifyOne = true;
      m_notified  = true;
      m_condition.Signal();
    }

    bool Wait()
    {
      LockGuard lock(m_mutex);
      ++m_waitingCount;
      bool notified = m_condition.Wait(m_mutex, m_notified);
      --m_waitingCount;
      if (m_autoReset && notified)
        __reset(m_notifyOne);
      return notified;
    }

    bool Wait(unsigned timeout)
    {
      LockGuard lock(m_mutex);
      ++m_waitingCount;
      bool notified = m_condition.Wait(m_mutex, m_notified, timeout);
      --m_waitingCount;
      if (m_autoReset && notified)
        __reset(m_notifyOne);
      return notified;
    }

    void Reset()
    {
      LockGuard lock(m_mutex);
      __reset(true);
    }

#if __cplusplus >= 201103L
    // Prevent copy
    Event(const Event& other) = delete;
    Event& operator=(const Event& other) = delete;
#endif

  private:
    volatile bool             m_notified;
    volatile bool             m_notifyOne;
    unsigned                  m_waitingCount;
    bool                      m_autoReset;
    Condition<volatile bool>  m_condition;
    Mutex                     m_mutex;

    void __reset(bool force)
    {
      if (force || m_waitingCount == 0)
        m_notified = false;
    }

#if __cplusplus < 201103L
    // Prevent copy
    Event(const Event& other);
    Event& operator=(const Event& other);
#endif
  };
}
#ifdef NSROOT
}
#endif
