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

// Compatibility with C++98 remains

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
      Clear();
      mutex_destroy(&m_handle);
    }

    mutex_t* NativeHandle()
    {
      return &m_handle;
    }

    bool TryLock()
    {
      if (mutex_trylock(&m_handle))
      {
        ++m_lockCount;
        return true;
      }
      return false;
    }

    void Lock()
    {
      mutex_lock(&m_handle);
      ++m_lockCount;
    }

    void Unlock()
    {
      if (mutex_trylock(&m_handle))
      {
        if (m_lockCount > 0)
        {
          mutex_unlock(&m_handle);
          --m_lockCount;
        }
        mutex_unlock(&m_handle);
      }
    }

    void Clear()
    {
      if (mutex_trylock(&m_handle))
      {
        for (unsigned i = m_lockCount; i > 0; --i)
          mutex_unlock(&m_handle);
        m_lockCount = 0;
        mutex_unlock(&m_handle);
      }
    }

#if __cplusplus >= 201103L
    // Prevent copy
    Mutex(const Mutex& other) = delete;
    Mutex& operator=(const Mutex& other) = delete;
#endif

  private:
    mutex_t           m_handle;
    unsigned          m_lockCount;

#if __cplusplus < 201103L
    // Prevent copy
    Mutex(const Mutex& other);
    Mutex& operator=(const Mutex& other);
#endif
  };

  class LockGuard
  {
  public:
    LockGuard(Mutex& mutex) : m_mutex(mutex) { m_mutex.Lock(); }
    ~LockGuard() { m_mutex.Unlock(); }
#if __cplusplus >= 201103L
    // Prevent copy
    LockGuard(const LockGuard& other) = delete;
    LockGuard& operator=(const LockGuard& other) = delete;
#endif
  private:
    Mutex&            m_mutex;
#if __cplusplus < 201103L
    // Prevent copy
    LockGuard(const LockGuard& other);
    LockGuard& operator=(const LockGuard& other);
#endif
  };

}
#ifdef NSROOT
}
#endif
