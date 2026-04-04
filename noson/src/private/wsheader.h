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

#ifndef WSHEADER_H
#define WSHEADER_H

#include "local_config.h"

#include <string>
#include <vector>
#include <cctype>
#include <algorithm>

class WSHeader
{
public:
  typedef std::vector<std::string> container_t;
  typedef container_t::iterator iterator;
  typedef container_t::const_iterator const_iterator;

  WSHeader()
  : m_name()
  {
    m_values.emplace_back(std::string());
  }

  explicit WSHeader(const std::string& name)
  : m_name(name)
  {
    m_values.emplace_back(std::string());
  }

  WSHeader(const std::string& name, const std::string& value)
  : m_name(name)
  {
    m_values.emplace_back(value);
  }

  explicit WSHeader(const WSHeader& o)
  : m_name(o.m_name), m_values(o.m_values)
  { }

  virtual ~WSHeader() { }

  void SetName(const std::string& name) { m_name.assign(name); }
  void SetName(const char * buf, size_t n) { m_name.assign(buf, n); }

  std::string Key() const
  {
    std::string key(m_name);
    std::transform(key.begin(), key.end(), key.begin(), ::toupper);
    return key;
  }

  const std::string& Name() const { return m_name; }
  const std::string& Last() const { return m_values.back(); }
  std::string& Back() { return m_values.back(); }

  iterator begin() { return m_values.begin(); }
  iterator end() { return m_values.end(); }
  const_iterator cbegin() { return m_values.cbegin(); }
  const_iterator cend() { return m_values.cend(); }

  void AddValue(const std::string& value)
  {
    if (m_values.back().empty())
      m_values.back().assign(value);
    else
      m_values.emplace_back(value);
  }

  void SetValue(const std::string& value)
  {
    m_values.clear();
    m_values.emplace_back(value);
  }

  void MergeValue(const std::string& value)
  {
    if (m_values.back().empty())
      m_values.back().assign(value);
    else
      m_values.back().append(",").append(value);
  }

private:
  std::string m_name;
  container_t m_values;
};

#endif /* WSHEADER_H */

