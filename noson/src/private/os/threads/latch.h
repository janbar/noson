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

// Compatibility with C++98 remains
#include <cstddef> // for NULL

#ifdef NSROOT
namespace NSROOT {
#endif
namespace OS
{

  class Latch
  {
  public:
    Latch();
    Latch(bool _px);
    ~Latch();

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

#if __cplusplus >= 201103L
    // Prevent copy
    Latch(const Latch& other) = delete;
    Latch& operator=(const Latch& other) = delete;
#endif

  private:
    mutable Atomic s_spin;
    thread_t x_owner;

    int x_wait;                         /* counts requests in wait for X  */
    int x_flag;                         /* X status: 0, 1, 2, or 3        */

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

#if __cplusplus < 201103L
    // Prevent copy
    Latch(const Latch& other);
    Latch& operator=(const Latch& other);
#endif

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

    void init();
    TNode * find_node(const thread_t& id);
    TNode * new_node(const thread_t& id);
    void free_node(TNode * n);
  };

  class ReadLock
  {
  private:
    Latch *p;
    bool owns;

#if __cplusplus < 201103L
    ReadLock(const ReadLock& other);
    ReadLock& operator=(const ReadLock& other);
#endif

  public:

    static struct adopt_lock_t { } adopt_lock;

    ReadLock() : p(NULL), owns(false) { }

    ReadLock(Latch& latch) : p(&latch), owns(true) { latch.lock_shared(); }

    /* Assume the calling thread already has ownership of the shared lock */
    ReadLock(Latch& latch, adopt_lock_t) : p(&latch), owns(true) { }

    ~ReadLock()
    {
      if (owns)
      {
        p->unlock_shared();
      }
    }

    void swap(ReadLock& rl)
    {
      Latch * _p = p;
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
      if (!owns && p != NULL)
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
      if (!owns && p != NULL)
      {
        owns = p->try_lock_shared();
      }
      return owns;
    }

#if __cplusplus >= 201103L
    ReadLock(const ReadLock& other) = delete;
    ReadLock& operator=(const ReadLock& other) = delete;
#endif
  };

  class WriteLock
  {
  private:
    Latch *p;
    bool owns;

#if __cplusplus < 201103L
    WriteLock(const WriteLock& other);
    WriteLock& operator=(const WriteLock& other);
#endif

  public:

    WriteLock() : p(NULL), owns(false) { }

    explicit WriteLock(Latch& latch) : p(&latch), owns(true) { latch.lock(); }

    ~WriteLock()
    {
      if (owns)
      {
        p->unlock();
      }
    }

    void swap(WriteLock& wl)
    {
      Latch * _p = p;
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
      if (!owns && p != NULL)
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

#if __cplusplus >= 201103L
    WriteLock(const WriteLock& other) = delete;
    WriteLock& operator=(const WriteLock& other) = delete;
#endif
  };

}
#ifdef NSROOT
}
#endif
