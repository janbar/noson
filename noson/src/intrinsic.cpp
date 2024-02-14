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

#include "intrinsic.h"
#include "private/os/threads/atomic.h"

using namespace NSROOT;

IntrinsicCounter::IntrinsicCounter(int val)
: m_ptr(new OS::Atomic(val))
{
}

IntrinsicCounter::~IntrinsicCounter()
{
  delete m_ptr;
}

int IntrinsicCounter::GetValue()
{
  return m_ptr->load();
}

int IntrinsicCounter::Increment()
{
  return m_ptr->increment();
}

int IntrinsicCounter::Decrement()
{
  return m_ptr->decrement();
}
