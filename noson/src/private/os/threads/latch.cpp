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

#include "latch.h"

#include <cassert>

#ifdef NSROOT
using namespace NSROOT::OS;
#else
using namespace OS;
#endif

CLatch::TNode * CLatch::find_node(const thread_t& id)
{
  TNode * p = s_nodes;
  while (p != NULL && thread_equal(p->id, id) == 0)
  {
    p = p->_next;
  }
  return p;
}

CLatch::TNode * CLatch::new_node(const thread_t& id)
{
  TNode * p;
  if (s_freed == NULL)
  {
    /* create node */
    p = new TNode();
  }
  else
  {
    /* pop front from free list */
    p = s_freed;
    s_freed = p->_next;
  }

  /* setup */
  p->id = id;
  p->count = 0;

  /* push front in list */
  p->_prev = NULL;
  p->_next = s_nodes;
  if (s_nodes != NULL)
  {
    s_nodes->_prev = p;
  }
  s_nodes = p;
  return p;
}

void CLatch::free_node(TNode * n)
{
  /* remove from list */
  if (n == s_nodes)
  {
    s_nodes = n->_next;
  }
  else
  {
    n->_prev->_next = n->_next;
  }
  if (n->_next != NULL)
  {
    n->_next->_prev = n->_prev;
  }

  /* push front in free list */
  if (s_freed != NULL)
  {
    s_freed->_prev = n;
  }
  n->_next = s_freed;
  n->_prev = NULL;
  s_freed = n;
}

CLatch::CLatch(bool _px)
: s_spin(0)
, x_wait(0)
, x_flag(0)
, px(_px)
, s_freed(NULL)
, s_nodes(NULL)
{
  mutex_init(&x_gate_lock);
  cond_init(&x_gate);
  mutex_init(&s_gate_lock);
  cond_init(&s_gate);

  /* preallocate free list with 2 nodes */
  TNode * n1 = new_node(thread_t());
  TNode * n2 = new_node(thread_t());
  free_node(n1);
  free_node(n2);
}

CLatch::~CLatch()
{
  /* destroy free nodes */
  while (s_freed != nullptr) {
    TNode * n = s_freed;
    s_freed = s_freed->_next;
    delete n;
  }
  /* it should be empty, but still tries to destroy any existing busy node */
  while (s_nodes != nullptr) {
    TNode * n = s_nodes;
    s_nodes = s_nodes->_next;
    delete n;
  }

  cond_destroy(&s_gate);
  mutex_destroy(&s_gate_lock);
  cond_destroy(&x_gate);
  mutex_destroy(&x_gate_lock);
}

/**
 * The X flag is set as follows based on the locking steps
 * Step 0 : X is released
 * Step 1 : X is held, but waits for release of S
 * Step 2 : X was released and left available for one of request in wait
 * Step 3 : X is held
 * Step N : X recursive N-3
 */
#define X_STEP_0 0
#define X_STEP_1 1
#define X_STEP_2 2
#define X_STEP_3 3

/* Depending on the internal implementation of conditional variable,
 * a race condition could arise, permanently blocking the thread;
 * Setting a timeout works around the issue.
 */
#define EXIT_TIMEOUT 1000

void CLatch::lock()
{
  thread_t tid = thread_self();

  spin_lock();

  if (!thread_equal(x_owner, tid))
  {
    /* increments the count of request in wait */
    ++x_wait;
    for (;;)
    {
      /* if flag is 0 or 2 then it hold X with no wait,
       * in other case it have to wait for X gate
       */
      if (x_flag == X_STEP_0 || x_flag == X_STEP_2)
      {
        x_flag = X_STEP_1;
        --x_wait;
        break;
      }
      else
      {
        /* !!! pop gate then unlock spin */
        mutex_lock(&x_gate_lock);
        spin_unlock();
        cond_timedwait(&x_gate, &x_gate_lock, EXIT_TIMEOUT);
        mutex_unlock(&x_gate_lock);
      }
      spin_lock();
    }

    /* find the thread node */
    TNode * n = find_node(tid);
    /* X = 1, check the releasing of S */
    for (;;)
    {
      /* if the count of S is zeroed, or equal to self count, then it finalizes
       * with no wait,
       * in other case it has to wait for S gate */
      if (s_nodes == NULL || (s_nodes == n && s_nodes->_next == NULL))
      {
        x_flag = X_STEP_3;
        break;
      }
      else
      {
        /* !!! pop gate then unlock spin (reverse order for S notifier) */
        mutex_lock(&s_gate_lock);
        spin_unlock();
        cond_timedwait(&s_gate, &s_gate_lock, EXIT_TIMEOUT);
        mutex_unlock(&s_gate_lock);
        spin_lock();
        /* check if the notifier has hand over, else retry */
        if (x_flag == X_STEP_3)
        {
          break;
        }
      }
    }

    /* X = 3, set owner */
    x_owner = tid;
  }
  else
  {
    /* recursive X lock */
    ++x_flag;
  }

  spin_unlock();
}

void CLatch::unlock()
{
  thread_t tid = thread_self();

  spin_lock();
  if (thread_equal(x_owner, tid))
  {
    /* decrement recursive lock */
    if (--x_flag == X_STEP_2)
    {
      x_owner = {};
      /* hand-over to a request in wait for X, else release */
      if (x_wait == 0)
      {
        x_flag = X_STEP_0;
      }
      /* !!! unlock spin then pop gate (reverse order for receiver) */
      spin_unlock();
      mutex_lock(&x_gate_lock);
      cond_broadcast(&x_gate);
      mutex_unlock(&x_gate_lock);
    }
    else
    {
      spin_unlock();
    }
  }
  else
  {
    spin_unlock();
  }
}

void CLatch::lock_shared()
{
  thread_t tid = thread_self();

  spin_lock();

  /* find the thread node */
  TNode * n = find_node(tid);

  if (!thread_equal(x_owner, tid))
  {
    /* if flag is 0 or 1 then it hold S with no wait,
     * in other case it have to wait for X gate
     */
    for (;;)
    {
      if (!px)
      {
        /* X precedence is false */
        if (x_flag < X_STEP_2)
        {
          break;
        }
      }
      else
      {
        /* X precedence is true,
         * test if this thread holds a recursive S lock
         */
        if (x_flag == X_STEP_0 || (x_flag == X_STEP_1 && n != NULL))
        {
          break;
        }
      }
      /* !!! pop gate then unlock spin */
      mutex_lock(&x_gate_lock);
      spin_unlock();
      cond_timedwait(&x_gate, &x_gate_lock, EXIT_TIMEOUT);
      mutex_unlock(&x_gate_lock);
      spin_lock();
    }
  }
  if (n == NULL)
  {
    n = new_node(tid);
  }
  /* increment recursive count for this thread */
  ++n->count;

  spin_unlock();
}

void CLatch::unlock_shared()
{
  thread_t tid = thread_self();

  spin_lock();

  /* find the thread node */
  TNode * n = find_node(tid);
  /* does it own shared lock ? */
  assert(n != NULL);

  /* decrement recursive count for this thread, finally free */
  if (--n->count == 0)
  {
    free_node(n);
    /* on last S, finalize X request in wait, and notify */
    if (x_flag == X_STEP_1 && !thread_equal(x_owner, tid))
    {
      if (s_nodes == NULL)
      {
        x_flag = X_STEP_3;
      }
      /* !!! unlock spin then pop gate (reverse order for X receiver) */
      spin_unlock();
      mutex_lock(&s_gate_lock);
      cond_signal(&s_gate);
      mutex_unlock(&s_gate_lock);
    }
    else
    {
      spin_unlock();
    }
  }
  else
  {
    spin_unlock();
  }
}

bool CLatch::try_lock_shared()
{
  thread_t tid = thread_self();

  spin_lock();
  /* if X = 0 then it hold S with success,
   * in other case fails
   */
  if (x_flag == X_STEP_0 || thread_equal(x_owner, tid))
  {
    /* find the thread node, else create */
    TNode * n = find_node(tid);
    if (n == NULL)
    {
      n = new_node(tid);
    }
    /* increment recursive count for this thread */
    ++n->count;

    spin_unlock();
    return true;
  }
  spin_unlock();
  return false;
}
