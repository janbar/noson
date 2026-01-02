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

#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <string>
#include <vector>

#define tokenize __tokenize
inline bool __tokenize(
        const std::string& str,
        const std::string& separator,
        const std::string& encapsulator,
        std::vector<std::string>& tokens,
        bool trimnull = false)
{
  tokens.clear();
  size_t pos = 0;
  size_t end = str.size();
  if (pos == end)
    return true;
  size_t lsep = separator.size();
  size_t lenc = encapsulator.size();
  std::string token;
  bool first =  true;
  bool encap = false;
  while (pos != end)
  {
    if (lenc && str.compare(pos, lenc, encapsulator) == 0)
    {
      pos += lenc;
      if (encap)
      {
        if (pos == end || str.compare(pos, lenc, encapsulator) != 0)
        {
          encap = false;
          while (pos != end && lsep && str.compare(pos, lsep, separator) != 0) ++pos;
        }
        else
        {
          token.push_back(str[pos]);
          ++pos;
        }
      }
      else if (!first)
      {
        // invalid character in stream
        return false;
      }
      else
      {
        encap = true;
      }
    }
    else if (!encap && lsep && str.compare(pos, lsep, separator) == 0)
    {
      // trim null token
      if (!trimnull || !token.empty())
      {
        tokens.push_back(std::move(token));
        token.clear();
      }
      first = true;
      pos += lsep;
    }
    else
    {
      first = false;
      token.push_back(str[pos]);
      ++pos;
    }
  }
  if (encap)
  {
    // quoted string not terminated
    return false;
  }
  if (!trimnull || !token.empty())
    tokens.push_back(std::move(token));
  return true;
}

#endif /* TOKENIZER_H */

