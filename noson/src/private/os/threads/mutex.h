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

#include "os-threads.h"

#ifdef NSROOT
namespace NSROOT {
#endif
namespace OS
{

  class Mutex
  {
  public:
    Mutex()
    : m_lockCount(0)
    {
      mutex_init(&m_handle);
    }

    ~Mutex()
    {
      clear();
      mutex_destroy(&m_handle);
    }

    mutex_t* native_handle()
    {
      return &m_handle;
    }

    bool try_lock()
    {
      if (mutex_trylock(&m_handle))
      {
        ++m_lockCount;
        return true;
      }
      return false;
    }

    void lock()
    {
      mutex_lock(&m_handle);
      ++m_lockCount;
    }

    void unlock()
    {
      if (mutex_trylock(&m_handle))
      {
        if (m_lockCount > 0)
        {
          --m_lockCount;
          mutex_unlock(&m_handle);
        }
        mutex_unlock(&m_handle);
      }
    }

    void clear()
    {
      if (mutex_trylock(&m_handle))
      {
        for (unsigned i = m_lockCount; i > 0; --i)
          mutex_unlock(&m_handle);
        m_lockCount = 0;
        mutex_unlock(&m_handle);
      }
    }

    // Prevent copy
    Mutex(const Mutex& other) = delete;
    Mutex& operator=(const Mutex& other) = delete;

  private:
    mutex_t           m_handle;
    unsigned          m_lockCount;
  };

  class LockGuard
  {
  public:
    LockGuard(Mutex& mutex) : m_mutex(mutex) { m_mutex.lock(); }
    ~LockGuard() { m_mutex.unlock(); }

    // Prevent copy
    LockGuard(const LockGuard& other) = delete;
    LockGuard& operator=(const LockGuard& other) = delete;

  private:
    Mutex&            m_mutex;
  };

}
#ifdef NSROOT
}
#endif
