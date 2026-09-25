/*
 *      Copyright (C) 2026 Jean-Luc Barriere
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

#include "sharedptr.h"
#include "private/os/threads/atomic.h"

using namespace NSROOT;

refcount::refcount(const refcount& s)
: pc(s.pc)
{
  if (pc != nullptr && pc->fetch_add(1) < 1)
    pc = nullptr;
}

refcount& refcount::operator=(const refcount& s)
{
  if (this != &s)
  {
    reset();
    pc = s.pc;
    if (pc != nullptr && pc->fetch_add(1) < 1)
      pc = nullptr;
  }
  return *this;
}

bool refcount::reset()
{
  if (pc != nullptr && pc->fetch_sub(1) == 1)
  {
    delete pc;
    pc = nullptr;
    return true;
  }
  pc = nullptr;
  return false;
}

void refcount::renew()
{
  if (pc == nullptr)
    pc = new OS::Atomic(1);
}

void refcount::swap(refcount& s)
{
  OS::Atomic* _pc = pc;
  pc = s.pc;
  s.pc = _pc;
}

int refcount::get_count() const
{
  return (pc != nullptr ? pc->load() : 0);
}

