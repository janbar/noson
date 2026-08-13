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

#include "mutex.h"
#include "timeout.h"

#ifdef NSROOT
namespace NSROOT {
#endif
namespace OS
{

  template <typename P>
  class Condition
  {
  public:
    Condition()
    {
      cond_init(&m_condition);
    }

    ~Condition()
    {
      cond_destroy(&m_condition);
    }

    void notify_all()
    {
      cond_broadcast(&m_condition);
    }

    void notify_one()
    {
      cond_signal(&m_condition);
    }

    bool wait(Mutex& lock, P& predicate)
    {
      while(!predicate)
        cond_wait(&m_condition, lock.native_handle());
      return true;
    }

    bool wait_for(Mutex& lock, unsigned millisec, P& predicate)
    {
      Timeout _timeout(millisec);
      while (!predicate)
      {
        // wait for time left
        millisec = _timeout.time_left();
        if (millisec == 0)
          return false;
        cond_timedwait(&m_condition, lock.native_handle(), millisec);
      }
      return true;
    }

    bool wait_for(Mutex& lock, Timeout& timeout)
    {
      cond_timedwait(&m_condition, lock.native_handle(), timeout.time_left());
      return (timeout.time_left() > 0 ? true : false);
    }

    // Prevent copy
    Condition(const Condition<P>& other) = delete;
    Condition<P>& operator=(const Condition<P>& other) = delete;

  private:
    condition_t m_condition;
  };

}
#ifdef NSROOT
}
#endif
