/*
 *      Copyright (C) 2014-2026 Jean-Luc Barriere
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

#ifndef SHAREDPTR_H
#define SHAREDPTR_H

#include "local_config.h"

#define SHARED_PTR NSROOT::shared_ptr

namespace NSROOT
{
  namespace OS {
    class Atomic;
  }

  class refcount
  {
  private:
    OS::Atomic* pc;
  public:
    refcount()
    : pc(nullptr) { }

    ~refcount()
    {
      reset();
    }

    refcount(const refcount& s);

    refcount& operator=(const refcount& s);

    bool reset(); /* returns true if pc is destroyed */

    void renew(); /* initialize pc */

    void swap(refcount& s);

    int get_count() const;

    bool is_null() const
    {
      return (pc == nullptr);
    }
  };

  template<class T>
  class shared_ptr
  {
  private:

    T *p;
    refcount pc;

  public:

    shared_ptr()
    : p(nullptr)
    , pc() { }

    explicit shared_ptr(T* s)
    : p(s)
    {
      if (s != nullptr)
        pc.renew();
    }

    shared_ptr(const shared_ptr& s)
    : p(s.p)
    , pc(s.pc)
    {
      if (pc.is_null())
        p = nullptr;
    }

    shared_ptr& operator=(const shared_ptr& s)
    {
      if (this != &s)
      {
        reset();
        p = s.p;
        pc = s.pc;
        if (pc.is_null())
          p = nullptr;
      }
      return *this;
    }

    shared_ptr& operator=(shared_ptr&& s) noexcept
    {
      if (this != &s)
        swap(s);
      return *this;
    }

    ~shared_ptr()
    {
      reset();
    }

    void reset()
    {
      if (pc.reset())
        delete p;
      p = nullptr;
    }

    void reset(T* s)
    {
      if (p != s)
      {
        reset();
        p = s;
        if (s != nullptr)
          pc.renew();
      }
    }

    T *get() const
    {
      return p;
    }

    void swap(shared_ptr<T>& s)
    {
      T* _p = p;
      p = s.p;
      s.p = _p;
      pc.swap(s.pc);
      if (pc.is_null())
        p = nullptr;
    }

    int use_count() const
    {
      return pc.get_count();
    }

    T *operator->() const
    {
      return get();
    }

    T& operator*() const
    {
      return *get();
    }

    operator bool() const
    {
      return p != nullptr;
    }

    bool operator!() const
    {
      return p == nullptr;
    }
  };

}

#endif	/* SHAREDPTR_H */

