/*
 *      Copyright (C) 2014-2024 Jean-Luc Barriere
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
#define	SHAREDPTR_H

#include "local_config.h"

#include <cstddef>  // for NULL

#define SHARED_PTR NSROOT::shared_ptr

namespace NSROOT
{
  namespace OS {
    class Atomic;
  }

  class shared_ptr_base
  {
  private:
    OS::Atomic* pc;
    OS::Atomic* spare;
  protected:
    virtual ~shared_ptr_base();
    shared_ptr_base();
    shared_ptr_base(const shared_ptr_base& s);
    shared_ptr_base& operator=(const shared_ptr_base& s);
    bool clear_counter(); /* returns true if destroyed */
    void reset_counter(); /* initialize a new count */
    void swap_counter(shared_ptr_base& s);
    int get_count() const;
    bool is_null() const { return pc == NULL; }
  };


  template<class T>
  class shared_ptr : private shared_ptr_base
  {
  private:
    T *p;
  public:

    shared_ptr()
    : shared_ptr_base()
    , p(NULL) { }

    explicit shared_ptr(T* s)
    : shared_ptr_base()
    , p(s)
    {
      if (s != NULL)
        shared_ptr_base::reset_counter();
    }

    shared_ptr(const shared_ptr& s)
    : shared_ptr_base(s)
    , p(s.p)
    {
      if (shared_ptr_base::is_null())
        p = NULL;
    }

    shared_ptr& operator=(const shared_ptr& s)
    {
      if (this != &s)
      {
        reset();
        p = s.p;
        shared_ptr_base::operator = (s);
        if (shared_ptr_base::is_null())
          p = NULL;
      }
      return *this;
    }

#if __cplusplus >= 201103L
    shared_ptr& operator=(shared_ptr&& s) noexcept
    {
      if (this != &s)
        swap(s);
      return *this;
    }
#endif

    ~shared_ptr()
    {
      reset();
    }

    void reset()
    {
      if (shared_ptr_base::clear_counter())
        delete p;
      p = NULL;
    }

    void reset(T* s)
    {
      if (p != s)
      {
        reset();
        p = s;
        if (s != NULL)
          shared_ptr_base::reset_counter();
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
      shared_ptr_base::swap_counter(s);
      if (shared_ptr_base::is_null())
        p = NULL;
    }

    int use_count() const
    {
      return shared_ptr_base::get_count();
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
      return p != NULL;
    }

    bool operator!() const
    {
      return p == NULL;
    }
  };

}

#endif	/* SHAREDPTR_H */
