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

#include "locked.h"
#include "private/os/threads/latch.h"

using namespace NSROOT;

Lockable::Lockable()
: m_latch(new OS::Latch())
{
}

Lockable::~Lockable()
{
  delete m_latch;
}

void Lockable::Lock()
{
  m_latch->lock();
}

void Lockable::Unlock()
{
  m_latch->unlock();
}

void Lockable::LockShared()
{
  m_latch->lock_shared();
}

void Lockable::UnlockShared()
{
  m_latch->unlock_shared();
}

bool Lockable::TryLockShared()
{
  return m_latch->try_lock_shared();
}
