/*
 *      Copyright (C) 2021 Jean-Luc Barriere
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "framebuffer.h"

#include "private/os/threads/mutex.h"
#include "private/debug.h"

using namespace NSROOT;

namespace NSROOT
{
  struct FrameBuffer::Lockable
  {
    OS::CMutex mutex;
  };
}

FrameBuffer::FrameBuffer(int capacity)
: m_lock(new Lockable())
, m_capacity(capacity)
, m_count(0)
, m_buffer()
, m_read(nullptr)
, m_write(nullptr)

{
  assert(capacity > 0);
  m_buffer.resize(capacity);
  init();
}

FrameBuffer::~FrameBuffer()
{
  OS::CLockGuard g(m_lock->mutex);
  for (std::vector<Frame*>::iterator it = m_buffer.begin(); it != m_buffer.end(); ++it)
    delete *it;
}

void FrameBuffer::init()
{
  Frame * previous = nullptr;
  for (std::vector<Frame*>::iterator it = m_buffer.begin(); it != m_buffer.end(); ++it)
  {
    *it = new Frame();
    if (previous)
      previous->next = *it;
    previous = *it;
  }
  if (m_buffer.begin() != m_buffer.end())
    previous->next = *(m_buffer.begin());
  m_write = *(m_buffer.begin());
  m_read = m_write;
}

int FrameBuffer::capacity() const
{
  return m_capacity;
}

int FrameBuffer::bytesAvailable() const
{
  OS::CLockGuard g(m_lock->mutex);
  return (m_read != m_write ? m_read->size : 0);
}

void FrameBuffer::clear()
{
  OS::CLockGuard g(m_lock->mutex);
  m_count = 0;
  m_read = m_write;
}

int FrameBuffer::write(const char * data, int len)
{
  if (len > 0)
  {
    char * _data = new char [len];
    memcpy(_data, data, len);
    {
      OS::CLockGuard g(m_lock->mutex);
      if (m_write->data)
        delete [] m_write->data;
      m_write->data = _data;
      m_write->size = len;
      m_write->id = ++m_count;
      m_write = m_write->next;
    }
  }
  return len;
}

FramePacket * FrameBuffer::read()
{
  FramePacket * p = nullptr;
  {
    OS::CLockGuard g(m_lock->mutex);
    if (m_read != m_write)
    {
      p = new FramePacket(m_read->id, m_read->size, m_read->data);
      m_read->size = 0;
      m_read->data = nullptr;
      m_read = m_read->next;
    }
  }
  return p;
}
