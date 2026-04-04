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

#include "uriencoder.h"

#include "builtin.h"
#include <cstdio>
#include <cstring>
#include <string>

/* Valid characters for the URI path */
/* The following characters are considered as reserved (RFC 3986):
 *   unreserved    = ALPHA DIGIT - . _ ~ /
 *   sub-delims    = ! $ & ' ( ) * + , ; =
 *   gen-delims    = % @ : [ | ]
 * They won't be encoded or decoded.
 */
static const char uri_pchar_table[128] = {
//0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 1
  0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 2
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, // 3
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 4
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, // 5
  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 6
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, // 7
};

std::string pathencode(const std::string& str)
{
  std::string out;
  out.reserve(2 * str.length());
  const char* cstr = str.c_str();
  while (*cstr)
  {
    if (*cstr > 0 && uri_pchar_table[*cstr])
      out.push_back(*cstr);
    else
    {
      BUILTIN_BUFFER buf;
      char_to_uhex(*cstr, &buf);
      out.append("%").append(buf.data);
    }
    ++cstr;
  }
  return out;
}

std::string pathdecode(const std::string& str)
{
  std::string out;
  out.reserve(str.length());
  const char* cstr = str.c_str();
  while (*cstr)
  {
    char c = *cstr;
    if (c == '%')
    {
      int v;
      char buf[3];
      strncpy(buf, cstr + 1, 3);
      buf[2] = '\0';
      if (hex_to_num(buf, &v) == 0)
      {
        char d = static_cast<char>(v);
        // do not decode valid pchar
        if (d < 0 || uri_pchar_table[d] == 0)
        {
          c = d;
          cstr += 2;
        }
      }
    }
    out.push_back(c);
    ++cstr;
  }
  return out;
}

/* Valid characters for an embedded URL */
static const char uri_uchar_table[128] = {
//0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 1
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, // 2
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, // 3
  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 4
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, // 5
  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 6
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, // 7
};

std::string urlencode(const std::string& str)
{
  std::string out;
  out.reserve(2 * str.length());
  const char* cstr = str.c_str();
  while (*cstr)
  {
    if (*cstr > 0 && uri_uchar_table[*cstr])
      out.push_back(*cstr);
    else
    {
      BUILTIN_BUFFER buf;
      char_to_uhex(*cstr, &buf);
      out.append("%").append(buf.data);
    }
    ++cstr;
  }
  return out;
}

std::string urldecode(const std::string& str)
{
  std::string out;
  out.reserve(str.length());
  const char* cstr = str.c_str();
  while (*cstr)
  {
    char c = *cstr;
    if (c == '%')
    {
      int v;
      char buf[3];
      strncpy(buf, cstr + 1, 3);
      buf[2] = '\0';
      if (hex_to_num(buf, &v) == 0)
      {
        c = static_cast<char>(v);
        cstr += 2;
      }
    }
    out.push_back(c);
    ++cstr;
  }
  return out;
}
