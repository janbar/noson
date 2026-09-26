/*
 *      Copyright (C) 2014-2026 Jean-Luc Barriere
 *
 *  This file is part of Noson
 *
 *  Noson is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Noson is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef LOCKED_H
#define	LOCKED_H

#include "local_config.h"

namespace NSROOT
{
  namespace OS { class Latch; }

  class Lockable
  {
    OS::Latch* m_latch;

  public:
    Lockable();
    virtual ~Lockable();

    void Lock();
    void Unlock();

    void LockShared();
    void UnlockShared();
    bool TryLockShared();
    
    class WriteLock
    {
      Lockable& m;
    public:
      WriteLock(Lockable& lock) : m(lock) { m.Lock(); }
      ~WriteLock() { m.Unlock(); }
    };

    class ReadLock
    {
      Lockable& m;
    public:
      ReadLock(Lockable& lock) : m(lock) { m.LockShared(); }
      ~ReadLock() { m.UnlockShared(); }
    };

    // Prevent copy
    Lockable(const Lockable& other) = delete;
    Lockable& operator=(const Lockable& other) = delete;
  };

  template<typename T>
  class Locked : private Lockable
  {
  public:
    Locked(const T& val) : Lockable(), m_val(val) { }
    ~Locked() override { }

    T Load()
    {
      ReadLock g(*this);
      return m_val; // return copy
    }

    const T& Store(const T& newval)
    {
      WriteLock g(*this);
      m_val = newval;
      return newval; // return input
    }

    class pointer
    {
      friend class Locked;
    public:
      T& operator* () const { return *m_val; }
      T *operator->() const { return m_val; }

      pointer() : m_val(nullptr), m_x(nullptr) { }
      ~pointer() { if (m_x) m_x->Unlock(); }

      pointer(const pointer& other)
      : m_val(other.m_val), m_x(other.m_x) { if (m_x) m_x->Lock(); }

      pointer& operator=(const pointer& other)
      {
        if (m_x) m_x->Unlock();
        m_val = other.m_val;
        m_x = other.m_x;
        if (m_x) m_x->Lock();
        return *this;
      }

      pointer(pointer&& other) noexcept
      : m_val(other.m_val), m_x(other.m_x)
      {
        other.m_val = nullptr;
        other.m_x = nullptr;
      }

      pointer& operator=(pointer&& other) noexcept
      {
        if (m_x) m_x->Unlock();
        m_val = other.m_val;
        m_x = other.m_x;
        other.m_val = nullptr;
        other.m_x = nullptr;
        return *this;
      }

    private:
      pointer(T* val, Lockable* lock)
      : m_val(val), m_x(lock) { m_x->Lock(); }
      T* m_val;
      Lockable* m_x;
    };

    pointer GetExclusive()
    {
      return pointer(&m_val, this);
    }

    class const_pointer
    {
      friend class Locked;
    public:
      const T& operator* () const { return *m_val; }
      const T *operator->() const { return m_val; }

      const_pointer() : m_val(nullptr), m_s(nullptr) { }
      ~const_pointer() { if (m_s) m_s->UnlockShared(); }

      const_pointer(const const_pointer& other)
      : m_val(other.m_val), m_s(other.m_s) { if (m_s) m_s->LockShared(); }

      const_pointer& operator=(const const_pointer& other)
      {
        if (m_s) m_s->UnlockShared();
        m_val = other.m_val;
        m_s = other.m_s;
        if (m_s) m_s->LockShared();
        return *this;
      }

      const_pointer(const_pointer&& other) noexcept
      : m_val(other.m_val), m_s(other.m_s)
      {
        other.m_val = nullptr;
        other.m_s = nullptr;
      }

      const_pointer& operator=(const_pointer&& other) noexcept
      {
        if (m_s) m_s->UnlockShared();
        m_val = other.m_val;
        m_s = other.m_s;
        other.m_val = nullptr;
        other.m_s = nullptr;
        return *this;
      }

    private:
      const_pointer(const T* val, Lockable* lock)
      : m_val(val), m_s(lock) { m_s->LockShared(); }
      const T* m_val;
      Lockable* m_s;
    };

    const_pointer GetShared()
    {
      return const_pointer(&m_val, this);
    }

    // Prevent copy
    Locked(const Locked<T>& other) = delete;
    Locked<T>& operator=(const Locked<T>& other) = delete;

  protected:
    T m_val;
  };

}

#endif	/* LOCKED_H */
