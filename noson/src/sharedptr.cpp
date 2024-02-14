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

#include "sharedptr.h"
#include "private/os/threads/atomic.h"

using namespace NSROOT;

shared_ptr_base::shared_ptr_base()
: pc(NULL)
, spare(NULL) { }

shared_ptr_base::~shared_ptr_base()
{
  clear_counter();
  if (spare != NULL)
    delete spare;
}

shared_ptr_base::shared_ptr_base(const shared_ptr_base& s)
: pc(s.pc)
, spare(NULL)
{
  if (pc != NULL && (pc->load() == 0 || pc->add_fetch(1) < 2))
    pc = NULL;
}

shared_ptr_base& shared_ptr_base::operator=(const shared_ptr_base& s)
{
  if (this != &s)
  {
    clear_counter();
    pc = s.pc;
    if (pc != NULL && (pc->load() == 0 || pc->add_fetch(1) < 2))
      pc = NULL;
  }
  return *this;
}

bool shared_ptr_base::clear_counter()
{
  if (pc != NULL && pc->load() > 0 && pc->sub_fetch(1) == 0)
  {
    /* delete later */
    if (spare != NULL)
      delete spare;
    spare = pc;
    pc = NULL;
    return true;
  }
  pc = NULL;
  return false;
}

void shared_ptr_base::reset_counter()
{
  clear_counter();
  if (spare != NULL)
  {
    /* reuse the spare */
    spare->store(1);
    pc = spare;
    spare = NULL;
  }
  else
  {
    /* create a new */
    pc = new OS::Atomic(1);
  }
}

void shared_ptr_base::swap_counter(shared_ptr_base& s)
{
  OS::Atomic* _pc = pc;
  pc = s.pc;
  s.pc = _pc;
}

int shared_ptr_base::get_count() const
{
  return (pc != NULL ? pc->load() : 0);
}
