/*
 *      Copyright (C) 2014-2015 Jean-Luc Barriere
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

#include "sonoszone.h"

using namespace NSROOT;

std::string Zone::GetZoneName() const
{
  // Concat ZonePlayer: zp1 + zp2 + ...
  std::string name;
  for (const_iterator it = begin(); it != end(); ++it)
    if (*it)
    {
      if (!name.empty())
        name.append("+");
      name.append(**it);
    }
  return name;
}

ZonePlayerPtr Zone::GetCoordinator() const
{
  for (const_iterator it = begin(); it != end(); ++it)
    if (*it && (*it)->GetAttribut("coordinator") == "true")
      return *it;
  return ZonePlayerPtr();
}
