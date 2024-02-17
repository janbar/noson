#pragma once
/*
 *      Copyright (C) 2024 Jean-Luc Barriere
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
#include "atomic.h"

#ifdef NSROOT
namespace NSROOT {
#endif
namespace OS
{

  class CLatch
  {
  public:
    CLatch(bool _px);
    CLatch() : CLatch(true) { }
    ~CLatch();

    /* Locks the latch for exclusive ownership,
     * blocks if the latch is not available
     */
    void lock();

    /* Unlocks the latch (exclusive ownership) */
    void unlock();

    /* Locks the latch for shared ownership,
     * blocks if the latch is not available
     */
    void lock_shared();

    /* Unlocks the latch (shared ownership) */
    void unlock_shared();

    /* Tries to lock the latch for shared ownership,
     * returns true if the latch has no exclusive ownership or any request for
     * exclusive ownership, else false
     */
    bool try_lock_shared();

  private:
    mutable Atomic s_spin;
    thread_t x_owner;

    volatile int x_wait;                /* counts requests in wait for X  */
    volatile int x_flag;                /* X status: 0, 1, 2, or 3        */

    mutex_t x_gate_lock;
    condition_t x_gate;                 /* wait for release of X          */
    mutex_t s_gate_lock;
    condition_t s_gate;                 /* wait for release of S          */

    bool px;                            /* enable X precedence            */

    struct TNode {
      TNode * _prev;
      TNode * _next;
      thread_t id;
      int count;
    };
    TNode * s_freed;
    TNode * s_nodes;

    // Prevent copy
    CLatch(const CLatch& other);
    CLatch& operator=(const CLatch& other);

    void spin_lock()
    {
      while (s_spin.increment() != 1)
      {
        do
        {
          sched_yield();
        } while (s_spin.load() != 0);
      }
    }
    void spin_unlock() { s_spin.store(0); }

    TNode * find_node(const thread_t& id);
    TNode * new_node(const thread_t& id);
    void free_node(TNode * n);
  };

  class CReadLock
  {
  private:
    CLatch *p = nullptr;
    bool owns = false;

    CReadLock(const CReadLock& other);
    CReadLock& operator=(const CReadLock& other);

  public:

    static struct adopt_lock_t { } adopt_lock;

    CReadLock() { }

    CReadLock(CLatch& latch) : p(&latch), owns(true) { latch.lock_shared(); }

    /* Assume the calling thread already has ownership of the shared lock */
    CReadLock(CLatch& latch, adopt_lock_t) : p(&latch), owns(true) { }

    ~CReadLock()
    {
      if (owns)
      {
        p->unlock_shared();
      }
    }

    void swap(CReadLock& rl)
    {
      CLatch * _p = p;
      bool _owns = owns;
      p = rl.p;
      owns = rl.owns;
      rl.p = _p;
      rl.owns = _owns;
    }

    bool owns_lock() const
    {
      return owns;
    }

    void lock()
    {
      if (!owns && p != nullptr)
      {
        p->lock_shared();
        owns = true;
      }
    }

    void unlock()
    {
      if (owns)
      {
        owns = false;
        p->unlock_shared();
      }
    }

    bool try_lock()
    {
      if (!owns && p != nullptr)
      {
        owns = p->try_lock_shared();
      }
      return owns;
    }
  };

  class CWriteLock
  {
  private:
    CLatch *p = nullptr;
    bool owns = false;

    CWriteLock(const CWriteLock& other);
    CWriteLock& operator=(const CWriteLock& other);

  public:

    CWriteLock() = default;

    explicit CWriteLock(CLatch& latch) : p(&latch), owns(true) { latch.lock(); }

    ~CWriteLock()
    {
      if (owns)
      {
        p->unlock();
      }
    }

    void swap(CWriteLock& wl)
    {
      CLatch * _p = p;
      bool _owns = owns;
      p = wl.p;
      owns = wl.owns;
      wl.p = _p;
      wl.owns = _owns;
    }

    bool owns_lock() const
    {
      return owns;
    }

    void lock()
    {
      if (!owns && p != nullptr)
      {
        p->lock();
        owns = true;
      }
    }

    void unlock()
    {
      if (owns)
      {
        owns = false;
        p->unlock();
      }
    }
  };

}
#ifdef NSROOT
}
#endif
