#pragma once
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

#include "../os.h"

#include <atomic>

#ifdef NSROOT
namespace NSROOT {
#endif
namespace OS
{
  class Atomic
  {
  private:
    std::atomic<int> m_val;
  public:
    Atomic() : m_val(0) {}
    Atomic(int val) : m_val(val) {}
    int load()
    {
      return m_val.load(std::memory_order_acquire);
    }
    int operator()()
    {
      return load();
    }
    void store(int val)
    {
      m_val.store(val, std::memory_order_release);
    }
    int operator=(int val)
    {
      store(val);
      return val;
    }
    int add_fetch(int amount)
    {
      return m_val.fetch_add(amount, std::memory_order_acq_rel) + amount;
    }
    int increment()
    {
      return add_fetch(1);
    }
    int sub_fetch(int amount)
    {
      return m_val.fetch_sub(amount, std::memory_order_acq_rel) - amount;
    }
    int decrement()
    {
      return sub_fetch(1);
    }
  };
}
#ifdef NSROOT
}
#endif
